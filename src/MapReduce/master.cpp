#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

#include <rpc/mapreduce/Master.h>
#include <tools/ToString.hpp>

#include <fmt/core.h>
#include <glog/logging.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TServerSocket.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using std::mutex;
using std::queue;
using std::string;
using std::unordered_map;
using std::vector;
using time_point = std::chrono::steady_clock::time_point;

constexpr auto TIME_OUT = std::chrono::seconds(5);

enum class TaskState {
    IDLE = 0,
    IN_PROGRESS,
    COMPLETED
};

struct Task {
    int32_t id;
    TaskState state;
    vector<string> params;
    vector<string> results;
};

struct TaskProcessStatus {
    int32_t id;
    time_point startTime;
    time_point lastSeen;
};

class MasterHandler : virtual public MasterIf {
private:
    enum class MasterState {
        MAP_PHASE,
        REDUCE_PHASE,
        COMPLETED_PHASE
    };

public:
    MasterHandler(vector<string>& files, int reduceNumber)
        : mapTasks_(vector<Task>(files.size()))
        , reduceTasks_(vector<Task>(reduceNumber))
        , state_(MasterState::MAP_PHASE)
        , completedCnt_(0)
        , workerId(1)
    {
        for (int i = 0; i < mapTasks_.size(); i++) {
            mapTasks_[i] = Task { i, TaskState::IDLE };
            mapTasks_[i].params.push_back(files[i]);
        }

        for (int i = 0; i < mapTasks_.size(); i++)
            idle_.push(i);

        std::thread de([this]() {
            this->detectTimeoutTask();
        });
        de.detach();

        LOG(INFO) << "Successfully initialized master, number of idle tasks: " << idle_.size();
    }

    void setExitServer(std::function<void(void)> func)
    {
        exitServer_ = func;
    }

    int32_t assignId() override
    {
        LOG(INFO) << "Get an assignId request.";
        int tid = workerId.fetch_add(1);
        LOG(INFO) << "Assign id: " << tid;
        return tid;
    }

    void assignTask(TaskResponse& _return) override
    {
        std::lock_guard<std::mutex> guard(lock_);
        int tid = tryToGetIdleTask();
        _return.id = tid;
        if (tid != -1)
            setTaskToInProgress(tid);
        switch (state_) {
        case MasterState::MAP_PHASE:
            _return.type = (tid == -1 ? ResponseType::WAIT : ResponseType::MAP_TASK);
            if (tid != -1)
                _return.params = mapTasks_[tid].params;
            _return.resultNum = reduceTasks_.size();
            break;
        case MasterState::REDUCE_PHASE:
            _return.type = (tid == -1 ? ResponseType::WAIT : ResponseType::REDUCE_TASK);
            if (tid != -1)
                _return.params = reduceTasks_[tid].params;
            _return.resultNum = 1;
            break;
        case MasterState::COMPLETED_PHASE:
            _return.type = ResponseType::COMPLETED;
            break;
        default:
            LOG(FATAL) << "Unexpected state";
        }

        LOG(INFO) << "Assign task: " << _return;
    }

