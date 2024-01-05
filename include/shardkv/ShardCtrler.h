#ifndef SHARDCTRLER_H
#define SHARDCTRLER_H

#include <future>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <vector>

#include <raft/StateMachine.h>
#include <raft/raft.h>
#include <rpc/kvraft/KVRaft_types.h>
#include <rpc/kvraft/ShardCtrler.h>

class CtrlerArgs;

constexpr int LATEST_CONFIG_NUM = -1;
constexpr int INVALID_GID = -1;

enum class ShardCtrlerOP {
    JOIN,
    LEAVE,
    MOVE,
    QUERY
};

inline constexpr const char* op2str(ShardCtrlerOP op) noexcept
{
    switch (op) {
    case ShardCtrlerOP::JOIN:
        return "JOIN";
    case ShardCtrlerOP::LEAVE:
        return "LEAVE";
    case ShardCtrlerOP::MOVE:
        return "MOVE";
    case ShardCtrlerOP::QUERY:
        return "QUERY";
    default:
        return "UNKONW";
    }
}

class ShardCtrler : virtual public ShardCtrlerIf,
                    virtual public StateMachineIf {
private:
    struct Reply {
        ShardCtrlerOP op;
        bool wrongLeader;
        ErrorCode::type code;
        Config config; // for Query
    };

public:
    ShardCtrler(std::vector<Host> peers, Host me, std::string persisterDir, int shards);

    ~ShardCtrler() = default;

    /*
     * method for shardctrler
     */
    void join(JoinReply& _return, const JoinArgs& jargs) override;
    void leave(LeaveReply& _return, const LeaveArgs& largs) override;
    void move(MoveReply& _return, const MoveArgs& margs) override;
    void query(QueryReply& _return, const QueryArgs& qargs) override;

    /*
     * methods for RaftIf
     */
    void requestVote(RequestVoteResult& _return, const RequestVoteParams& params) override;
    void appendEntries(AppendEntriesResult& _return, const AppendEntriesParams& params) override;
    void getState(RaftState& _return) override;
    void start(StartResult& _return, const std::string& command) override;
    TermId installSnapshot(const InstallSnapshotParams& params) override;

    /*
     * methods for state machine
     */
    void apply(ApplyMsg msg) override;
    void startSnapShot(std::string filePath, std::function<void(LogId, TermId)> callback) override;
    void applySnapShot(std::string filePath) override;

private:
    Reply handleJoin(const JoinArgs& join, const ApplyMsg& msg);

    Reply handleLeave(const LeaveArgs& leave, const ApplyMsg& msg);

    Reply handleMove(const MoveArgs& move, const ApplyMsg& msg);

    Reply handleQuery(const QueryArgs& query, const ApplyMsg& msg);

    Reply sendArgsToRaft(const CtrlerArgs& args);

    Reply defaultReply(ShardCtrlerOP op);

private:
    std::vector<Config> configs_;
    std::unique_ptr<RaftHandler> raft_;
    std::mutex lock_;
    std::unordered_map<LogId, std::promise<Reply>> waits_;
    std::unordered_map<GID, std::vector<Host>> groupHosts_;
    LogId lastApplyIndex_;
    TermId lastApplyTerm_;
};

inline void ShardCtrler::requestVote(RequestVoteResult& _return, const RequestVoteParams& params)
{
    raft_->requestVote(_return, params);
}
inline void ShardCtrler::appendEntries(AppendEntriesResult& _return, const AppendEntriesParams& params)
{
    raft_->appendEntries(_return, params);
}
inline void ShardCtrler::getState(RaftState& _return)
{
    raft_->getState(_return);
}
inline void ShardCtrler::start(StartResult& _return, const std::string& command)
{
    raft_->start(_return, command);
}
inline TermId ShardCtrler::installSnapshot(const InstallSnapshotParams& params)
{
    return raft_->installSnapshot(params);
}

inline ShardCtrler::Reply ShardCtrler::defaultReply(ShardCtrlerOP op) {
    Reply rep;
    rep.code = ErrorCode::SUCCEED;
    rep.op = op; 
    rep.wrongLeader = false;
    return rep;
}

#endif