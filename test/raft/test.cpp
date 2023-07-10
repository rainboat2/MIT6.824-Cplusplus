#include "RaftProcess.hpp"
#include <chrono>
#include <iostream>
#include <thread>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

int main()
{
    std::vector<RaftAddr> peers;
    RaftAddr me;
    me.ip = "localhost";
    me.port = 9001;
    RaftProcess rp(peers, me, 1, "/Users/rain/vscodeProjects/MIT6.824/logs/raft1");
    rp.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    auto sk = new TSocket("localhost", 9001);
    sk->setConnTimeout(RPC_TIMEOUT.count());
    sk->setRecvTimeout(RPC_TIMEOUT.count());
    sk->setSendTimeout(RPC_TIMEOUT.count());
    std::shared_ptr<TTransport> socket(sk);
    std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
    std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
    RaftRPCClient client(protocol);
    transport->open();

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    RaftState st;
    client.getState(st);
    std::cout << st.currentTerm << std::endl;

    rp.killRaft();
    return 0;
}