    void commitTask(const TaskResult& result) override
    {
        std::lock_guard<std::mutex> guard(lock_);
        LOG(INFO) << "Receive task result: " << result;

        /*
         * The worker's result may be delayed for a very long time, which is completely
         * meaningless if the Master's state has changed.
         */
        if (!(state_ == MasterState::MAP_PHASE && result.type == ResponseType::MAP_TASK)
            && !(state_ == MasterState::REDUCE_PHASE && result.type == ResponseType::REDUCE_TASK)) {
            removeFiles(result.rs_loc);
            LOG(INFO) << "Task result expired, ignore this result.";
            return;
        }

        auto& tasks = (state_ == MasterState::MAP_PHASE ? mapTasks_ : reduceTasks_);
        /*
         * This task may have been previously committed by another woker,
         * so there is no need to commit task again.
         */
        int32_t id = result.id;
        if (tasks[id].state == TaskState::COMPLETED) {
            removeFiles(result.rs_loc);
            LOG(INFO) << "Task has been committed, ignore this result.";
            return;
        }
        tasks[id].state = TaskState::COMPLETED;
        inProgress_.erase(id);

        switch (state_) {
        case MasterState::MAP_PHASE: {
            commitMapTask(result);
            completedCnt_++;
            LOG(INFO) << fmt::format("Finished tasks: {}, Total map tasks: {}", completedCnt_, mapTasks_.size());
            if (completedCnt_ == mapTasks_.size()) {
                switchToReduceState();
            }
        } break;

        case MasterState::REDUCE_PHASE: {
            commitReduceTask(result);
            completedCnt_++;
            LOG(INFO) << fmt::format("Finished tasks: {}, Total reduce tasks: {}", completedCnt_, mapTasks_.size());
            if (completedCnt_ == reduceTasks_.size()) {
                switchToCompletedState();
            }
        } break;

        default:
            LOG(FATAL) << "Unexpected state";
        }
    }

private:
    void commitMapTask(const TaskResult& result)
    {
        auto id = result.id;
        auto& rs_loc = result.rs_loc;

        vector<string> commit_files(rs_loc.size());
        for (int i = 0; i < rs_loc.size(); i++) {
            auto& name = rs_loc[i];
            // file name format: mr.mapTaskId.reduceTaskId
            string newName = fmt::format("mr.mid{}.rid{}", id, i);
            std::rename(name.c_str(), newName.c_str());
            commit_files[i] = std::move(newName);
        }
        mapTasks_[id].results = std::move(commit_files);

        LOG(INFO) << "Commit map task " << id << ", before commit: " << rs_loc
                  << ", after commit: " << mapTasks_[id].results;
    }

    void commitReduceTask(const TaskResult& result)
    {
        auto id = result.id;
        auto& rs_loc = result.rs_loc;

        if (rs_loc.size() != 1)
            LOG(FATAL) << "Reduce task " << id << " return multiple results";

        string newName = "mr-out-" + std::to_string(id);
        std::rename(rs_loc[0].c_str(), newName.c_str());
        reduceTasks_[id].results.push_back(std::move(newName));

        LOG(INFO) << "Commit reduce task " << id << ", before commit: " << rs_loc
                  << ", after commit: " << newName;
    }

    void clearIntermediateFiles()
    {
        for (Task& t : mapTasks_) {
            removeFiles(t.results);
        }
    }

    void removeFiles(vector<string> files)
    {
        for (auto& file : files) {
            std::remove(file.c_str());
        }
    }

    void switchToCompletedState()
    {
        LOG(INFO) << fmt::format("Finished {} reduce tasks, switch to COMPLETED_PHASE state.", completedCnt_);

        state_ = MasterState::COMPLETED_PHASE;
        clearIntermediateFiles();
        std::thread fin(exitServer_);
        fin.detach();
    }

    void switchToReduceState()
    {
        LOG(INFO) << fmt::format("Finished {} map tasks, switch to REDUCE_PHASE state.", completedCnt_);
        completedCnt_ = 0;
        state_ = MasterState::REDUCE_PHASE;
        for (int i = 0; i < reduceTasks_.size(); i++) {
            auto& task = reduceTasks_[i];
            task.id = i;
            task.state = TaskState::IDLE;
            task.params.reserve(mapTasks_.size());

            auto& params = task.params;
            for (int j = 0; j < mapTasks_.size(); j++) {
                params.push_back(mapTasks_[j].results[i]);
            }
        }

        for (int i = 0; i < reduceTasks_.size(); i++) {
            idle_.push(i);
        }
    }

