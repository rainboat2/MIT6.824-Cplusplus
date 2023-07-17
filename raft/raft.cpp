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

#include <raft/ClientManager.h>
#include <raft/raft.h>
#include <tools/Timer.hpp>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using std::string;
using std::vector;
using time_point = std::chrono::steady_clock::time_point;

RaftRPCHandler::RaftRPCHandler(vector<RaftAddr>& peers, RaftAddr me)
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
    , cmForHB_(peers.size(), HEART_BEATS_INTERVAL)
    , cmForRV_(peers.size(), RPC_TIMEOUT)
    , cmForAE_(peers.size(), RPC_TIMEOUT)
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
}

void RaftRPCHandler::requestVote(RequestVoteResult& _return, const RequestVoteParams& params)
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

    if (!logs_.empty()) {
        /*
         * If the logs have last entries with different terms, then the log with the later term is more up-to-date. If the logs
         * end with the same term, then whichever log is longer is more up-to-date.
         */
        auto lastLog = logs_.back();
        if (lastLog.term < params.LastLogTerm || (lastLog.term == params.LastLogTerm && lastLog.index < params.lastLogIndex)) {
            LOG(INFO) << fmt::format("Receive a vote request from {} with outdate logs, reject it.", to_string(params.candidateId));
        }
    }

    LOG(INFO) << "Vote for " << params.candidateId;
    votedFor_ = params.candidateId;
    _return.voteGranted = true;
    _return.term = currentTerm_;
}

void RaftRPCHandler::appendEntries(AppendEntriesResult& _return, const AppendEntriesParams& params)
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

    if (!params.entries.empty() && params.prevLogIndex > logs_.back().index) {
        LOG(INFO) << fmt::format("Expected older logs. prevLogIndex: {}, param.prevLogIndex: {}",
            logs_.back().index, params.prevLogIndex);
        return;
    }

    /*
     * find the latest log entry where the two logs agree
     */
    while (params.prevLogIndex < logs_.back().index) {
        LOG(INFO) << "Delete extra uncommitted entry: " << logs_.back();
        logs_.pop_back();
    }
    if (params.prevLogIndex == logs_.back().index && params.prevLogTerm != logs_.back().term) {
        LOG(INFO) << fmt::format("Expected log term: {}, get {}, delete it!", params.prevLogTerm, logs_.back().term);
        logs_.pop_back();
        return;
    }

    for (auto& entry : params.entries) {
        logs_.push_back(entry);
    }

    commitIndex_ = std::min(params.leaderCommit, logs_.back().index);

    if (params.entries.empty()) {
        LOG_EVERY_N(INFO, 10) << fmt::format("Received 10 heart beats from leader {}, term: {}, currentTerm: {}",
            to_string(params.leaderId), params.term, currentTerm_);
    } else {
        LOG(INFO) << "Received appendEntries request : " << params;
    }
    _return.success = true;
}

void RaftRPCHandler::getState(RaftState& _return)
{
    Timer t("Start getState", "Finished get state");
    std::lock_guard<std::mutex> guard(raftLock_);
    _return.currentTerm = currentTerm_;
    _return.votedFor = votedFor_;
    _return.commitIndex = commitIndex_;
    _return.lastApplied = lastApplied_;
    _return.state = state_;
    _return.peers = peers_;
    LOG(INFO) << "Get State, logs_ size: " << logs_.size();
    for (auto& log : logs_) {
        _return.logs.push_back(log);
    }
    LOG(INFO) << fmt::format("Get raft state: term = {}, votedFor = {}, commitIndex = {}, lastApplied = {}, state={}, logs size={}",
        currentTerm_, to_string(votedFor_), commitIndex_, lastApplied_, to_string(state_), _return.logs.size());
    // LOG(INFO) << "Get raft state" << _return;
}

void RaftRPCHandler::start(StartResult& _return, const std::string& command)
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
}

void RaftRPCHandler::switchToFollow()
{
    state_ = ServerState::FOLLOWER;
    LOG(INFO) << "Switch to follower!";
    std::thread cl([this]() {
        this->async_checkLeaderStatus();
    });
    cl.detach();
    votedFor_ = NULL_ADDR;
}

void RaftRPCHandler::switchToCandidate()
{
    state_ = ServerState::CANDIDAE;
    LOG(INFO) << "Switch to Candidate!";
    std::thread se([this]() {
        this->async_startElection();
    });
    se.detach();
    votedFor_ = NULL_ADDR;
}

void RaftRPCHandler::switchToLeader()
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

LogEntry& RaftRPCHandler::getLogByLogIndex(int logIndex)
{
    int i = logIndex - logs_.front().index;
    LOG(INFO) << fmt::format("Get log {}, which pos in logs_ is {}, logs_ size: {}", logIndex, i, logs_.size());
    return logs_[i];
}

