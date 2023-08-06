#include <shardkv/ShardKV.h>

void ShardKV::putAppend(PutAppendReply& _return, const PutAppendParams& params)
{
}

void ShardKV::get(GetReply& _return, const GetParams& params)
{
}

void ShardKV::requestVote(RequestVoteResult& _return, const RequestVoteParams& params)
{
}

void ShardKV::appendEntries(AppendEntriesResult& _return, const AppendEntriesParams& params)
{
}
void ShardKV::getState(RaftState& _return)
{
}
void ShardKV::start(StartResult& _return, const std::string& command)
{
}

TermId ShardKV::installSnapshot(const InstallSnapshotParams& params)
{
    return 0;
}
