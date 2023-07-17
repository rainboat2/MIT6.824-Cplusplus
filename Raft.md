# Raft实现

精读论文：[In Search of an Understandable Consensus Algorithm (Extended Version)](https://pages.cs.wisc.edu/~remzi/Classes/739/Spring2004/Papers/raft.pdf)

## 选举算法

通过选举算法，能确保在同一个时间内只会有一个leader。即使出现故障，只要故障节点数少于总数的一半，就能在一个选举超时周期内恢复(150ms~300ms)，下面列举几个关键的点：

### 超时选举

raft节点会随机睡眠一段时间（在150ms~300ms之内），醒来后会检查上次见到leader的时间，如果时间超时，那么可以认为leader已经出现故障，这种情况下就会开启新一轮的投票，选出新的leader。

```c++
void RaftRPCHandler::async_checkLeaderStatus()
{
    while (true) {
        std::this_thread::sleep_for(getElectionTimeout());   // 随机睡眠一段时间
        {
            std::lock_guard<std::mutex> guard(lock_);
            switch (state_) {
            case ServerState::CANDIDAE:
            case ServerState::FOLLOWER: {
                if (NOW() - lastSeenLeader_ > MIN_ELECTION_TIMEOUT) {       // 超时选举
                    LOG(INFO) << "Election timeout, start a election.";
                    switchToCandidate();
                }
            } break;
                LOG(INFO) << "Raft become a leader, exit the checkLeaderStatus thread!";
                return;
            default:
                LOG(FATAL) << "Unexpected state!";
            }
        }
    }
}
```



### 选举流程

下面给出了一个节点成为candidate之后，向其它节点请求选票的核心流程，实现的过程中值得注意的几个点：

1. 为了效率起见，需要**同时**给所有的节点发送requestVote请求，发送请求的代码应该避免使用互斥锁，否则还是会一个个发送
2. 在CANDIDAE获取了大多数选票后，应该立即成为leader，否则可能会因为某个节点的延迟，导致整个选举流程被拖慢，从而出现选举超时的情况（本代码使用条件变量解决这一问题）。
3. 使用原子变量，解决计数的需求，避免频繁获取和释放锁，降低效率。

```c++
// 使用原子变量避免出现race condition
std::atomic<int> voteCnt(1);
std::atomic<int> finishCnt(0);
std::mutex finish;
std::condition_variable cv;
  
vector<std::thread> threads(peersForRV.size());
for (int i = 0; i < peersForRV.size(); i++) {
    threads[i] = std::thread([i, this, &peersForRV, &params, &voteCnt, &finishCnt, &cv]() {
        RequestVoteResult rs;
        RaftRPCClient* client;
        try {
            client = cmForRV_.getClient(i, peersForRV[i]);
            client->requestVote(rs, params);  // 发送选票请求
        } catch (TException& tx) {
            LOG(ERROR) << fmt::format("Request {} for voting error: {}", to_string(peersForRV[i]), tx.what());
            cmForRV_.setInvalid(i);
        }

        if (rs.voteGranted)
            voteCnt++;   // 选票+1
        finishCnt++;
        cv.notify_one();
    });
}


int raftNum = peers_.size() + 1;
std::unique_lock<std::mutex> lk(finish);
// 使用条件变量，当收集到足够多的选票后就可以立刻转换为leader，向其它节点发送心跳包，阻止其它节点发起下一轮选举
cv.wait(lk, [raftNum, &finishCnt, &voteCnt]() {
    return voteCnt > (raftNum / 2) || (finishCnt == raftNum - 1);
});

if (voteCnt > raftNum / 2) {
    std::lock_guard<std::mutex> guard(lock_);
    if (state_ == ServerState::CANDIDAE)
        switchToLeader();
    else
        LOG(INFO) << fmt::format("Raft is not candidate now!");  // 选举过程中可能会出现更高任期的leader，此时应该放弃选举
}
```



### 发送心跳包

如下为Leader节点定期给其它所有节点发送心跳包的核心代码，在实现的过程中，主要需要注意以下几点：

1. 只有Leader节点才能发送心跳包，因此每次发送之前需要检查一下当前节点是否为Leader
2. 应该先发送心跳包，再sleep，这样成为leader后调用这个函数就能立刻通知到所有其它节点。

```c++
while (true) {
    {
        std::lock_guard<std::mutex> guard(lock_);
        if (state_ != ServerState::LEADER)
            return;
    }

    vector<std::thread> threads(peers_.size());
    for (int i = 0; i < peers_.size(); i++) {
        threads[i] = std::thread([i, &peersForHB, this]() {
            RaftAddr addr;
            AppendEntriesParams params;
            params.term = currentTerm_;
            params.leaderId = me_;
            try {
                RaftRPCClient* client;
                addr = peersForHB[i];
                client = cmForHB_.getClient(i, addr);

                auto app_start = NOW();
                AppendEntriesResult rs;
                client->appendEntries(rs, params);
            } catch (TException& tx) {
                cmForHB_.setInvalid(i);
            }
        });
    }

    for (int i = 0; i < peers_.size(); i++) {
        threads[i].detach();
    }
    std::this_thread::sleep_for(HEART_BEATS_INTERVAL);
}

```



## 日志同步



## 实现过程中碰到的问题

1. raft进程随机奔溃的问题，特别是在之前运行了一大堆测试代码或者电脑没有插电源的情况。分析：leader会为每个follower维护一个nextIndex，用于指向下一条需要向子节点传输的日志。如果发送的日志过于超前，就会将nextIndex对应的位置减1。

```c++
AppendEntriesResult rs;
try {
    auto* client_ = cmForAE_.getClient(i, peersForAE[i]);
    client_->appendEntries(rs, params);
    LOG(INFO) << fmt::format("Send {} logs to {}, the result is: ", logsNum, to_string(peersForAE[i])) << rs;
} catch (TException& tx) {
    cmForAE_.setInvalid(i);
    LOG(INFO) << fmt::format("Send logs to {} failed: ", to_string(peersForAE[i]), tx.what());
    return;    // 没有加这个return，导致将通讯出错的情况当作follower拒绝日志的逻辑处理，从而导致nextIndex里对应的位置被减成负数，出现访问空指针的情况
}

{
    std::lock_guard<std::mutex> guard(raftLock_);
    handleAEResultFor(i, rs, logsNum);
}Ï
```