AppendEntriesParams RaftRPCHandler::buildAppendEntriesParams()
{
    AppendEntriesParams params;
    params.term = currentTerm_;
    params.leaderId = me_;
    params.prevLogIndex = logs_.back().index;
    params.prevLogTerm = logs_.back().term;
    params.leaderCommit = commitIndex_;
    return params;
}

void RaftRPCHandler::handleAEResultFor(int peerIndex, AppendEntriesResult& rs, int logsNum)
{
    int i = peerIndex;
    if (rs.success) {
        nextIndex_[i] += logsNum;
        matchIndex_[i] = nextIndex_[i] - 1;
        /*
         * Update commitIndex_. This is a typical top k problem, since the size of matchIndex_ is tiny,
         * to simplify the code, we handle this problem by sorting matchIndex_.
         */
        auto matchI = matchIndex_;
        sort(matchI.begin(), matchI.end());
        int agreeIndex = matchI[matchI.size() / 2];
        if (getLogByLogIndex(agreeIndex).term == currentTerm_)
            commitIndex_ = agreeIndex;
    } else {
        nextIndex_[i]--;
    }
}

int RaftRPCHandler::gatherLogsFor(int peerIndex, AppendEntriesParams& params)
{
    int target = logs_.back().index;
    LOG(INFO) << target << ',' << nextIndex_[peerIndex] << std::endl;
    if (nextIndex_[peerIndex] > target)
        return 0;

    auto& prevLog = getLogByLogIndex(nextIndex_[peerIndex] - 1);
    params.prevLogIndex = prevLog.index;
    params.prevLogTerm = prevLog.term;

    int logsNum = std::min(target - nextIndex_[peerIndex] + 1, MAX_LOGS_PER_REQUEST);
    for (int j = 0; j < logsNum; j++) {
        params.entries.push_back(getLogByLogIndex(j + nextIndex_[peerIndex]));
    }

    return logsNum;
}

std::chrono::microseconds RaftRPCHandler::getElectionTimeout()
{
    static std::random_device rd;
    static std::uniform_int_distribution<int> randomTime(
        MIN_ELECTION_TIMEOUT.count(),
        MAX_ELECTION_TIMEOUT.count());
    auto timeout = std::chrono::milliseconds(randomTime(rd));
    return timeout;
}

void RaftRPCHandler::async_checkLeaderStatus() noexcept
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

void RaftRPCHandler::async_startElection() noexcept
{
    bool expected = false;
    if (!inElection_.compare_exchange_strong(expected, true)) {
        LOG(INFO) << "Current raft is in the election!";
        return;
    }

    vector<RaftAddr> peersForRV;
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
            RaftRPCClient* client;
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

void RaftRPCHandler::async_sendHeartBeats() noexcept
{
    /*
     * copy peers_ to a local variable, so we don't need to aquire and release raftLock_ frequently
     * to access peers_
     */
    vector<RaftAddr> peersForHB;
    {
        std::lock_guard<std::mutex> guard(raftLock_);
        peersForHB = peers_;
    }

    while (true) {
        AppendEntriesParams params;
        {
            std::lock_guard<std::mutex> guard(raftLock_);
            if (state_ != ServerState::LEADER)
                return;
            params = buildAppendEntriesParams();
        }

        auto startNext = NOW() + HEART_BEATS_INTERVAL;
        vector<std::thread> threads(peersForHB.size());
        for (int i = 0; i < peersForHB.size(); i++) {
            threads[i] = std::thread([i, &peersForHB, &params, this]() {
                RaftAddr addr;
                try {
                    RaftRPCClient* client;
                    addr = peersForHB[i];
                    client = cmForHB_.getClient(i, addr);

                    AppendEntriesResult rs;
                    client->appendEntries(rs, params);
                } catch (TException& tx) {
                    LOG(ERROR) << fmt::format("Send 1 heart beats to {} failed: {}", to_string(addr), tx.what());
                    cmForHB_.setInvalid(i);
                }
            });
        }

        for (int i = 0; i < peersForHB.size(); i++) {
            threads[i].join();
        }
        LOG_EVERY_N(INFO, 10) << fmt::format("Broadcast 10 heart beats");

        std::this_thread::sleep_until(startNext);
    }
}

void RaftRPCHandler::async_sendLogEntries() noexcept
{
    while (true) {
        vector<RaftAddr> peersForAE;
        {
            /*
             * wait for RaftRPCHandler::start function to notify
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
                        params = buildAppendEntriesParams();
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
                        LOG(INFO) << fmt::format("Send {} logs to {}, the result is: ", logsNum, to_string(peersForAE[i])) << rs;
                    } catch (TException& tx) {
                        cmForAE_.setInvalid(i);
                        LOG(INFO) << fmt::format("Send logs to {} failed: ", to_string(peersForAE[i]), tx.what());
                        return;
                    }

                    {
                        std::lock_guard<std::mutex> guard(raftLock_);
                        handleAEResultFor(i, rs, logsNum);
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
