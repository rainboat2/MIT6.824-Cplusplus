#include <raft/raft.h>

#include <fmt/format.h>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using std::string;
using std::vector;
using time_point = std::chrono::steady_clock::time_point;


DEFINE_string(peers, "", "address of other raft servers, fomat: ip:port,ip:port...");
DEFINE_string(self_addr, "", "self address, fomat ip:port");

static vector<RaftAddr> getPeerAddress(RaftAddr& me);

int main(int argc, char** argv)
{
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    LOG(INFO) << "log_dir: " << FLAGS_log_dir;

    RaftAddr me;
    vector<RaftAddr> peers = getPeerAddress(me);

    int port = me.port;
    std::shared_ptr<RaftRPCHandler> handler(new RaftRPCHandler(peers, me));
    std::shared_ptr<TProcessor> processor(new RaftRPCProcessor(handler));
    std::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
    std::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    std::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

    TThreadedServer server(processor, serverTransport, transportFactory, protocolFactory);
    LOG(INFO) << "start server!" << std::endl;
    server.serve();
    return 0;
}


static vector<RaftAddr> getPeerAddress(RaftAddr& me)
{
    auto extractAddr = [](string& host) {
        LOG(INFO) << host;
        int k = 0;
        while (k < host.size() && host[k] != ':')
            k++;
        if (k == host.size())
            LOG(FATAL) << "Bad ip address fomat: " << host << " rigth fomat: ip:port";
        RaftAddr addr;
        addr.ip = host.substr(0, k);
        addr.port = std::stoi(host.substr(k + 1));
        return addr;
    };

    vector<RaftAddr> rs;
    string peers = FLAGS_peers;
    int i = 0, j = 0;
    while (i < peers.size()) {
        while (j < peers.size() && peers[j] != ',') {
            j++;
        }
        string host = peers.substr(i, j - i);
        rs.push_back(extractAddr(host));
        i = j;
    }

    me = extractAddr(FLAGS_self_addr);
    return rs;
}