#ifndef SHARDKVCLIENT_H
#define SHARDKVCLIENT_H

#include <rpc/kvraft/KVRaft.h>
#include <tools/ClientManager.hpp>
#include <shardkv/ShardCtrlerClerk.h>

class ShardKVClerk {
public:
    ShardKVClerk(std::vector<Host>& ctrlerHosts);

    void putAppend(PutAppendReply& _return, const PutAppendParams& params);

    void get(GetReply& _return, const GetParams& params);

private:
    ShardctrlerClerk ctrlerClerk_;
};

#endif