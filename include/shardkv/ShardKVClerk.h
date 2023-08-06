#ifndef SHARDKVCLIENT_H
#define SHARDKVCLIENT_H

#include <kvraft/KVClerk.h>
#include <rpc/kvraft/KVRaft.h>
#include <tools/ClientManager.hpp>

class ShardKVClerk {
public:
    void putAppend(PutAppendReply& _return, const PutAppendParams& params);

    void get(GetReply& _return, const GetParams& params);
};

#endif