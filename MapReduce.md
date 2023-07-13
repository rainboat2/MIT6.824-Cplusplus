# MapReduce 实现思路

> 精读论文: [MapReduce: Simplified Data Processing on Large Clusters](https://static.googleusercontent.com/media/research.google.com/zh-CN//archive/mapreduce-osdi04.pdf)

## Master节点

### 整体设计

<<<<<<< HEAD
Master节点负责统筹全局，分配任务。如下为Master的整体设计，本着大道至简的原则，其对woker只提供三个函数：
=======
Master节点负责统筹全局，分配任务。如下为Master的整体设计，本着大道至简的原则，其对woker提供三个函数：
>>>>>>> 706695bce8fad63cbfb1b6b9951c5155656c811b

```C++
class MasterHandler : virtual public MasterIf {
private:
    enum class MasterState {
        MAP_PHASE,
        REDUCE_PHASE,
        COMPLETED_PHASE
    };

public:
  	// 给worker节点分配id
    int32_t assignId() override;

  	// 依据当前的状态，给worker节点分配Map或Reduce任务，若暂时没有任务，则让worker过会再来询问
    void assignTask(TaskResponse& _return) override;

  	// 接收worker完成的任务结果
    void commitTask(const TaskResult& result) override;

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
```



### 并发控制

一个master可以同时指挥多个worker进程，因此采用了以下两种不同的并发控制方式

```c++
class MasterHandler : virtual public MasterIf {
private:
		// ...
    mutex lock_;  // 互斥锁，在assignTask和commitTask时对成员变量上锁，互斥访问整体的工作状态
  	// ..
    std::atomic<int> workerId;   // 原子变量，负责给每个worker节点提供id
};
```



### 容错

在大量worker参与计算时，容易存在小部分worker故障的情况，master提供了对这种故障的检测以及应对措施，如下为实现思路

```c++
// 在master启动时，该函数就会被一个背景线程调用，在master的整个生命周期里面，周期型的检查inProgress_里面任务的状态
// 如果任务超时，则任务负责该任务的worker出现了问题，对应的任务状态会被重置为IDLE，分配给其它worker处理。
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
```



## Worker节点

### 整体设计

每个worker都以单进程单线的方式运行，不断的向master请求，完成任务，直到结束：

```c++
void run()
    {
        id_ = master_.assignId();    // 在正式开始工作之前，请求一个id
        while (true) {
            TaskResponse res;
            master_.assignTask(res);  // 向master请求Task
            simulateFailure();       // 模拟各类故障
            switch (res.type) {
            case ResponseType::WAIT: {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            } break;
            case ResponseType::COMPLETED: {
                return;
            } break;
            case ResponseType::MAP_TASK: {
                vector<string> files = doMapTask(res);   // 执行map任务
              	// add files into rs (TaskResult)
                master_.commitTask(rs);
            } break;
            case ResponseType::REDUCE_TASK: {
                vector<string> files = doReduceTask(res);  // 执行task任务
                // add files into rs (TaskResult)
                master_.commitTask(rs);
            } break;
            default:
                LOG(FATAL) << "Unexpected state!";
            }
        }
    }
```



### 动态加载Map和Reduce函数

用户按照`MapFunc`和`ReduceFunc`的接口提供相应的函数，编译成so文件

```c++
struct KeyValue
{
    std::string key;
    std::string val;
};

using MapFunc = std::vector<KeyValue> (*)(std::string& key, std::string& value);
using ReduceFunc = std::vector<std::string> (*)(std::string& key, std::vector<std::string>& value);
```

调用`dlopen`和`dlsym`函数动态加载用户自定义的函数

```c++
std::pair<MapFunc, ReduceFunc> loadFunc()
{
    LOG(INFO) << "Load MapReduce functions.";
    void* handle = dlopen("../user-program/WordCount.so", RTLD_LAZY);
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
```

