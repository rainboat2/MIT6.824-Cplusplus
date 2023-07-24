#include <raft/ClientManager.h>
#include <raft/raft.h>

#include <fmt/format.h>
#include <glog/logging.h>
#include <chrono>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

ClientManager::ClientManager(int num, std::chrono::milliseconds timeout)
    : clients_(num)
    , timeout_(timeout)
{
}

RaftClient* ClientManager::getClient(int i, Host& addr)
{
    if (clients_[i] == nullptr) {
        auto sk = new TSocket(addr.ip, addr.port);
        sk->setConnTimeout(timeout_.count());
        sk->setRecvTimeout(timeout_.count());
        sk->setSendTimeout(timeout_.count());
        std::shared_ptr<TTransport> socket(sk);
        std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        clients_[i] = std::make_unique<RaftClient>(protocol);
        transport->open();
    }

    return clients_[i].get();
}

void ClientManager::setInvalid(int i)
{
    clients_[i].reset();
}