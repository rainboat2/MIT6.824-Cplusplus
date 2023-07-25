#include <cstdlib>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <thrift/TOutput.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include <kvraft/KVServer.h>
#include <rpc/kvraft/KVRaft.h>
#include <tools/ClientManager.hpp>

#include "KVRaftProcess.hpp"

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using std::string;
using std::vector;

static void outputErrmsg(const char* msg)
{
    static std::ofstream ofs("../../logs/test_kvraft/errmsg.txt", std::ios::app);
    static std::mutex m;
    std::lock_guard<std::mutex> guard(m);
    ofs << msg << std::endl;
}

class KVRaftTest : public testing::Test {
protected:
    void SetUp() override
    {
        logDir_ = fmt::format("../../logs/{}", testing::UnitTest::GetInstance()->current_test_info()->name());
        mkdir(logDir_.c_str(), S_IRWXU);
        ports_ = { 7001, 7002, 7003, 7004, 7005, 7006, 7007, 7008 };
        cm_ = ClientManager<KVRaftClient>(ports_.size(), RPC_TIMEOUT);
        GlobalOutput.setOutputFunction(outputErrmsg);
    }

    void TearDown() override
    {
    }

    void initKVRafts(int num)
    {
        EXPECT_GE(ports_.size(), num);
        addrs_ = vector<Host>(num);
        for (int i = 0; i < num; i++) {
            addrs_[i].ip = "127.0.0.1";
            addrs_[i].port = ports_[i];
        }

        for (int i = 0; i < num; i++) {
            vector<Host> peers = addrs_;
            Host me = addrs_[i];
            peers.erase(peers.begin() + i);
            string dirName = fmt::format("{}/kvraft{}", logDir_, i + 1);
            kvrafts_.emplace_back(peers, me, i + 1, dirName);
            mkdir(dirName.c_str(), S_IRWXU);
        }

        for (int i = 0; i < num; i++) {
            kvrafts_[i].start();
        }
        /*
         * Waiting for kvrafts to start
         */
        std::this_thread::sleep_for(MAX_ELECTION_TIMEOUT);
    }

protected:
    std::vector<KVRaftProcess> kvrafts_;
    std::vector<int> ports_;
    std::vector<Host> addrs_;
    std::string logDir_;
    ClientManager<KVRaftClient> cm_;
};