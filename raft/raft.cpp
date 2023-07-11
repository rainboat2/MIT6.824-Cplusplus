#include <atomic>
#include <chrono>
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
{
    switchToFollow();
}

void RaftRPCHandler::requestVote(RequestVoteResult& _return, const RequestVoteParams& params)
{
    std::lock_guard<std::mutex> guard(lock_);
    _return.term = currentTerm_;
    if (params.term <= currentTerm_) {
        LOG(INFO) << fmt::format("Out of fashion vote request, term: {}, currentTerm: {}", params.term, currentTerm_);
        _return.voteGranted = false;
        return;
    }

    if (votedFor_ != NULL_ADDR && votedFor_ != params.candidateId) {
        LOG(INFO) << fmt::format("Receive a vote request, but already voted to ({}, {}), reject it.", votedFor_.ip, votedFor_.port);
        _return.voteGranted = false;
        return;
    }

    // if (params.lastLogIndex < commitIndex_) {
    //     LOG(INFO) << "Receive a vote request from candidate with older log, reject it.";
    //     _return.voteGranted = false;
    // }

    LOG(INFO) << "Vote to " << params.candidateId;
    votedFor_ = params.candidateId;
    _return.voteGranted = true;
}

void RaftRPCHandler::appendEntries(AppendEntriesResult& _return, const AppendEntriesParams& params)
{
    std::lock_guard<std::mutex> guard(lock_);
    _return.term = currentTerm_;
    if (params.term < currentTerm_) {
        LOG(INFO) << fmt::format("Out of fashion appendEntries, term: {}, currentTerm: {}", params.term, currentTerm_);
        _return.success = false;
        return;
    }

    if (params.term > currentTerm_ && state_ != ServerState::FOLLOWER) {
        LOG(INFO) << fmt::format("Received logs from higher term leader, switch to follower!");
        switchToFollow();
    }

    lastSeenLeader_ = NOW();
    
    LOG(INFO) << "Receive appendEntries request : " << params;
    _return.success = true;
}

void RaftRPCHandler::getState(RaftState& _return)
{
    std::lock_guard<std::mutex> guard(lock_);
    _return.currentTerm = currentTerm_;
    _return.votedFor = votedFor_;
    _return.commitIndex = commitIndex_;
    _return.lastApplied = lastApplied_;
    _return.state = state_;
    _return.peers = peers_;
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
        return;
    }

    LOG(INFO) << "Start a new election term!";
    bool needRequestVote = true;
    {
        std::lock_guard<std::mutex> guard(lock_);
        currentTerm_++;
        votedFor_ = me_;

        /*
         * reset lastSeenLeader_, so the async_checkLeaderStatus thread can start a new election
         * if no leader is selected in this term.
         */
        lastSeenLeader_ = NOW();
    }

    if (needRequestVote) {
        RequestVoteParams params;
        params.term = currentTerm_;
        params.candidateId = me_;
        params.lastLogIndex = -1;
        params.LastLogTerm = -1;
        std::atomic<int> voteCnt(1);

        vector<std::thread> threads(peers_.size());
        for (int i = 0; i < peers_.size(); i++) {
            threads[i] = std::thread([i, this, &params, &voteCnt]() {
                RequestVoteResult rs;
                RaftAddr addr;
                RaftRPCClient* client;
                try {
                    {
                        std::lock_guard<std::mutex> guard(lock_);
                        addr = this->peers_[i];
                        client = this->cm_.getClient(i, addr);
                    }
                    client->requestVote(rs, params);
                } catch (TException& tx) {
                    LOG(ERROR) << fmt::format("Request ({},{}) for voting error: {}", addr.ip, addr.port, tx.what());
                    cm_.setInvalid(i);
                }

                if (rs.voteGranted) {
                    voteCnt++;
                } else {
                    std::lock_guard<std::mutex> guard(this->lock_);
                    if (this->currentTerm_ < rs.term)
                        this->currentTerm_ = rs.term;
                }
            });
        }

        for (int i = 0; i < threads.size(); i++)
            threads[i].join();

        int raftNum = peers_.size() + 1;
        LOG(INFO) << fmt::format("Raft nums: {}, get votes: {}", raftNum, voteCnt.load());
        if (voteCnt > raftNum / 2) {
            std::lock_guard<std::mutex> guard(lock_);
            switchToLeader();
        }
    }
    inElection_ = false;
}

void RaftRPCHandler::async_sendHeartBeats()
{
    while (true) {
        {
            std::lock_guard<std::mutex> guard(lock_);
            if (state_ != ServerState::LEADER)
                return;
        }

        AppendEntriesParams params;
        params.term = currentTerm_;
        params.leaderId = me_;

        vector<std::thread> threads(peers_.size());
        for (int i = 0; i < peers_.size(); i++) {
            threads[i] = std::thread([i, this, &params]() {
                try {
                    RaftAddr addr;
                    RaftRPCClient* client;
                    {
                        std::lock_guard<std::mutex> guard(lock_);
                        addr = this->peers_[i];
                        client = cm_.getClient(i, addr);
                    }

                    AppendEntriesResult rs;
                    client->appendEntries(rs, params);
                    LOG(INFO) << fmt::format("Send heart beats to ({}, {})", addr.ip, addr.port);
                } catch (TException& tx) {
                    LOG(ERROR) << fmt::format("Send heart beats failed: {}", tx.what());
                }
            });
        }

        for (int i = 0; i < peers_.size(); i++) {
            threads[i].join();
        }

        std::this_thread::sleep_for(HEART_BEATS_INTERVAL);
    }
}