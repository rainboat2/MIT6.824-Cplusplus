#include <kvraft/KVServer.h>
#include <rpc/kvraft/KVRaft.h>
#include <tools/RaftFlags.hpp>

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

int main(int argc, char** argv)
{
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    LOG(INFO) << "log_dir: " << FLAGS_log_dir;

    Host me;
    vector<Host> peers = getPeerAddress(me);

    int port = me.port;
    std::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
    std::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    std::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

    auto stopProcessRequest_ = [serverTransport](){
        serverTransport->close();
    };
    std::shared_ptr<KVServer> handler(new KVServer(peers, me, FLAGS_log_dir));
    std::shared_ptr<TProcessor> processor(new KVRaftProcessor(handler));

    TThreadedServer server(processor, serverTransport, transportFactory, protocolFactory);
    LOG(INFO) << "start server!" << std::endl;
    server.serve();

    return 0;
}