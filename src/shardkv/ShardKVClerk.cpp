#include <shardkv/ShardKVClerk.h>

ShardKVClerk::ShardKVClerk(std::vector<Host>& ctrlerHosts)
    : ctrlerClerk_(ctrlerHosts)
{
}

void ShardKVClerk::putAppend(PutAppendReply& _return, const PutAppendParams& params)
{
}

void ShardKVClerk::get(GetReply& _return, const GetParams& params)
{
}
