#ifndef SHARDKV_H
#define SHARDKV_H

#include <unordered_map>

#include <glog/logging.h>

#include <rpc/kvraft/ShardKVRaft.h>
#include <raft/StateMachine.h>
#include <raft/RaftConfig.h>
#include <shardkv/ShardGroup.h>

class ShardKV: ShardKVRaftIf {
public:
    ShardKV();

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
    std::unordered_map<GID, ShardGroup> groups_;
};

inline void ShardKV::putAppend(PutAppendReply& _return, const PutAppendParams& params)
{
    if (groups_.find(params.gid) == groups_.end()) {
        _return.code == ErrorCode::ERR_NO_SUCH_GROUP;
        return;
    }
    groups_[params.gid].putAppend(_return, params);
}

inline void ShardKV::get(GetReply& _return, const GetParams& params)
{
    if (groups_.find(params.gid) == groups_.end()) {
        _return.code == ErrorCode::ERR_NO_SUCH_GROUP;
        return;
    }
    groups_[params.gid].get(_return, params);
}

inline void ShardKV::requestVote(RequestVoteResult& _return, const RequestVoteParams& params)
{
    if (groups_.find(params.gid) == groups_.end()) {
        _return.code == ErrorCode::ERR_NO_SUCH_GROUP;
        return;
    }
    groups_[params.gid].requestVote(_return, params);
}

inline void ShardKV::appendEntries(AppendEntriesResult& _return, const AppendEntriesParams& params)
{
    if (groups_.find(params.gid) == groups_.end()) {
        _return.code == ErrorCode::ERR_NO_SUCH_GROUP;
        return;
    }
    groups_[params.gid].appendEntries(_return, params);
}

inline void ShardKV::getState(RaftState& _return)
{
    LOG(FATAL) << "Unexpected to invoke this function!";
}

void ShardKV::start(StartResult& _return, const std::string& command)
{
    LOG(FATAL) << "Unexpected to invoke this function!";
}

TermId ShardKV::installSnapshot(const InstallSnapshotParams& params)
{
    if (groups_.find(params.gid) == groups_.end()) {
        return INVALID_TERM_ID;
    }
    return groups_[params.gid].installSnapshot(params);
}

#endif