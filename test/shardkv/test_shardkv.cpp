#include <array>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>
#include <glog/logging.h>
#include <gtest/gtest.h>

#include <raft/RaftConfig.h>
#include <rpc/kvraft/KVRaft_types.h>
#include <shardkv/ShardCtrler.h>
#include <shardkv/ShardCtrlerClerk.h>
#include <thrift/TOutput.h>
#include <tools/ProcessManager.hpp>

using std::array;
using std::string;
using std::unordered_map;
using std::vector;

class ShardCtrlerTest : public testing::Test {
protected:
    void SetUp() override
    {
        ports_ = { 7001, 7002, 7003, 7004, 7005, 7006, 7007, 7008 };
        logDir_ = fmt::format("../../logs/{}", testing::UnitTest::GetInstance()->current_test_info()->name());
        testLogDir_ = fmt::format("{}/test_shardkv", logDir_);
        mkdir(logDir_.c_str(), S_IRWXU);
        mkdir(testLogDir_.c_str(), S_IRWXU);

        FLAGS_log_dir = testLogDir_;
        google::InitGoogleLogging(testLogDir_.c_str());
        apache::thrift::GlobalOutput.setOutputFunction([](const char* msg) {
            LOG(WARNING) << msg;
        });
    }

    void initCtrlers(int num)
    {
        EXPECT_GE(ports_.size(), num);

        hosts_ = vector<Host>(num);
        for (uint i = 0; i < hosts_.size(); i++) {
            hosts_[i].ip = "127.0.0.1";
            hosts_[i].port = ports_[i];
        }

        for (int i = 0; i < num; i++) {
            auto peers = hosts_;
            Host me = hosts_[i];
            peers.erase(peers.begin() + i);
            string dirName = fmt::format("{}/shardkv{}", logDir_, i + 1);
            mkdir(dirName.c_str(), S_IRWXU);

            using apache::thrift::TProcessor;
            ctrs_.emplace_back(peers, me, i + 1, dirName, [peers, me, dirName, this]() -> std::shared_ptr<TProcessor> {
                auto handler = std::make_shared<ShardCtrler>(peers, me, dirName, SHARD_NUM_);
                std::shared_ptr<TProcessor> processor(new ShardCtrlerProcessor(handler));
                return processor;
            });
        }

        for (int i = 0; i < num; i++) {
            ctrs_[i].start();
        }
        std::this_thread::sleep_for(MAX_ELECTION_TIMEOUT);
    }

    unordered_map<GID, vector<Host>> getGroupsInfo()
    {
        unordered_map<GID, vector<Host>> gid2hosts;
        array<string, 3> ips { { "192.168.0.1", "192.168.0.3", "192.168.0.3" } };

        for (GID gid = 0; gid < 3; gid++) {
            Host h;
            for (uint i = 0; i < ips.size(); i++) {
                h.ip = ips[gid];
                h.port = ports_[gid];
                gid2hosts[gid].push_back(std::move(h));
            }
        }
        return gid2hosts;
    }

    void TearDown() override
    {
        google::ShutdownGoogleLogging();
    }

protected:
    vector<int> ports_;
    string logDir_;
    string testLogDir_;
    vector<ProcessManager> ctrs_;
    vector<Host> hosts_;
    int SHARD_NUM_ = 10;
};

TEST_F(ShardCtrlerTest, BasicTest4A)
{
    LOG(INFO) << "Start BasicTest4A";
    const int CTRL_NUM = 3;
    initCtrlers(CTRL_NUM);

    auto gid2hosts = getGroupsInfo();
    ShardctrlerClerk clerk(hosts_);

    {
        QueryReply qrep;
        QueryArgs args;
        args.configNum = LATEST_CONFIG_NUM;
        clerk.query(qrep, args);
        EXPECT_EQ(qrep.code, ErrorCode::SUCCEED);
        Config config = qrep.config;
        EXPECT_EQ(config.gid2shards.size(), 0);
        for (GID gid : config.shard2gid) {
            EXPECT_EQ(gid, -1);
        }
    }

    {
        JoinReply jrep;
        JoinArgs join;
        join.servers[0] = gid2hosts[0];
        clerk.join(jrep, join);
        EXPECT_EQ(jrep.code, ErrorCode::SUCCEED);
    }

    {
        QueryReply qrep;
        QueryArgs args;
        args.configNum = LATEST_CONFIG_NUM;
        clerk.query(qrep, args);
        EXPECT_EQ(qrep.code, ErrorCode::SUCCEED);
        Config config = qrep.config;
        EXPECT_EQ(config.configNum, 1);
        EXPECT_EQ(config.gid2shards.size(), 1);
        for (GID gid : config.shard2gid) {
            EXPECT_EQ(gid, 0);
        }
    }
}