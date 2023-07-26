#ifndef KVCLIENT_H
#define KVCLIENT_H

#include <chrono>
#include <vector>

#include <rpc/kvraft/KVRaft.h>
#include <tools/ClientManager.hpp>

constexpr auto KV_PRC_TIMEOUT = std::chrono::milliseconds(1000);

/*
 * A client to invoke KV rpc operation.
 * KVClient is thread-unsafe.
 */
class KVClient {
public:
    KVClient(std::vector<Host>& hosts);

    void putAppend(PutAppendReply& _return, const PutAppendParams& params);

    void get(GetReply& _return, const GetParams& params);

private:
    void putAppendTo(int hostId, PutAppendReply& _return, const PutAppendParams& params);

    void getTo(int hostId, PutAppendReply& _return, const PutAppendParams& params);

private:
    ClientManager<KVRaftClient> clients_;
    std::vector<Host> hosts_;
    int leaderId_;
};

#endif