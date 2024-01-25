#include <shardkv/ShardKV.h>

ShardKV::ShardKV(std::vector<Host>& ctrlerHosts)
    : ctrlerHosts_(ctrlerHosts)
{
}

void ShardKV::pullShardParams(PullShardReply& reply, const PullShardParams& params)
{
}
