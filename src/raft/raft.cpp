#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <memory>
#include <queue>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <fmt/format.h>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include <thrift/TToString.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TServerSocket.h>

#include <raft/raft.h>
#include <tools/ClientManager.hpp>
#include <tools/Timer.hpp>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using std::string;
using std::vector;
using time_point = std::chrono::steady_clock::time_point;

RaftHandler::RaftHandler(vector<Host>& peers, Host me, string persisterDir, StateMachineIf* stateMachine)
    : currentTerm_(0)
    , votedFor_(NULL_ADDR)
    , commitIndex_(0)
    , lastApplied_(0)
    , nextIndex_(vector<int32_t>(peers.size()))
    , matchIndex_(vector<int32_t>(peers.size()))
    , state_(ServerState::FOLLOWER)
    , lastSeenLeader_(NOW())
    , peers_(peers)
    , me_(std::move(me))
    , inElection_(false)
    , persister_(persisterDir)
    , cmForHB_(ClientManager<RaftClient>(peers.size(), HEART_BEATS_INTERVAL))
    , cmForRV_(ClientManager<RaftClient>(peers.size(), RPC_TIMEOUT))
    , cmForAE_(ClientManager<RaftClient>(peers.size(), RPC_TIMEOUT))
    , stateMachine_(stateMachine)
{
    switchToFollow();
    /*
     *  We avoid dealing with the "empty logs_" situation by adding an invalid log
     *  in which the term and index are both 0.
     */
    logs_.emplace_back();
    std::thread ae([this]() {
        this->async_sendLogEntries();
    });
    ae.detach();
    
    std::thread applier([this]() {
        this->async_applyMsg();
    });
    applier.detach();

    persister_.loadRaftState(this);
}

void RaftHandler::requestVote(RequestVoteResult& _return, const RequestVoteParams& params)
{
    std::lock_guard<std::mutex> guard(raftLock_);

    if (params.term > currentTerm_) {
        votedFor_ = NULL_ADDR;
        if (state_ != ServerState::FOLLOWER)
            switchToFollow();
        currentTerm_ = params.term;
    }

    if (params.term < currentTerm_) {
        LOG(INFO) << fmt::format("Out of fashion vote request from {}, term: {}, currentTerm: {}",
            to_string(params.candidateId), params.term, currentTerm_);
        _return.term = currentTerm_;
        _return.voteGranted = false;
        return;
    }

    if (params.term == currentTerm_ && votedFor_ != NULL_ADDR && votedFor_ != params.candidateId) {
        LOG(INFO) << fmt::format("Receive a vote request from {}, but already voted to {}, reject it.",
            to_string(params.candidateId), to_string(votedFor_));
        _return.term = currentTerm_;
        _return.voteGranted = false;
        return;
    }

    /*
     * If the logs have last entries with different terms, then the log with the later term is more up-to-date. If the logs
     * end with the same term, then whichever log is longer is more up-to-date.
     */
    auto lastLog = logs_.back();
    if (lastLog.term < params.LastLogTerm || (lastLog.term == params.LastLogTerm && lastLog.index < params.lastLogIndex)) {
        LOG(INFO) << fmt::format("Receive a vote request from {} with outdate logs, reject it.", to_string(params.candidateId));
    }

    LOG(INFO) << "Vote for " << params.candidateId;
    votedFor_ = params.candidateId;
    _return.voteGranted = true;
    _return.term = currentTerm_;
    persister_.saveRaftState(this);
}

