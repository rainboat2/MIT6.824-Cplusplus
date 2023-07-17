#ifndef RAFT_H
#define RAFT_H

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <vector>

#include <raft/ClientManager.h>
#include <raft/rpc/RaftRPC.h>
#include <raft/rpc/raft_types.h>

constexpr auto NOW = std::chrono::steady_clock::now;
constexpr auto MIN_ELECTION_TIMEOUT = std::chrono::milliseconds(150);
constexpr auto MAX_ELECTION_TIMEOUT = std::chrono::milliseconds(300);
constexpr auto HEART_BEATS_INTERVAL = std::chrono::milliseconds(50);
constexpr auto RPC_TIMEOUT = std::chrono::milliseconds(250);
constexpr int MAX_LOGS_PER_REQUEST = 20;
const RaftAddr NULL_ADDR;

inline std::string to_string(const RaftAddr& addr)
{
    return '(' + addr.ip + ',' + std::to_string(addr.port) + ')';
}


class RaftRPCHandler : virtual public RaftRPCIf {
public:
    RaftRPCHandler(std::vector<RaftAddr>& peers, RaftAddr me);

    void requestVote(RequestVoteResult& _return, const RequestVoteParams& params) override;

    void appendEntries(AppendEntriesResult& _return, const AppendEntriesParams& params) override;

    void getState(RaftState& _return) override;

    void start(StartResult& _return, const std::string& command) override;

private:
    void switchToFollow();

    void switchToCandidate();

    void switchToLeader();

    LogEntry& getLogByLogIndex(int logIndex);

    AppendEntriesParams buildAppendEntriesParams();

    void handleAEResultFor(int peerIndex, AppendEntriesResult& rs, int logsNum);
    
    int gatherLogsFor(int peerIndex, AppendEntriesParams& params);

    std::chrono::microseconds getElectionTimeout();

    void async_checkLeaderStatus() noexcept;

    void async_startElection() noexcept;

    void async_sendHeartBeats() noexcept;

    void async_sendLogEntries() noexcept;

private:
    // persisten state on all servers
    TermId currentTerm_;
    RaftAddr votedFor_;
    std::deque<LogEntry> logs_;

    // volatile state on all servers
    int32_t commitIndex_;
    int32_t lastApplied_;

    // volatile state on leaders
    std::vector<int32_t> nextIndex_;
    std::vector<int32_t> matchIndex_;

    // some auxiliary data not listed in raft paper
    ServerState::type state_;
    /*
     * For clarity, only public methods or methods prefixed
     *  with async_ can acquire raftLock_
     */
    std::mutex raftLock_;
    std::chrono::steady_clock::time_point lastSeenLeader_;
    std::vector<RaftAddr> peers_;
    RaftAddr me_;
    std::atomic<bool> inElection_;
    std::condition_variable sendEntries_;

    /*
     * Thrift client is thread-unsafe. Considering efficiency and safety,
     * for each kind of task we arrange a ClientManager.
     */
    ClientManager cmForHB_; // client manager for heart beats
    ClientManager cmForRV_; //  client manager for request vote
    ClientManager cmForAE_; //  client manager for  append entries
};

#endif
