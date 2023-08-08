#ifndef PROCESSPMANAGER_HPP
#define PROCESSPMANAGER_HPP

#include <functional>
#include <memory>
#include <signal.h>
#include <sstream>
#include <string>
#include <unistd.h>

#include <fmt/format.h>
#include <glog/logging.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

class ProcessManager {
public:
    ProcessManager(std::vector<Host>& peers, Host me, int id, std::string log_dir,
        std::function<std::shared_ptr<apache::thrift::TProcessor>()> processorCreator)
        : pid_(-1)
        , peers_(peers)
        , me_(me)
        , id_(id)
        , log_dir_(log_dir)
        , processorCreator_(processorCreator)
    {
    }

    ~ProcessManager()
    {
        killRaft();
    }

    void start()
    {
        using namespace apache::thrift;
        using namespace apache::thrift::protocol;
        using namespace apache::thrift::transport;
        using namespace ::apache::thrift::server;

        if (pid_ > 0)
            return;

        pid_ = fork();
        if (pid_ == 0) {
            if (google::IsGoogleLoggingInitialized())
                google::ShutdownGoogleLogging();

            FLAGS_log_dir = log_dir_;
            FLAGS_logbuflevel = -1;
            FLAGS_stderrthreshold = 5;
            google::InitGoogleLogging(log_dir_.c_str());

            std::shared_ptr<TServerTransport> serverTransport(new TServerSocket(me_.port));
            std::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
            std::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

            auto processor = processorCreator_();
            TThreadedServer server(processor, serverTransport, transportFactory, protocolFactory);
            LOG(INFO) << "Start to listen on " << me_ << ", peers size: " << peers_.size();
            server.serve();
        }
    }

    void killRaft()
    {
        if (pid_ > 0) {
            kill(pid_, SIGKILL);
            wait(&pid_);
            pid_ = -id_;
        }
    }

private:
    pid_t pid_;
    std::vector<Host> peers_;
    Host me_;
    int id_;
    std::string log_dir_;
    std::function<std::shared_ptr<apache::thrift::TProcessor>()> processorCreator_;
};

#endif