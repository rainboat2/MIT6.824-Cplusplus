# Raft实现

> 精读论文：[In Search of an Understandable Consensus Algorithm (Extended Version)](https://pages.cs.wisc.edu/~remzi/Classes/739/Spring2004/Papers/raft.pdf)

## 选举算法

通过选举算法，能确保在同一个时间内只会有一个leader。即使出现故障，只要故障节点数少于总数的一半，就能在一个选举超时周期内恢复(150ms~300ms)，下面列举几个关键的点：

### 超时选举

raft节点会随机睡眠一段时间（在150ms~300ms之内），醒来后会检查上次见到leader的时间，如果时间超时，那么可以认为leader已经出现故障，这种情况下就会开启新一轮的投票，选出新的leader。

```c++
void RaftHandler::async_checkLeaderStatus()
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

### RequestVote

如下为请求投票的的逻辑，大致的逻辑都已经在注释里面标注了出来，实现的过程中需要着重注意的点：

1. 在收到高任期的选票时，要重置当前的节点的选举超时。否则在投票之后，选出新leader的这段时间内，如果本节点超时，会提升任期开启新一轮的选举，直接导致本轮选举的失败。

```c++

void RaftHandler::requestVote(RequestVoteResult& _return, const RequestVoteParams& params)
{
    std::lock_guard<std::mutex> guard(raftLock_);
	
  	// 来自高任期节点的选票请求，接受！
    if (params.term > currentTerm_) {
        if (state_ != ServerState::FOLLOWER)
            switchToFollow();
        currentTerm_ = params.term;
        votedFor_ = NULL_HOST;
        lastSeenLeader_ = NOW();
    }

  	// 来自低任期节点的选票请求，直接否决！
    if (params.term < currentTerm_) {
        LOG(INFO) << fmt::format("Out of fashion vote request from {}, term: {}, currentTerm: {}",
            to_string(params.candidateId), params.term, currentTerm_);
        _return.term = currentTerm_;
        _return.voteGranted = false;
        return;
    }
		
  	// 来自当前任期的选票请求，如果没有投票，接受！
    if (params.term == currentTerm_ && votedFor_ != NULL_HOST && votedFor_ != params.candidateId) {
        LOG(INFO) << fmt::format("Receive a vote request from {}, but already voted to {}, reject it.",
            to_string(params.candidateId), to_string(votedFor_));
        _return.term = currentTerm_;
        _return.voteGranted = false;
        return;
    }

		// 投票前需要判断一下谁的日志更新，不接受日志更旧的节点的投票请求
    auto lastLog = logs_.back();
    if (lastLog.term < params.LastLogTerm || (lastLog.term == params.LastLogTerm && lastLog.index < params.lastLogIndex)) {
        LOG(INFO) << fmt::format("Receive a vote request from {} with outdate logs, reject it.", to_string(params.candidateId));
    }

    votedFor_ = params.candidateId;
    _return.voteGranted = true;
    _return.term = currentTerm_;
    persister_.saveRaftState(this);
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
        RaftClient* client;
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

1. 心跳包除了告诉follower自己还活着之外，还承担了同步日志index，同步commitIndex等重要功能，因此发送新跳包的逻辑应该与拷贝日志的逻辑一致（这一点很重要，课程官网特别强调了这一点）
2. 只有Leader节点才能发送心跳包，因此每次发送之前需要检查一下当前节点是否为Leader
3. 应该先发送心跳包，再sleep，这样成为leader后调用这个函数就能立刻通知到所有其它节点。

```c++
while (true) {
  vector<AppendEntriesParams> params(peersForHB.size());
  {
      std::lock_guard<std::mutex> guard(raftLock_);
      if (state_ != ServerState::LEADER)     // 只有leader才能发送心跳包
          return;
      for (int i = 0; i < peersForHB.size(); i++) {
          params[i] = buildAppendEntriesParamsFor(i);   // 由于心跳包也承担了同步日志index，commit状态的任务，因此需要为每个follower节点单独构建一个参数
      }
  }

  auto startNext = NOW() + HEART_BEATS_INTERVAL;
  vector<std::thread> threads(peersForHB.size());
  for (int i = 0; i < peersForHB.size(); i++) {
      threads[i] = std::thread([i, &peersForHB, &params, this]() {    // 为每个follower都创建一个线程，并发发送
          Host addr;
          try {
              RaftClient* client;
              addr = peersForHB[i];
              client = cmForHB_.getClient(i, addr);
              AppendEntriesResult rs;
              client->appendEntries(rs, params[i]);     // RPC发起appendEntries调用
              {
                   std::lock_guard<std::mutex> guard(raftLock_);
                   handleAEResultFor(i, paramsList[i], rs);
              }
          } catch (TException& tx) {
              LOG_EVERY_N(ERROR, HEART_BEATS_LOG_COUNT) << fmt::format("Send {} heart beats to {} failed: {}",
                  HEART_BEATS_LOG_COUNT, to_string(addr), tx.what());
              cmForHB_.setInvalid(i);
          }
      });
  }

  for (int i = 0; i < peersForHB.size(); i++) {
      threads[i].join();
  }
  LOG_EVERY_N(INFO, HEART_BEATS_LOG_COUNT) << fmt::format("Broadcast {} heart beats", HEART_BEATS_LOG_COUNT);

  std::this_thread::sleep_until(startNext);
}
```



## 日志同步

当客户端发送一个新的指令，状态机会首先给raft集群里面的leader发送一个同步的请求。leader为这条指令创建一个日志，并开始日志的拷贝。当日志被复制到绝大多数的raft节点上之后，这条日志就被标记为commit状态，可以被安全的应用到状态机上。下面说明实现日志的几个重要流程。

### 启动同步流程

状态机收到客户端的指令后，会首先通过调用Leader的`start`函数来启动线程，如果被调用raft节点的不是leader，则该请求会被拒绝，需要重试找到leader。如果是leader，就会创建一条日志，并开启同步流程。该函数实现需要注意如下几点：

1. 用户的请求比较随机，直接发送心跳包的逻辑就可以完成日志同步，然而每次睡眠时间过长会导致日志同步慢，而睡眠时间太短则会导致过高的CPU占用，因此使用条件变量，在有新的日志到来时，立刻通知线程开始工作。
2. 使用条件变量需要注意，如果没有线程等待条件变量，那么notify_one发出的信号会丢失，从而造成一条日志不能被及时同步，在编程的时候需要考虑notify_one丢失的情况。

```c++
void RaftHandler::start(StartResult& _return, const std::string& command)
{
    std::lock_guard<std::mutex> guard(raftLock_);

    if (state_ != ServerState::LEADER) {
        _return.isLeader = false;              // 只有leader才能开启一个日志的同步
        return;
    }
  
    LogEntry log;                             // 创建一个新的日志
    log.index = logs_.back().index + 1;
    log.command = command;
    log.term = currentTerm_;
    logs_.push_back(std::move(log));

    _return.expectedLogIndex = log.index;
    _return.term = currentTerm_;
    _return.isLeader = true;

    sendEntries_.notify_one();               // 使用条件变量，通知拷贝日志的线程开始工作
}
```



### 构建日志同步参数

构建日志的参数分为了两个函数，分别是`buildAppendEntriesParamsFor`和`gatherLogsFor`两个函数，这是因为发送心跳包和同步日志都需要构建AppendEntriesParams参数，而发送心跳包不需要日志，也就是不要`gatherLogsFor`函数里面的内容，因此将逻辑分为两块，便于复用函数。

```c++
AppendEntriesParams RaftHandler::buildAppendEntriesParamsFor(int peerIndex)
{
    AppendEntriesParams params;
    params.term = currentTerm_;
    params.leaderId = me_;
    auto& prevLog = getLogByLogIndex(nextIndex_[peerIndex] - 1);
    params.prevLogIndex = prevLog.index;
    params.prevLogTerm = prevLog.term;
    params.leaderCommit = commitIndex_;
    return params;
}

int RaftHandler::gatherLogsFor(int peerIndex, AppendEntriesParams& params)
{
    int target = logs_.back().index;
    if (nextIndex_[peerIndex] > target)
        return 0;
    int logsNum = std::min(target - nextIndex_[peerIndex] + 1, MAX_LOGS_PER_REQUEST);
    for (int j = 0; j < logsNum; j++) {
        params.entries.push_back(getLogByLogIndex(j + nextIndex_[peerIndex]));
    }
    return logsNum;
}
```

### 处理同步日志的返回值

这个函数需要做的就是依据返回的值，更新raft相应的状态，具体逻辑如下：

1. 如果发送成功，更新`nextIndex_[i]`和`matchIndex[i]`的值
2. 如果发送成功，依据`matchIndex`数组和当前的`commitIndex_`值，找到大多数节点都同意的一个值，来更新`commitIndex_`。需要注意的是，这是一个典型的topk问题，但是问题规模非常小，所以为了简化编程，直接使用了库函数里面提供的sort来解决
3. 提交index的时候一定要判断提交的日志term是不是和当前的term保持一致。为了避免出现已提交的日志被覆盖的情况，一个leader不会提交之前任期的日志，这一点非常重要，在论文里面花了一个小节特地来讲。
4. 如果发送失败，则奖`nextIndex[i]`的值后退到`params.prevLogIndex`。这个地方在编写的时候需要考虑到其它的线程可能已经更新了这个nextIndex[i]好几次，注意不要覆盖更新或是受到这些更新的影响。

```c++
void RaftHandler::handleAEResultFor(int peerIndex, const AppendEntriesParams& params, const AppendEntriesResult& rs)
{
    int i = peerIndex;
    if (rs.success) {
        nextIndex_[i] += params.entries.size();
        matchIndex_[i] = nextIndex_[i] - 1;
        /*
         * Update commitIndex_. This is a typical top k problem, since the size of matchIndex_ is tiny,
         * to simplify the code, we handle this problem by sorting matchIndex_.
         */
        auto matchI = matchIndex_;
        matchI.push_back(logs_.back().index);
        sort(matchI.begin(), matchI.end());
        int agreeIndex = matchI[matchI.size() / 2];
        if (getLogByLogIndex(agreeIndex).term == currentTerm_)
            commitIndex_ = agreeIndex;
    } else {
        nextIndex_[i] = std::min(nextIndex_[i], params.prevLogIndex);
    }
}
```



### 接收日志

follower接收leader发送的日志，并返回成果或是失败的消息，实现的过程中主要需要注意以下几点

1. 一定要严格按照论文Fig.2所描述的逻辑来，此前没有写对齐params.entry和raft节点logs_的逻辑，简单的判断如果params里面prevLogIndex要小于当前日志的末尾的index，就将后续多余的日志全部删掉。结果导致一个心跳包和一个发送日志的请求同时发送时，心跳包可能会导致刚append的日志又被删掉这一问题。
2. 依据leader节点提供的commit信息，更新自己的commit信息。

```c++

void RaftHandler::appendEntries(AppendEntriesResult& _return, const AppendEntriesParams& params)
{
    std::lock_guard<std::mutex> guard(raftLock_);

    _return.success = false;
    _return.term = currentTerm_;

    // ...
    // Check validity of params
    lastSeenLeader_ = NOW();

    /*
     * find the latest log entry where the two logs agree
     */
    if (params.prevLogIndex > logs_.back().index) {
        LOG(INFO) << fmt::format("Expected older logs. prevLogIndex: {}, param.prevLogIndex: {}",
            logs_.back().index, params.prevLogIndex);
        return;
    }

    auto preLog = getLogByLogIndex(params.prevLogIndex);
    if (params.prevLogTerm != preLog.term) {
        LOG(INFO) << fmt::format("Expected log term {} at index {}, get {}, delete all logs after it!",
            params.prevLogTerm, preLog.index, preLog.term);
        while (logs_.back().index >= params.prevLogIndex)
            logs_.pop_back();
    }

    {
        // If an existing entry conficts with a new one (same index but different terms), delete it!;
        int i;
        for (i = 0; i < params.entries.size(); i++) {
            auto& newEntry = params.entries[i];
            if (newEntry.index > logs_.back().index)
                break;
            auto& entry = getLogByLogIndex(newEntry.index);
            if (newEntry.term != entry.term) {
                LOG(INFO) << fmt::format("Expected log term {} at index {}, get {}, delete all logs after it!",
                    newEntry.term, entry.index, entry.term);
                while (logs_.back().index >= newEntry.index)
                    logs_.pop_back();
                break;
            }
        }

        // append any new entries not aleady in the logs_
        for (; i < params.entries.size(); i++) {
            auto& entry = params.entries[i];
            logs_.push_back(entry);
        }
    }

    commitIndex_ = std::min(params.leaderCommit, logs_.back().index);
    _return.success = true;
}
```



## 实现过程中碰到的一些问题

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



2. 函数设计有问题，不论状态直接减少nextIndex里面的值，当心跳包和日志同步请求同时发送后，就会导致nextIndex被错误的减少两两次：

```c++
void RaftHandler::handleAEResultFor(int peerIndex, const AppendEntriesParams& params, const AppendEntriesResult& rs)
{
    int i = peerIndex;
    if (rs.success) {
      // update nextIndex_[i]
      // update matchIndex_[i]
      // update commitIndex_
    } else {
        // 这个地方不能直接nextIndex_[i]，否则会出现减少次数过多的情况
        nextIndex_[i] = std::min(nextIndex_[i], params.prevLogIndex);
    }
}
```