void RaftHandler::appendEntries(AppendEntriesResult& _return, const AppendEntriesParams& params)
{
    std::lock_guard<std::mutex> guard(raftLock_);

    _return.success = false;
    _return.term = currentTerm_;

    if (params.term < currentTerm_) {
        LOG(INFO) << fmt::format("Out of fashion appendEntries from {}, term: {}, currentTerm: {}",
            to_string(params.leaderId), params.term, currentTerm_);
        return;
    }

    if (params.term == currentTerm_ && state_ != ServerState::FOLLOWER) {
        LOG_IF(FATAL, state_ == ServerState::LEADER) << "Two leader in the same term!";
        switchToFollow();
    }

    if (params.term > currentTerm_) {
        LOG(INFO) << fmt::format("Received logs from higher term leader {}, term: {}, currentTerm: {}",
            to_string(params.leaderId), params.term, currentTerm_);
        currentTerm_ = params.term;
        votedFor_ = NULL_ADDR;
        if (state_ != ServerState::FOLLOWER) {
            switchToFollow();
        }
    }
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

    if (params.entries.empty()) {
        LOG_EVERY_N(INFO, HEART_BEATS_LOG_COUNT)
            << fmt::format("Received {} heart beats from leader {}, term: {}, currentTerm: {}",
                   HEART_BEATS_LOG_COUNT, to_string(params.leaderId), params.term, currentTerm_);
    } else {
        LOG(INFO) << "Received appendEntries request!";
        persister_.saveRaftState(this);
    }
    _return.success = true;
}

void RaftHandler::getState(RaftState& _return)
{
    std::lock_guard<std::mutex> guard(raftLock_);
    _return.currentTerm = currentTerm_;
    _return.votedFor = votedFor_;
    _return.commitIndex = commitIndex_;
    _return.lastApplied = lastApplied_;
    _return.state = state_;
    _return.peers = peers_;
    for (auto& log : logs_) {
        _return.logs.push_back(log);
    }
    LOG(INFO) << fmt::format("Get raft state: term = {}, votedFor = {}, commitIndex = {}, lastApplied = {}, state={}, logs size={}",
        currentTerm_, to_string(votedFor_), commitIndex_, lastApplied_, to_string(state_), _return.logs.size());
}

void RaftHandler::start(StartResult& _return, const std::string& command)
{
    std::lock_guard<std::mutex> guard(raftLock_);

    if (state_ != ServerState::LEADER) {
        _return.isLeader = false;
        return;
    }

    LogEntry log;
    log.index = logs_.back().index + 1;
    log.command = command;
    log.term = currentTerm_;
    logs_.push_back(std::move(log));

    _return.expectedLogIndex = log.index;
    _return.term = currentTerm_;
    _return.isLeader = true;

    sendEntries_.notify_one();
    LOG(INFO) << "start to synchronization cmd: " << command;
}

void RaftHandler::switchToFollow()
{
    state_ = ServerState::FOLLOWER;
    LOG(INFO) << "Switch to follower!";
    std::thread cl([this]() {
        this->async_checkLeaderStatus();
    });
    cl.detach();
    votedFor_ = NULL_ADDR;
}

void RaftHandler::switchToCandidate()
{
    state_ = ServerState::CANDIDAE;
    LOG(INFO) << "Switch to Candidate!";
    std::thread se([this]() {
        this->async_startElection();
    });
    se.detach();
    votedFor_ = NULL_ADDR;
}

void RaftHandler::switchToLeader()
{
    state_ = ServerState::LEADER;
    LOG(INFO) << "Switch to Leader! Term: " << currentTerm_;
    std::thread hb([this]() {
        this->async_sendHeartBeats();
    });
    hb.detach();
    votedFor_ = NULL_ADDR;
    std::fill(nextIndex_.begin(), nextIndex_.end(), logs_.back().index + 1);
    std::fill(matchIndex_.begin(), matchIndex_.end(), 0);
}

LogEntry& RaftHandler::getLogByLogIndex(LogId logIndex)
{
    int i = logIndex - logs_.front().index;
    LOG_IF(FATAL, i < 0 || i > logs_.size()) << fmt::format("Unexpected log index {}, cur logs size is {}", logIndex, logs_.size());
    auto& entry = logs_[i];
    LOG_IF(FATAL, logIndex != entry.index) << "Unexpected log entry: " << entry << " in index: " << logIndex;
    return entry;
}

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
        LogId agreeIndex = matchI[matchI.size() / 2];
        if (getLogByLogIndex(agreeIndex).term == currentTerm_)
            commitIndex_ = agreeIndex;
    } else {
        nextIndex_[i] = std::min(nextIndex_[i], params.prevLogIndex);
    }
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

