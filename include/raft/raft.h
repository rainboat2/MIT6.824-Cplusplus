#ifndef RAFT_H
#define RAFT_H

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <vector>

#include <raft/Persister.h>
#include <raft/StateMachine.h>
#include <rpc/kvraft/KVRaft_types.h>
#include <rpc/kvraft/Raft.h>
#include <tools/ClientManager.hpp>
#include <tools/ToString.hpp>


class RaftHandler : virtual public RaftIf {
public:
    RaftHandler(std::vector<Host>& peers, Host me, std::string persisterDir, StateMachineIf* stateMachine, GID gid = 0);

    ~RaftHandler();

    void requestVote(RequestVoteResult& _return, const RequestVoteParams& params) override;

    void appendEntries(AppendEntriesResult& _return, const AppendEntriesParams& params) override;

    void getState(RaftState& _return) override;

    void start(StartResult& _return, const std::string& command) override;

    TermId installSnapshot(const InstallSnapshotParams& params) override;

private:
    void switchToFollow();

    void switchToCandidate();

    void switchToLeader();

    void updateCommitIndex(LogId newIndex);

    LogEntry& getLogByLogIndex(LogId logIndex);

    AppendEntriesParams buildAppendEntriesParamsFor(int peerIndex);

    void handleReplicateResultFor(int peerIndex, LogId prevLogIndex, LogId matchedIndex, bool success);

    int gatherLogsFor(int peerIndex, AppendEntriesParams& params);

    std::chrono::microseconds getElectionTimeout();

    LogId lastLogIndex() { return (logs_.empty() ? snapshotIndex_ : logs_.back().index); };

    TermId lastLogTerm() { return (logs_.empty() ? snapshotTerm_ : logs_.back().term); };

    void compactLogs();

    void async_checkLeaderStatus() noexcept;

    void async_startElection() noexcept;

    void async_sendHeartBeats() noexcept;

    void async_replicateLogTo(int peerIndex, Host host) noexcept;

    bool async_sendLogsTo(int peerIndex, Host host, AppendEntriesParams& params, ClientManager<RaftClient>& cm);

    bool async_sendSnapshotTo(int peerIndex, Host host);

    void async_applyMsg() noexcept;

    void async_startSnapShot() noexcept;

private:
    // persisten state on all servers
    TermId currentTerm_;
    Host votedFor_;
    std::deque<LogEntry> logs_;

    // volatile state on all servers
    LogId commitIndex_;
    LogId lastApplied_;

    // volatile state on leaders
    std::vector<LogId> nextIndex_;
    std::vector<LogId> matchIndex_;

    // some auxiliary data not listed in raft paper
    ServerState::type state_;
    /*
     * For clarity, only public methods or methods prefixed
     *  with async_ can acquire raftLock_
     */
    std::mutex raftLock_;
    std::chrono::steady_clock::time_point lastSeenLeader_;
    std::vector<Host> peers_;
    Host me_;
    std::atomic<bool> inElection_;
    std::atomic<bool> inSnapshot_;
    std::condition_variable sendEntries_;
    std::condition_variable applyLogs_;
    std::condition_variable startSnapshot_;
    Persister persister_;

    /*
     * Thrift client is thread-unsafe. Considering efficiency and safety,
     * for each kind of task we arrange a ClientManager.
     */
    ClientManager<RaftClient> cmForHB_; // client manager for heart beats
    ClientManager<RaftClient> cmForRV_; //  client manager for request vote
    ClientManager<RaftClient> cmForAE_; //  client manager for  append entries

    StateMachineIf* stateMachine_;
    LogId snapshotIndex_;
    TermId snapshotTerm_;

    GID gid_; // the group id of raft, designed for multiraft

    std::atomic<bool> isExit_;   // When this object is destroyed, it is marked with the variable isExit
};

#endif
