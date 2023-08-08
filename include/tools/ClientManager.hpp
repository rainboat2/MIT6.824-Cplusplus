#ifndef CLIENT_MANAGER_H
#define CLIENT_MANAGER_H

#include <memory>
#include <unordered_map>

#include <rpc/kvraft/KVRaft_types.h>

#include <glog/logging.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

template <class ClientType>
class ClientManager {
public:
    ClientManager() = default;

    ClientManager(int num, std::chrono::milliseconds timeout)
        : clients_(num)
        , timeout_(timeout)
    {
    }

    ClientType* getClient(int i, const Host& addr)
    {
        using namespace ::apache::thrift;
        using namespace ::apache::thrift::protocol;
        using namespace ::apache::thrift::transport;

        // LOG_IF(FATAL, i < 0 || i >= clients_.size()) << "Index out of bound! i = "
        //                                              << i << ", client size: " << clients_.size();
        if (clients_[i] == nullptr) {
            auto sk = new TSocket(addr.ip, addr.port);
            sk->setConnTimeout(timeout_.count());
            sk->setRecvTimeout(timeout_.count());
            sk->setSendTimeout(timeout_.count());
            std::shared_ptr<TTransport> socket(sk);
            std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
            std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
            clients_[i] = std::make_unique<ClientType>(protocol);
            transport->open();
        }

        return clients_[i].get();
    }

    void setInvalid(int i)
    {
        clients_[i].reset();
    }

private:
    std::vector<std::unique_ptr<ClientType>> clients_;
    std::chrono::milliseconds timeout_;
};

#endif
