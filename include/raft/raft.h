#ifndef RAFT_H
#define RAFT_H

#include <chrono>
#include <mutex>
#include <vector>

#include <raft/ClientManager.h>
#include <raft/rpc/RaftRPC.h>
#include <raft/rpc/raft_types.h>

constexpr auto NOW = std::chrono::steady_clock::now;
constexpr auto RPC_TIMEOUT = std::chrono::milliseconds(500);
constexpr auto MIN_ELECTION_TIMEOUT = std::chrono::milliseconds(300);
constexpr auto MAX_ELECTION_TIMEOUT = std::chrono::milliseconds(600);
constexpr auto HEART_BEATS_INTERVAL = std::chrono::milliseconds(50);
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

private:
    void switchToFollow();

    void switchToCandidate();

    void switchToLeader();

    std::chrono::microseconds getElectionTimeout();

    void async_checkLeaderStatus();

    void async_startElection() noexcept;

    void async_sendHeartBeats();

private:
    // persisten state on all servers
    TermId currentTerm_;
    RaftAddr votedFor_;
    std::vector<LogEntry> log_;

    // volatile state on all servers
    int32_t commitIndex_;
    int32_t lastApplied_;

    // volatile state on leaders
    std::vector<int32_t> nextIndex_;
    std::vector<int32_t> matchIndex_;

    // some auxiliary data not listed in raft paper
    ServerState::type state_;
    std::mutex lock_;
    std::chrono::steady_clock::time_point lastSeenLeader_;
    std::vector<RaftAddr> peers_;
    RaftAddr me_;
    std::atomic<bool> inElection_;

    /*
     * Thrift client is thread-unsafe. Considering efficiency and safety, 
     * for each kind of task we arrange a ClientManager.
     */
    ClientManager cmForHB_;  // client manager for heart beats
    ClientManager cmForRV_;  //  client manager for request vote
};

#endif