    int32_t tryToGetIdleTask()
    {
        LOG(INFO) << "Before get an idle task, idle task number: " << idle_.size();
        auto& tasks = (state_ == MasterState::MAP_PHASE ? mapTasks_ : reduceTasks_);

        int32_t tid = -1;
        while (!idle_.empty()) {
            tid = idle_.front();
            idle_.pop();

            /*
             * If a worker node does not commit a result for too long, the task for which this worker is responsible
             * is added to the idle queue and waits for a new woker to process it. However, if the result is committed
             * before a new worker arrives, then it is no longer necessary to schedule a new worker to handle the task.
             */
            if (tasks[tid].state != TaskState::IDLE) {
                LOG(INFO) << fmt::format("Task {} has been handled, skip it.", tid);
                continue;
            } else {
                break; // find an idle task
            }
        }
        LOG(INFO) << fmt::format("Get {} task {}, idle task number: {}",
            (state_ == MasterState::MAP_PHASE ? "map" : "reduce"),
            tid, idle_.size());

        return tid;
    }

    void setTaskToInProgress(int id)
    {
        auto cur = std::chrono::steady_clock::now();
        inProgress_[id] = { id, cur, cur };
        if (state_ == MasterState::MAP_PHASE) {
            mapTasks_[id].state = TaskState::IN_PROGRESS;
        } else {
            reduceTasks_[id].state = TaskState::IN_PROGRESS;
        }
        LOG(INFO) << fmt::format("set {} task {} to in-progress, idle task number: {}",
            (state_ == MasterState::MAP_PHASE ? "map" : "reduce"),
            id, idle_.size());
    }

    void detectTimeoutTask()
    {
        while (true) {
            std::this_thread::sleep_for(TIME_OUT / 2);
            {
                std::lock_guard<mutex> guard(lock_);
                LOG(INFO) << "start to detect timeout tasks.";
                auto cur = std::chrono::steady_clock::now();
                for (auto it = inProgress_.begin(); it != inProgress_.end();) {
                    auto startTime = it->second.startTime;
                    /*
                     * For various reasons, the worker may delay or interrupt execution,
                     * at which point the master node will assign the task to other worker
                     */
                    if (cur - startTime > TIME_OUT) {
                        auto id = it->first;
                        LOG(INFO) << fmt::format("Task {} time out, reset it's state to IDLE.", id);
                        auto& tasks = (state_ == MasterState::MAP_PHASE ? mapTasks_ : reduceTasks_);
                        idle_.push(id);
                        tasks[id].state = TaskState::IDLE;
                        it = inProgress_.erase(it);
                    } else {
                        it++;
                    }
                }
                LOG(INFO) << "detect timeout tasks finished.";
            }
        }
    }

private:
    vector<Task> mapTasks_;
    vector<Task> reduceTasks_;
    queue<int> idle_;
    mutex lock_;
    unordered_map<int, TaskProcessStatus> inProgress_;
    MasterState state_;
    int32_t completedCnt_;
    std::atomic<int> workerId;
    std::function<void(void)> exitServer_;
};

int main(int argc, char** argv)
{
    FLAGS_log_dir = "../logs/master";
    google::InitGoogleLogging(argv[0]);

    if (argc < 2) {
        std::cout << "Usage: ./master file1 file2 ..." << std::endl;
        exit(-1);
    }

    LOG(INFO) << "INPUT ARGS: " << argc;

    vector<string> files(argc - 1);
    for (int i = 1; i < argc; i++) {
        files[i - 1] = string(argv[i]);
        LOG(INFO) << i << "-th input file: " << files[i - 1];
    }
    LOG(INFO) << "Total " << files.size() << " files wait to be handled";

    int port = 8888;
    std::shared_ptr<MasterHandler> handler(new MasterHandler(files, 8));
    std::shared_ptr<TProcessor> processor(new MasterProcessor(handler));
    std::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
    std::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    std::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

    TThreadedServer server(processor, serverTransport, transportFactory, protocolFactory);

    handler->setExitServer([&server]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        LOG(INFO) << "The master node has finished its work and now exits!";
        google::FlushLogFiles(google::INFO);
        server.stop();
    });

    LOG(INFO) << "start master!";
    server.serve();

    google::FlushLogFiles(google::INFO);
    return 0;
}
