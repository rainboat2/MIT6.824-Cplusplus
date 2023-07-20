#ifndef RAFT_PROCESS_H
#define RAFT_PROCESS_H

#include <signal.h>
#include <sstream>
#include <string>
#include <unistd.h>
#include <memory>

#include <fmt/format.h>
#include <glog/logging.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/server/TThreadedServer.h>

#include <raft/raft.h>

class RaftProcess {
public:
    RaftProcess(std::vector<RaftAddr>& peers, RaftAddr me, int id, std::string log_dir)
        : pid_(-1)
        , peers_(peers)
        , me_(me)
        , id_(id)
        , log_dir_(log_dir)
    {
    }

    ~RaftProcess()
    {
        killRaft();
    }

    void start()
    {
        using namespace apache::thrift;
        using namespace apache::thrift::protocol;
        using namespace apache::thrift::transport;
        using namespace ::apache::thrift::server;

        if (pid_ > 0) return;
        
        pid_ = fork();
        if (pid_ == 0) {
            google::InitGoogleLogging(log_dir_.c_str());
            FLAGS_log_dir = log_dir_;
            FLAGS_logbuflevel = -1;
            FLAGS_stderrthreshold = 5;
            std::shared_ptr<RaftRPCHandler> handler(new RaftRPCHandler(peers_, me_, log_dir_));
            std::shared_ptr<TProcessor> processor(new RaftRPCProcessor(handler));
            std::shared_ptr<TServerTransport> serverTransport(new TServerSocket(me_.port));
            std::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
            std::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

            TThreadedServer server(processor, serverTransport, transportFactory, protocolFactory);
            LOG(INFO) << "Start to listen on " << me_ << ", peers size: " << peers_.size();
            server.serve();
        }
    }

    void killRaft()
    {
        if (pid_ > 0) {
            // fmt::print("kill raft {}, pid: {}!\n", id_, pid_);
            kill(pid_, SIGKILL);
            wait(&pid_);
            pid_ = -id_;
        }
    }

private:
    pid_t pid_;
    std::vector<RaftAddr> peers_;
    RaftAddr me_;
    int id_;
    std::string log_dir_;
};

#endif