std::chrono::microseconds RaftHandler::getElectionTimeout()
{
    static std::random_device rd;
    static std::uniform_int_distribution<int> randomTime(
        MIN_ELECTION_TIMEOUT.count(),
        MAX_ELECTION_TIMEOUT.count());
    auto timeout = std::chrono::milliseconds(randomTime(rd));
    return timeout;
}

void RaftHandler::async_checkLeaderStatus() noexcept
{
    while (true) {
        std::this_thread::sleep_for(getElectionTimeout());
        {
            std::lock_guard<std::mutex> guard(raftLock_);
            switch (state_) {
            case ServerState::CANDIDAE:
            case ServerState::FOLLOWER: {
                if (NOW() - lastSeenLeader_ > MIN_ELECTION_TIMEOUT) {
                    LOG(INFO) << "Election timeout, start a election.";
                    switchToCandidate();
                }
            } break;
            case ServerState::LEADER:
                /*
                 * Leader does not need to check the status of leader,
                 * exit this thread
                 */
                LOG(INFO) << "Raft become a leader, exit the checkLeaderStatus thread!";
                return;
            default:
                LOG(FATAL) << "Unexpected state!";
            }
        }
    }
}

void RaftHandler::async_startElection() noexcept
{
    bool expected = false;
    if (!inElection_.compare_exchange_strong(expected, true)) {
        LOG(INFO) << "Current raft is in the election!";
        return;
    }

    vector<Host> peersForRV;
    RequestVoteParams params;
    {
        std::lock_guard<std::mutex> guard(raftLock_);
        LOG(INFO) << fmt::format("Start a new election! currentTerm: {}, nextTerm: {}", currentTerm_, currentTerm_ + 1);
        currentTerm_++;
        votedFor_ = me_;

        /*
         * reset lastSeenLeader_, so the async_checkLeaderStatus thread can start a new election
         * if no leader is selected in this term.
         */
        lastSeenLeader_ = NOW();

        // copy peers_ to a local variable to avoid aquire and release raftLock_ frequently
        peersForRV = peers_;

        params.term = currentTerm_;
        params.candidateId = me_;
        params.lastLogIndex = logs_.back().index;
        params.LastLogTerm = logs_.back().term;
    }

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
                client->requestVote(rs, params);
            } catch (TException& tx) {
                LOG(ERROR) << fmt::format("Request {} for voting error: {}", to_string(peersForRV[i]), tx.what());
                cmForRV_.setInvalid(i);
            }

            if (rs.voteGranted)
                voteCnt++;
            finishCnt++;
            cv.notify_one();
        });
    }

    int raftNum = peers_.size() + 1;
    std::unique_lock<std::mutex> lk(finish);
    cv.wait(lk, [raftNum, &finishCnt, &voteCnt]() {
        return voteCnt > (raftNum / 2) || (finishCnt == raftNum - 1);
    });

    LOG(INFO) << fmt::format("Raft nums: {}, get votes: {}", raftNum, voteCnt.load());
    if (voteCnt > raftNum / 2) {
        std::lock_guard<std::mutex> guard(raftLock_);
        if (state_ == ServerState::CANDIDAE)
            switchToLeader();
        else
            LOG(INFO) << fmt::format("Raft is not candidate now!");
    }

    for (int i = 0; i < threads.size(); i++)
        threads[i].join();
    inElection_ = false;
}

