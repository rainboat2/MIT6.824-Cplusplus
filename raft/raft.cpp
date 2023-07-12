#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <memory>
#include <queue>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include <fmt/format.h>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TServerSocket.h>

#include <raft/ClientManager.h>
#include <raft/raft.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using std::string;
using std::vector;
using time_point = std::chrono::steady_clock::time_point;

class Timer {
public:
    Timer(string start_msg, string end_msg)
        : end_msg_(std::move(end_msg))
    {
        start_ = NOW();
        LOG(INFO) << start_msg;
    }

    ~Timer()
    {
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(NOW() - start_);
        LOG(INFO) << end_msg_ << " Used: " << dur.count() << "ms";
    }

private:
    time_point start_;
    string end_msg_;
};

RaftRPCHandler::RaftRPCHandler(vector<RaftAddr>& peers, RaftAddr me)
    : currentTerm_(0)
    , votedFor_(NULL_ADDR)
    , commitIndex_(0)
    , lastApplied_(0)
    , state_(ServerState::FOLLOWER)
    , lastSeenLeader_(NOW())
    , peers_(peers)
    , me_(std::move(me))
    , inElection_(false)
    , cmForHB_(peers.size())
    , cmForRV_(peers.size())
{
    switchToFollow();
}

void RaftRPCHandler::requestVote(RequestVoteResult& _return, const RequestVoteParams& params)
{
    Timer t("Start requestVote!", "Finished requestVote!");
    std::lock_guard<std::mutex> guard(lock_);

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

    LOG(INFO) << "Vote for " << params.candidateId;
    votedFor_ = params.candidateId;
    _return.voteGranted = true;
    _return.term = currentTerm_;
}

void RaftRPCHandler::appendEntries(AppendEntriesResult& _return, const AppendEntriesParams& params)
{
    Timer t("Start appendEntries!", "Finished appendEntries!");
    std::lock_guard<std::mutex> guard(lock_);
    _return.term = currentTerm_;
    if (params.term < currentTerm_) {
        LOG(INFO) << fmt::format("Out of fashion appendEntries from {}, term: {}, currentTerm: {}",
            to_string(params.leaderId), params.term, currentTerm_);
        _return.success = false;
        return;
    }

    if (params.term == currentTerm_ && state_ != ServerState::FOLLOWER) {
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
    if (params.entries.empty()) {
        LOG(INFO) << fmt::format("Received heart beats from leader {}, term: {}, currentTerm: {}",
            to_string(params.leaderId), params.term, currentTerm_);
    } else {
        LOG(INFO) << "Received appendEntries request : " << params;
    }
    _return.success = true;
}

void RaftRPCHandler::getState(RaftState& _return)
{
    Timer t("Start getState!", "Finished getState!");
    std::lock_guard<std::mutex> guard(lock_);
    _return.currentTerm = currentTerm_;
    _return.votedFor = votedFor_;
    _return.commitIndex = commitIndex_;
    _return.lastApplied = lastApplied_;
    _return.state = state_;
    _return.peers = peers_;
    LOG(INFO) << "Get raft state: " << _return;
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
}

std::chrono::microseconds RaftRPCHandler::getElectionTimeout()
{
    static std::random_device rd;
    static std::uniform_int_distribution<int> randomTime(
        MIN_ELECTION_TIMEOUT.count(),
        MAX_ELECTION_TIMEOUT.count());
    return std::chrono::milliseconds(randomTime(rd));
}

void RaftRPCHandler::async_checkLeaderStatus()
{
    while (true) {
        std::this_thread::sleep_for(MIN_ELECTION_TIMEOUT);
        {
            std::lock_guard<std::mutex> guard(lock_);
            switch (state_) {
            case ServerState::CANDIDAE:
            case ServerState::FOLLOWER: {
                if (NOW() - lastSeenLeader_ > getElectionTimeout()) {
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
    {
        std::lock_guard<std::mutex> guard(lock_);
        LOG(INFO) << fmt::format("Start a new election! currentTerm: {}, nextTerm: {}", currentTerm_, currentTerm_ + 1);
        currentTerm_++;
        votedFor_ = me_;

        /*
         * reset lastSeenLeader_, so the async_checkLeaderStatus thread can start a new election
         * if no leader is selected in this term.
         */
        lastSeenLeader_ = NOW();

        // copy peers_ to a local variable to avoid aquire and release lock_ frequently
        peersForRV = peers_;
    }

    RequestVoteParams params;
    params.term = currentTerm_;
    params.candidateId = me_;
    params.lastLogIndex = -1;
    params.LastLogTerm = -1;

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
        std::lock_guard<std::mutex> guard(lock_);
        if (state_ == ServerState::CANDIDAE)
            switchToLeader();
        else
            LOG(INFO) << fmt::format("Raft is not candidate now!");
    }

    for (int i = 0; i < threads.size(); i++)
        threads[i].join();
    inElection_ = false;
}

void RaftRPCHandler::async_sendHeartBeats()
{
    /*
     * copy peers_ to a local variable, so we don't need to aquire and release lock_ frequently
     * to access peers_
     */
    vector<RaftAddr> peersForHB;
    {
        std::lock_guard<std::mutex> guard(lock_);
        peersForHB = peers_;
    }

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
                    LOG_EVERY_N(INFO, 1) << fmt::format("Send 1 heart beats to {}, used: {}ms",
                        to_string(addr), std::chrono::duration_cast<std::chrono::milliseconds>(NOW() - app_start).count());
                } catch (TException& tx) {
                    LOG_EVERY_N(ERROR, 1) << fmt::format("Send 1 heart beats to {} failed: {}", to_string(addr), tx.what());
                    cmForHB_.setInvalid(i);
                }
            });
        }

        for (int i = 0; i < peers_.size(); i++) {
            threads[i].detach();
        }

        std::this_thread::sleep_for(HEART_BEATS_INTERVAL);
    }
}