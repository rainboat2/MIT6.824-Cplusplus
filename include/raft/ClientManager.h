#ifndef CLIENT_MANAGER_H
#define CLIENT_MANAGER_H

#include <memory>
#include <unordered_map>

#include <rpc/kvraft/Raft.h>
#include <rpc/kvraft/KVRaft_types.h>

class ClientManager {
public:
    ClientManager() = default;

    ClientManager(int num, std::chrono::milliseconds timeout);

    RaftClient* getClient(int i, Host& addr);

    void setInvalid(int i);

private:
    std::vector<std::unique_ptr<RaftClient>> clients_;
    std::chrono::milliseconds timeout_;
};

#endif