void RaftHandler::async_sendHeartBeats() noexcept
{
    /*
     * copy peers_ to a local variable, so we don't need to aquire and release raftLock_ frequently
     * to access peers_
     */
    vector<Host> peersForHB;
    {
        std::lock_guard<std::mutex> guard(raftLock_);
        peersForHB = peers_;
    }

    while (true) {
        vector<AppendEntriesParams> paramsList(peersForHB.size());
        {
            std::lock_guard<std::mutex> guard(raftLock_);
            if (state_ != ServerState::LEADER)
                return;
            for (int i = 0; i < peersForHB.size(); i++) {
                paramsList[i] = buildAppendEntriesParamsFor(i);
            }
        }

        auto startNext = NOW() + HEART_BEATS_INTERVAL;
        vector<std::thread> threads(peersForHB.size());
        for (int i = 0; i < peersForHB.size(); i++) {
            threads[i] = std::thread([i, &peersForHB, &paramsList, this]() {
                Host addr;
                try {
                    RaftClient* client;
                    addr = peersForHB[i];
                    client = cmForHB_.getClient(i, addr);

                    AppendEntriesResult rs;
                    client->appendEntries(rs, paramsList[i]);

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
}

void RaftHandler::async_sendLogEntries() noexcept
{
    while (true) {
        vector<Host> peersForAE;
        {
            /*
             * wait for RaftHandler::start function to notify
             */
            std::unique_lock<std::mutex> logLock(raftLock_);
            sendEntries_.wait(logLock);
            LOG(INFO) << "async_sendLogEntries is notified";
            peersForAE = peers_;
            logLock.unlock();
        }

        bool finish = false;
        while (!finish) {
            {
                std::lock_guard<std::mutex> guard(raftLock_);
                if (state_ != ServerState::LEADER)
                    break;
            }
            LOG(INFO) << "Start to send logs to all peers. peers size: " << peersForAE.size();
            vector<std::thread> threads(peersForAE.size());
            for (int i = 0; i < peersForAE.size(); i++) {
                threads[i] = std::thread([i, this, &peersForAE]() {
                    AppendEntriesParams params;
                    int logsNum;
                    {
                        std::lock_guard<std::mutex> guard(raftLock_);
                        params = buildAppendEntriesParamsFor(i);
                        logsNum = gatherLogsFor(i, params);
                    }

                    if (logsNum == 0) {
                        LOG(INFO) << "No logs need to send to: " << peersForAE[i];
                        return;
                    }

                    AppendEntriesResult rs;
                    try {
                        auto* client_ = cmForAE_.getClient(i, peersForAE[i]);
                        client_->appendEntries(rs, params);
                        LOG(INFO) << fmt::format("Send {} logs to {}", logsNum, to_string(peersForAE[i]))
                                  << fmt::format(", params: (prevLogIndex={}, prevLogTerm={}, commit={})",
                                         params.prevLogIndex, params.prevLogTerm, params.leaderCommit)
                                  << ", the result is " << rs;
                        {
                            std::lock_guard<std::mutex> guard(raftLock_);
                            handleAEResultFor(i, params, rs);
                        }
                    } catch (TException& tx) {
                        cmForAE_.setInvalid(i);
                        LOG(INFO) << fmt::format("Send logs to {} failed: {}", to_string(peersForAE[i]), tx.what());
                    }
                });
            }

            for (int i = 0; i < peersForAE.size(); i++) {
                threads[i].join();
            }

            {
                std::lock_guard<std::mutex> guard(raftLock_);
                /*
                 * Exit this loop if we send all logs successfully
                 */
                finish = std::all_of(matchIndex_.begin(), matchIndex_.end(), [this](int index) {
                    return logs_.back().index == index;
                });
            }
        }
    }
}

void RaftHandler::async_applyMsg() noexcept
{
    std::queue<LogEntry> logsForApply;
    while (true) {
        std::this_thread::sleep_for(APPLY_MSG_INTERVAL);

        while (true) {
            {
                std::lock_guard<std::mutex> guard(raftLock_);
                // we only copy 20 logs each time to avoid holding the lock for too long
                for (int i = lastApplied_; i <= std::min(lastApplied_ + 20, commitIndex_); i++) {
                    logsForApply.push(getLogByLogIndex(i));
                }
            }

            if (logsForApply.empty())
                break;

            const int N = logsForApply.size();
            while (!logsForApply.empty()) {
                LogEntry log = std::move(logsForApply.front());
                logsForApply.pop();

                ApplyMsg msg;
                msg.commandValid = true;
                msg.command = log.command;
                msg.commandIndex = log.index;
                stateMachine_->apply(msg);
            }

            {
                std::lock_guard<std::mutex> guard(raftLock_);
                lastApplied_ += N;
            }
        }
    }
}