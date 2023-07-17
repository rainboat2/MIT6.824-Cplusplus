#ifndef CLIENT_MANAGER_H
#define CLIENT_MANAGER_H

#include <memory>
#include <unordered_map>

#include <raft/rpc/RaftRPC.h>
#include <raft/rpc/raft_types.h>

class ClientManager {
public:
    ClientManager() = default;

    ClientManager(int num, std::chrono::milliseconds timeout);

    RaftRPCClient* getClient(int i, RaftAddr& addr);

    void setInvalid(int i);

private:
    std::vector<std::unique_ptr<RaftRPCClient>> clients_;
    std::chrono::milliseconds timeout_;
};

#endif
