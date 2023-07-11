#ifndef CLIENT_MANAGER_H
#define CLIENT_MANAGER_H

#include <unordered_map>
#include <memory>

#include <raft/rpc/RaftRPC.h>
#include <raft/rpc/raft_types.h>


class ClientManager {
public:
    ClientManager() = default;

    RaftRPCClient* getClient(int i, RaftAddr& addr);

    void setInvalid(int i);

private:
    std::unordered_map<int, std::unique_ptr<RaftRPCClient>> clientsMap_;
};

#endif
