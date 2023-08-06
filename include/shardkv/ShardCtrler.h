#ifndef SHARDCTRLER_H
#define SHARDCTRLER_H

#include <future>
#include <mutex>
#include <queue>
#include <vector>
#include <unordered_map>

#include <raft/StateMachine.h>
#include <raft/raft.h>
#include <rpc/kvraft/KVRaft_types.h>
#include <rpc/kvraft/Shardctrler.h>

class CtrlerArgs;

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

class ShardCtrler : ShardctrlerIf, StateMachineIf {
private:
    struct Reply {
        ShardCtrlerOP op;
        bool wrongLeader;
        ErrorCode::type code;
        Config config; // for Query
    };

public:
    ShardCtrler(std::vector<Host> peers, Host me, std::string persisterDir);

    void join(JoinReply& _return, const JoinArgs& jargs) override;

    void leave(LeaveReply& _return, const LeaveArgs& largs) override;

    void move(MoveReply& _return, const MoveArgs& margs) override;

    void query(QueryReply& _return, const QueryArgs& qargs) override;

    void apply(ApplyMsg msg) override;

    void startSnapShot(std::string filePath, std::function<void(LogId, TermId)> callback) override;

    void applySnapShot(std::string filePath) override;

private:
    void handleJoin(const JoinArgs& join, const ApplyMsg& msg);

    void handLeave(const LeaveArgs& leave, const ApplyMsg& msg);

    void handleMove(const MoveArgs& move, const ApplyMsg& msg);

    void handleQuery(const QueryArgs& query, const ApplyMsg& msg);

    Reply sendArgsToRaft(const CtrlerArgs& args);

private:
    std::vector<Config> configs_;
    RaftHandler raft_;
    std::mutex lock_;
    std::unordered_map<LogId, std::promise<Reply>> waits_;
    std::unordered_map<GID, std::vector<Host>> groupHosts_;
    LogId lastApplyIndex_;
    TermId lastApplyTerm_;
};

#endif