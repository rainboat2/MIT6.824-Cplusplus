#ifndef SHARDGROUP_H
#define SHARDGROUP_H

#include <memory>
#include <mutex>
#include <vector>

#include <raft/StateMachine.h>
#include <raft/raft.h>
#include <rpc/kvraft/KVRaft_types.h>
#include <rpc/kvraft/ShardKVRaft.h>
#include <shardkv/KVArgs.hpp>
#include <shardkv/KVService.h>

class ShardGroup : public virtual ShardKVRaftIf,
                   public virtual StateMachineIf {
private:
    struct Shard {
        KVService kv;
        ShardStatus::type status;
    };

    struct Reply {
        ErrorCode::type code;
        ShardStatus::type status;
        std::string value;
    };

public:
    ShardGroup(std::vector<Host>& peers, Host me, std::string persisterDir, StateMachineIf* stateMachine, GID gid);

    /*
     * methods for shardkv
     */
    virtual void pullShardParams(PullShardReply& _return, const PullShardParams& params) override;

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
    TermId installSnapshot(const InstallSnapshotParams& params) override;

private:
    ErrorCode::type checkShard(ShardId sid, ErrorCode::type& code, ShardStatus::type& status);
    void handlePutAppend(PutAppendReply& _return, const PutAppendParams& params);
    void handleGet(GetReply& _return, const GetParams& params);
    Reply sendArgsToRaft(const KVArgs& args);

private:
    RaftHandler raft_;
    std::mutex lock_;
    std::unordered_map<ShardId, Shard> shards_;
    std::unordered_map<LogId, std::promise<Reply>> waits_;
};

inline void ShardGroup::requestVote(RequestVoteResult& _return, const RequestVoteParams& params)
{
    raft_.requestVote(_return, params);
}

inline void ShardGroup::appendEntries(AppendEntriesResult& _return, const AppendEntriesParams& params)
{
    raft_.appendEntries(_return, params);
}

inline void ShardGroup::getState(RaftState& _return)
{
    raft_.getState(_return);
}

inline void ShardGroup::start(StartResult& _return, const std::string& command)
{
    LOG(ERROR) << "ShardGroup::start do not support RPC invoke!";
    _return.code = ErrorCode::ERR_REQUEST_FAILD;
}

inline TermId ShardGroup::installSnapshot(const InstallSnapshotParams& params)
{
    return raft_.installSnapshot(params);
}

#endif