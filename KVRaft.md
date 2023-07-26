# KVRaft实现原理

本节的内容为基于raft协议，实现一个容错的分布式KV系统，该只要超过半数的节点正常工作，该KV系统就能正常运行。

> 精读论文：[In Search of an Understandable Consensus Algorithm (Extended Version)](https://pages.cs.wisc.edu/~remzi/Classes/739/Spring2004/Papers/raft.pdf)

## KVServer

### 总体设计

如下的代码给出KVServer节点的一个总体设计，这个节点主要包括三部分的内容：

1. 对客户端响应的KV接口，主要包括putAppend和get两种方法。需要注意的是，只有leader节点才会响应客户端的请求，否则会给客户端返回一个错误状态码。
2. raft集群内部沟通的几个方法（`RaftIf`）。一般来说在设计kvraft集群的时候，每个kv节点都会对应一个raft节点，每对kv节点和raft节点之间可以独立运行监听不同的端口，通过RPC调用进行沟通，也可以合并成一个整体。本实现从运行效率和代码实现复杂度的角度考虑，采用的就是合并成一个整体的方法。
3. 状态机相关的方法（`StateMachineIf`)。raft协议不考虑被管理的kv节点的内部实现细节，而是将其当作做一个状态机，这一点在raft论文里面有详细的说明。

```c++
class KVServer : virtual public KVRaftIf,
                 virtual public StateMachineIf {
public:
    KVServer(std::vector<Host>& peers, Host me, std::string persisterDir, std::function<void()> stopListenPort);

    /*
     * methods for KVRaftIf
     */
    void putAppend(PutAppendReply& _return, const PutAppendParams& params) override;
    void get(GetReply& _return, const GetParams& params) override;

    /*
     * methods for RaftIf
     */
    void requestVote(RequestVoteResult& _return, const RequestVoteParams& params) override;
    void appendEntries(AppendEntriesResult& _return, const AppendEntriesParams& params) override;
    void getState(RaftState& _return) override;
    void start(StartResult& _return, const std::string& command) override;

    /*
     * methods for state machine
     */
    void apply(ApplyMsg msg) override;
    void startSnapShot(std::string fileName, std::function<void()> callback) override;
};
```



### KV操作

KV操作包括`Get`和`PutAppend`两类，下面是`PutAppend`操作为例说明实现原理，leader节点在接受到KV操作请求后，会首先构建操作日志，请求raft集群进行同步。在raft集群提交了这条日志后，会通过`apply`方法执行这条日志里面保存的命令，最终完成请求，返回结果。实现过程中主要需要注意以下几点：

1. 只有Leader才会执行KV操作，其它类型的节点收到请求后会拒绝
2. raft集群同步日志，提交日志最后调用`apply`方法，这是一个异步回调的过程。由于等待处理KV操作的线程有多个，而处理apply方法通常只有一个，这种场景比较适合使用`future/promise`来进行同步。

```c++
void KVServer::putAppend(PutAppendReply& _return, const PutAppendParams& params)
{
    StartResult sr;
    string type = (params.op == PutOp::PUT ? "PUT" : "APPEND");
    string command = fmt::format("{} {} {}", type, params.key, params.value);
    raft_->start(sr, command);                     // 请求raft集群同步日志

    if (!sr.isLeader) {
        _return.status = KVStatus::ERR_WRONG_LEADER;     // 非Leader节点会拒绝处理请求
        return;
    }

    auto logId = sr.expectedLogIndex;
    std::future<PutAppendReply> f;                     // 采用promise/future来等待完成

    {
        std::lock_guard<std::mutex> guard(lock_);
        f = putWait_[logId].get_future();
    }

    f.wait();                                         // 等待raft集群提交日志后完成执行
    _return = std::move(f.get());
}
```



### apply操作

如下为`apply`操作对应的实现逻辑，主要包括处理请求，设置返回值两个步骤，实现的过程需要注意两个点：

1. 只有leader节点才需要设置返回值，其它类型的节点只用执行操作即可

```c++

void KVServer::apply(ApplyMsg msg)
{
    auto& cmd = msg.command;
    std::istringstream iss(cmd.c_str());

    // use rfind to simulate "startswith"
    if (cmd.rfind("APPEND", 0) == 0 || cmd.rfind("PUT", 0) == 0) {
        PutAppendParams params;
        string type;
        iss >> type >> params.key >> params.value;
        params.op = (cmd[0] == 'A' ? PutOp::APPEND : PutOp::PUT);

        PutAppendReply reply;
        putAppend_internal(reply, params);

        {
            std::lock_guard<std::mutex> guard(lock_);
            if (putWait_.find(msg.commandIndex) != putWait_.end()) {
                putWait_[msg.commandIndex].set_value(std::move(reply));
                putWait_.erase(msg.commandIndex);
            } else {
                LOG(INFO) << fmt::format("Command {} is not in the putWait_", msg.commandIndex);
            }
        }
        LOG(INFO) << "Apply put command: " << msg.command;
    } else if (cmd.rfind("GET", 0) == 0) {
        GetParams params;
        string type;
        iss >> type >> params.key;
        GetReply reply;
        get_internal(reply, params);
        {
            std::lock_guard<std::mutex> guard(lock_);
            if (getWait_.find(msg.commandIndex) != getWait_.end()) {
                getWait_[msg.commandIndex].set_value(std::move(reply));
                getWait_.erase(msg.commandIndex);
            } else {
                LOG(INFO) << fmt::format("Command {} is not in the getWait_", msg.commandIndex);
            }
        }
        LOG(INFO) << "Apply get command: " << msg.command;
    } else {
        LOG(ERROR) << "Invaild command: " << cmd;
    }
    lastApply_ = msg.commandIndex;
}
```

