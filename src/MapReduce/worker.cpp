#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <dlfcn.h>
#include <fmt/core.h>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include <mapreduce/MapReduce.h>
#include <rpc/mapreduce/Master.h>

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using std::string;
using std::unordered_map;
using std::vector;

constexpr auto TIME_OUT = std::chrono::seconds(5);

DEFINE_double(exit_possibility, 0, "exit possibility");
DEFINE_double(delay_possibility, 0, "delay possibility");
DEFINE_string(map_reduce_func, "", "map reduce dynamic library");

class Worker {
public:
    Worker(std::shared_ptr<TProtocol> protocol, MapFunc mapf, ReduceFunc reducef)
        : master_(MasterClient(protocol))
        , mapf_(mapf)
        , reducef_(reducef)
    {
    }

    void run()
    {
        LOG(INFO) << "Try to get id from master.";
        id_ = master_.assignId();
        LOG(INFO) << "Get id " << id_ << " from master.";
        while (true) {
            LOG(INFO) << "Worker " << id_ << " request a task.";
            TaskResponse res;
            master_.assignTask(res);
            LOG(INFO) << "Worker " << id_ << " get task: " << res;

            simulateFailure();
            switch (res.type) {
            case ResponseType::WAIT: {
                LOG(INFO) << "Worker " << id_ << " sleep for a while.";
                std::this_thread::sleep_for(std::chrono::seconds(1));
            } break;

            case ResponseType::COMPLETED: {
                LOG(INFO) << "Worker " << id_ << " exit.";
                return;
            } break;

            case ResponseType::MAP_TASK: {
                LOG(INFO) << fmt::format("Worker {} do map task {}.", id_, res.id);
                vector<string> files = doMapTask(res);
                TaskResult rs;
                rs.id = res.id;
                rs.type = res.type;
                rs.rs_loc = std::move(files);
                master_.commitTask(rs);
                LOG(INFO) << fmt::format("Worker {} commit map task {}.", id_, res.id);
            } break;

            case ResponseType::REDUCE_TASK: {
                LOG(INFO) << fmt::format("Worker {} do reduce task {}.", id_, res.id);
                vector<string> files = doReduceTask(res);
                TaskResult rs;
                rs.id = res.id;
                rs.type = res.type;
                rs.rs_loc = std::move(files);
                master_.commitTask(rs);
                LOG(INFO) << fmt::format("Worker {} commit reduce task {}.", id_, res.id);
            } break;

            default:
                LOG(FATAL) << "Unexpected state!";
            }
        }
    }

private:
    void simulateFailure()
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 10000);
        double r1 = distrib(gen) / 10000.0;
        double r2 = distrib(gen) / 10000.0;
        if (r1 < FLAGS_exit_possibility) {
            LOG(INFO) << fmt::format("Worker {} exited due to a fault.", id_);
            exit(0);
        }

        if (r2 < FLAGS_delay_possibility) {
            LOG(INFO) << fmt::format("Worker {} delay due to a fault.", id_);
            std::this_thread::sleep_for(TIME_OUT);
        }
    }

    vector<string> doMapTask(TaskResponse& res)
    {
        string content = readFile(res.params[0]);
        vector<KeyValue> rs = mapf_(res.params[0], content);

        vector<string> files(res.resultNum);
        string prefix = fmt::format("mr-wid{}-tid{}", id_, res.id);
        for (int i = 0; i < files.size(); i++)
            files[i] = prefix + "_rid" + std::to_string(i);
        divideResultsToFile(rs, files);
        return files;
    }

    vector<string> doReduceTask(TaskResponse& res)
    {
        auto& files = res.params;

        vector<KeyValue> keyvals;
        string key, val;
        for (int i = 0; i < files.size(); i++) {
            std::ifstream ifs(files[i]);
            while (ifs >> key >> val) {
                keyvals.push_back({ key, val });
            }
        }
        LOG(INFO) << fmt::format("Collect {} kvs from {} files", keyvals.size(), files.size());

        vector<KeyValue> kvs = reduceKV(keyvals);
        string fileName = "mr-out-" + std::to_string(res.id);
        std::ofstream ofs(fileName);
        for (auto kv : kvs) {
            ofs << kv.key << '\t' << kv.val << '\n';
        }

        LOG(INFO) << fmt::format("The reduce task {} get {} kvs, results are stored in {}", res.id, kvs.size(), fileName);
        return { fileName };
    }

    vector<KeyValue> reduceKV(vector<KeyValue>& kvs)
    {
        if (kvs.empty())
            return {};
        std::sort(kvs.begin(), kvs.end(), [](const KeyValue& kv1, const KeyValue& kv2) {
            return kv1.key < kv2.key;
        });

        string preKey = kvs[0].key;
        vector<string> vals;
        vector<KeyValue> ans;
        for (int i = 0; i < kvs.size(); i++) {
            if (preKey != kvs[i].key) {
                vector<string> rs = reducef_(preKey, vals);
                if (!rs.empty()) {
                    ans.push_back({ preKey, rs[0] });
                }
                vals.clear();
                preKey = kvs[i].key;
                vals.push_back(kvs[i].val);
            } else {
                vals.push_back(kvs[i].val);
            }
        }
        vector<string> rs = reducef_(preKey, vals);
        if (!rs.empty()) {
            ans.push_back({ preKey, rs[0] });
        }
        return ans;
    }

    void divideResultsToFile(vector<KeyValue>& rs, vector<string>& resultFiles)
    {
        const int N = resultFiles.size();
        vector<std::ofstream> files(N);
        for (int i = 0; i < N; i++) {
            files[i] = std::ofstream(resultFiles[i]);
        }

        for (int i = 0; i < rs.size(); i++) {
            int h = std::hash<string> {}(rs[i].key) % N;
            files[h] << rs[i].key << '\t' << rs[i].val << '\n';
        }
    }

    std::string readFile(string& name)
    {
        std::ifstream ifs(name);
        ifs.seekg(0, std::ios::end);
        std::size_t size = ifs.tellg();

        LOG(INFO) << fmt::format("Read file {}, file size: {} bytes", name, size);
        string buf(size, ' ');
        ifs.seekg(0);
        ifs.read(&buf[0], size);
        return buf;
    }

