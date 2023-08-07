#ifndef KVCLIENT_H
#define KVCLIENT_H

#include <chrono>
#include <vector>

#include <common.h>
#include <rpc/kvraft/KVRaft.h>
#include <tools/ClientManager.hpp>

/*
 * A client to invoke KV rpc operation.
 * Notice: KVClerk is thread-unsafe.
 */
class KVClerk {
public:
    KVClerk(std::vector<Host>& hosts);

    void putAppend(PutAppendReply& _return, const PutAppendParams& params);

    void get(GetReply& _return, const GetParams& params);

private:
    void putAppendTo(int hostId, PutAppendReply& _return, const PutAppendParams& params);

    void getTo(int hostId, GetReply& _return, const GetParams& params);

private:
    std::vector<Host> hosts_;
    ClientManager<KVRaftClient> clients_;
    int leaderId_;
};

#endif