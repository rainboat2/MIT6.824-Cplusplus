#ifndef SHARDGROUP_H
#define SHARDGROUP_H

#include <future>
#include <memory>
#include <mutex>
#include <vector>

#include <raft/StateMachine.h>
#include <raft/raft.h>
#include <rpc/kvraft/KVRaft_types.h>
#include <rpc/kvraft/ShardKVRaft.h>
#include <shardkv/KVArgs.hpp>
#include <shardkv/KVService.h>
#include <shardkv/ShardReply.hpp>

/*
 * manage shards and handle kv requests
 */
class ShardManger : public virtual StateMachineIf {
private:
    struct Shard {
        KVService kv;
        ShardStatus::type status;

        Shard()
            : kv(KVService(-1))
            , status(ShardStatus::STOP)
        {
        }
    };

public:
    ShardManger() = default;

    /*
     * method for StateMachineIf
     */
    void apply(ApplyMsg msg) override;
    void startSnapShot(std::string filePath, std::function<void(LogId, TermId)> callback) override;
    void applySnapShot(std::string filePath) override;

    std::future<ShardReply> getFuture(LogId id);

private:
    void handlePutAppend(PutAppendReply& _return, const PutAppendParams& params);
    void handleGet(GetReply& _return, const GetParams& params);
    ErrorCode::type checkShard(ShardId sid, ErrorCode::type& code);

private:
    std::unordered_map<ShardId, Shard> shards_;
    std::unordered_map<LogId, std::promise<ShardReply>> waits_;
    std::mutex lock_;
};

/*
 * handle and redirect raft requests
 */
class ShardGroup : public virtual ShardKVRaftIf {
public:
    ShardGroup() = default;
    ShardGroup(std::vector<Host>& peers, Host me, std::string persisterDir, GID gid);
    ShardGroup(const ShardGroup& g) = delete;

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
    bool sendArgsToRaft(std::future<ShardReply>& f, const KVArgs& args);

private:
    std::unique_ptr<RaftHandler> raft_;
    std::mutex lock_;
    ShardManger shardManger_;
};

inline void ShardGroup::requestVote(RequestVoteResult& _return, const RequestVoteParams& params)
{
    raft_->requestVote(_return, params);
}

inline void ShardGroup::appendEntries(AppendEntriesResult& _return, const AppendEntriesParams& params)
{
    raft_->appendEntries(_return, params);
}

inline void ShardGroup::getState(RaftState& _return)
{
    raft_->getState(_return);
}

inline void ShardGroup::start(StartResult& _return, const std::string& command)
{
    LOG(ERROR) << "Disable RPC invoke for ShardGroup::start!";
    _return.code = ErrorCode::ERR_REQUEST_FAILD;
}

inline TermId ShardGroup::installSnapshot(const InstallSnapshotParams& params)
{
    return raft_->installSnapshot(params);
}

#endif