private:
    MasterClient master_;
    MapFunc mapf_;
    ReduceFunc reducef_;
    int32_t id_;
};

std::pair<MapFunc, ReduceFunc> loadFunc()
{
    LOG(INFO) << "Load MapReduce functions.";
    void* handle = dlopen(FLAGS_map_reduce_func.c_str(), RTLD_LAZY);
    if (!handle) {
        LOG(FATAL) << "Cannot open library: " << dlerror();
    }

    MapFunc mapf = (MapFunc)dlsym(handle, "wordCountMapF");
    ReduceFunc reducef = (ReduceFunc)dlsym(handle, "wordCountReduceF");
    if (mapf == nullptr || reducef == nullptr) {
        dlclose(handle);
        LOG(FATAL) << "cannot load symbol wordCountMapF or wordCountReduceF: " << dlerror();
    }
    return { mapf, reducef };
}

int main(int argc, char** argv)
{
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);

    LOG(INFO) << "Set log_dir: " << FLAGS_log_dir;
    LOG(INFO) << fmt::format("exit_possibility: {}, delay_possibility: {}", FLAGS_exit_possibility, FLAGS_delay_possibility);
    LOG(INFO) << "dynamic library: " << FLAGS_map_reduce_func;

    std::shared_ptr<TTransport> socket(new TSocket("localhost", 8888));
    std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
    std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
    google::FlushLogFiles(google::INFO);

    auto funcs = loadFunc();
    try {
        transport->open();
        Worker w(protocol, funcs.first, funcs.second);
        LOG(INFO) << "A worker start working";
        w.run();
        transport->close();
    } catch (TException& tx) {
        LOG(WARNING) << "Warning: " << tx.what();
    }
    google::FlushLogFiles(google::INFO);
    return 0;
}