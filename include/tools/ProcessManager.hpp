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
    ProcessManager(Host me, std::string log_dir,
        std::function<std::shared_ptr<apache::thrift::TProcessor>()> processorCreator)
        : pid_(-1)
        , me_(me)
        , log_dir_(log_dir)
        , processorCreator_(processorCreator)
    {
    }

    ~ProcessManager()
    {
        killProcess();
    }

    void start()
    {
        using namespace apache::thrift;
        using namespace apache::thrift::protocol;
        using namespace apache::thrift::transport;
        using namespace ::apache::thrift::server;

        // avoid repeated calls
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
            server.serve();
        }
    }

    void killProcess()
    {
        if (pid_ > 0) {
            kill(pid_, SIGKILL);
            wait(&pid_);
            pid_ = -1;
        }
    }

private:
    pid_t pid_;
    Host me_;
    std::string log_dir_;
    std::function<std::shared_ptr<apache::thrift::TProcessor>()> processorCreator_;
};

#endif