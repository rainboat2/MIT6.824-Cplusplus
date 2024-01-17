#include <algorithm>
#include <array>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
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
using std::unordered_set;
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
            string dirName = fmt::format("{}/ShardCtrler{}", logDir_, i + 1);
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

    Config getConfig(int configNum, ShardctrlerClerk& clerk)
    {
        QueryReply qrep;
        QueryArgs args;
        args.configNum = LATEST_CONFIG_NUM;
        clerk.query(qrep, args);
        EXPECT_EQ(qrep.code, ErrorCode::SUCCEED);
        Config config = qrep.config;
        return config;
    }

    void check(unordered_map<GID, vector<Host>>& groups, ShardctrlerClerk& clerk)
    {
        auto config = getConfig(LATEST_CONFIG_NUM, clerk);
        EXPECT_EQ(groups.size(), config.groupHosts.size());

        for (auto it : groups) {
            GID gid = it.first;
            auto& hosts = it.second;
            auto& chosts = config.groupHosts[gid];
            EXPECT_EQ(hosts.size(), chosts.size());
            for (int i = 0; i < hosts.size(); i++) {
                EXPECT_EQ(hosts[i].ip, chosts[i].ip);
                EXPECT_EQ(hosts[i].port, chosts[i].port);
            }
        }

        for (GID gid : config.shard2gid) {
            EXPECT_NE(gid, INVALID_GID);
        }

        int maxS = -1, minS = INT_MAX;
        for (auto it : config.gid2shards) {
            auto& shards = it.second;
            int shardSize = static_cast<int>(shards.size());
            maxS = std::max(maxS, shardSize);
            minS = std::min(minS, shardSize);
        }
        EXPECT_LE(maxS - minS, 1);
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
        Config config = getConfig(LATEST_CONFIG_NUM, clerk);
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
        Config config = getConfig(LATEST_CONFIG_NUM, clerk);
        EXPECT_EQ(config.configNum, 1);
        EXPECT_EQ(config.gid2shards.size(), 1);
        for (GID gid : config.shard2gid) {
            EXPECT_EQ(gid, 0);
        }
    }

    {
        JoinReply jrep;
        JoinArgs join;
        join.servers[1] = gid2hosts[1];
        clerk.join(jrep, join);
        EXPECT_EQ(jrep.code, ErrorCode::SUCCEED);
    }

    {
        Config config = getConfig(LATEST_CONFIG_NUM, clerk);
        EXPECT_EQ(config.configNum, 2);
        EXPECT_EQ(config.gid2shards.size(), 2);

        for (GID gid : config.shard2gid) {
            EXPECT_TRUE(gid == 1 || gid == 0);
        }
    }

    {
        LeaveReply lrep;
        LeaveArgs leave;
        leave.gids = { 1 };
        clerk.leave(lrep, leave);
        EXPECT_EQ(lrep.code, ErrorCode::SUCCEED);
    }

    {
        Config config = getConfig(LATEST_CONFIG_NUM, clerk);
        EXPECT_EQ(config.configNum, 3);
        EXPECT_EQ(config.gid2shards.size(), 1);

        for (GID gid : config.shard2gid) {
            EXPECT_EQ(gid, 0);
        }
    }

    {
        LeaveReply lrep;
        LeaveArgs leave;
        leave.gids = { 0 };
        clerk.leave(lrep, leave);
        EXPECT_EQ(lrep.code, ErrorCode::SUCCEED);
    }

    {
        Config config = getConfig(LATEST_CONFIG_NUM, clerk);
        EXPECT_EQ(config.configNum, 4);
        EXPECT_EQ(config.gid2shards.size(), 0);

        for (GID gid : config.shard2gid) {
            EXPECT_EQ(gid, INVALID_GID);
        }
    }
}

TEST_F(ShardCtrlerTest, TestMulti4A)
{
    const int CTRL_NUM = 3, CLERK_NUM = 10;
    initCtrlers(CTRL_NUM);

    auto createHost = [](string ip, int port) -> Host {
        Host host;
        host.ip = std::move(ip);
        host.port = port;
        return host;
    };

    unordered_map<GID, vector<Host>> groups;
    vector<std::thread> threads(10);
    for (int i = 0; i < CLERK_NUM; i++) {
        groups[i] = {createHost(fmt::format("ip{}", i), 8000 + i)};

        threads[i] = std::thread([this, i, createHost]() {
            auto clerk = ShardctrlerClerk(hosts_);
            JoinArgs jargs;
            JoinReply jrep;
            jargs.servers = {
                { i, vector<Host> { createHost(fmt::format("ip{}", i), 8000 + i) } },
                { i + 1000, vector<Host> { createHost(fmt::format("ip{}", i + 1000), 8000 + i) } },
                { i + 2000, vector<Host> { createHost(fmt::format("ip{}", i + 2000), 8000 + i) } }
            };
            clerk.join(jrep, jargs);
            EXPECT_EQ(jrep.code, ErrorCode::SUCCEED);
            
            LeaveReply lrep;
            LeaveArgs largs;
            largs.gids = {i + 1000, i + 2000};
            clerk.leave(lrep, largs);
        });
    }

    for (int i = 0; i < CLERK_NUM; i++) {
        threads[i].join();
    }
    ShardctrlerClerk clerk(hosts_);
    check(groups, clerk);
}