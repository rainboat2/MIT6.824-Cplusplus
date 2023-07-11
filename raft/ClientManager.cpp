#include <raft/ClientManager.h>
#include <raft/raft.h>

#include <fmt/format.h>
#include <glog/logging.h>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

RaftRPCClient* ClientManager::getClient(int i, RaftAddr& addr)
{
    if (clientsMap_.find(i) == clientsMap_.end()) {
        auto sk = new TSocket(addr.ip, addr.port);
        sk->setConnTimeout(RPC_TIMEOUT.count());
        sk->setRecvTimeout(RPC_TIMEOUT.count());
        sk->setSendTimeout(RPC_TIMEOUT.count());
        std::shared_ptr<TTransport> socket(sk);
        std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        clientsMap_[i] = std::make_unique<RaftRPCClient>(protocol);
        transport->open();
    }

    return clientsMap_[i].get();
}

void ClientManager::setInvalid(int i)
{
    clientsMap_.erase(i);
}