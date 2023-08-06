#ifndef SHARDKV_H
#define SHARDKV_H

#include <vector>

#include <rpc/kvraft/ShardKVRaft.h>
#include <raft/StateMachine.h>
#include <kvraft/KVRaft.h>

class ShardKV: ShardKVRaftIf {
public:

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
    std::vector<KVRaft> parts_;
};

#endif