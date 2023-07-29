#ifndef KVSERVER_HH
#define KVSERVER_HH

#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include <raft/StateMachine.h>
#include <raft/raft.h>
#include <rpc/kvraft/KVRaft.h>

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
    void startSnapShot(std::string fileName, std::function<void(LogId, TermId)> callback) override;

private:
    void putAppend_internal(PutAppendReply& _return, const PutAppendParams& params);
    void get_internal(GetReply& _return, const GetParams& params);

private:
    std::shared_ptr<RaftHandler> raft_;
    std::unordered_map<std::string, std::string> um_;
    std::unordered_map<LogId, std::promise<PutAppendReply>> putWait_;
    std::unordered_map<LogId, std::promise<GetReply>> getWait_;
    std::mutex lock_;
    std::function<void()> stopListenPort_;
    LogId lastApplyIndex_;
    TermId lastApplyTerm_;
};

inline void KVServer::requestVote(RequestVoteResult& _return, const RequestVoteParams& params)
{
    raft_->requestVote(_return, params);
}
inline void KVServer::appendEntries(AppendEntriesResult& _return, const AppendEntriesParams& params)
{
    raft_->appendEntries(_return, params);
}
inline void KVServer::getState(RaftState& _return)
{
    raft_->getState(_return);
}
inline void KVServer::start(StartResult& _return, const std::string& command)
{
    raft_->start(_return, command);
}

#endif