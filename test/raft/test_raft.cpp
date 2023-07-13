#include <array>
#include <gtest/gtest.h>
#include <random>
#include <vector>
#include <sstream>

#include <thrift/TOutput.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include "RaftProcess.hpp"

#include <raft/ClientManager.h>

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

const RaftState INVALID_RAFTSTATE;

using std::array;
using std::vector;

class RaftTest : public testing::Test {
protected:
    void SetUp() override
    {
        ports_ = { 7001, 7002, 7003, 7004, 7005, 7006, 7007, 7008 };
        cm_ = ClientManager(ports_.size());
        GlobalOutput.setOutputFunction([](const char* msg) {
            LOG(INFO) << msg;
        });
    }

    void initRafts(int num)
    {
        EXPECT_GE(ports_.size(), num);
        vector<RaftAddr> addrs(num);
        for (int i = 0; i < num; i++) {
            addrs[i].ip = "127.0.0.1";
            addrs[i].port = ports_[i];
        }

        for (int i = 0; i < num; i++) {
            vector<RaftAddr> peers = addrs;
            RaftAddr me = addrs[i];
            peers.erase(peers.begin() + i);
            rafts_.emplace_back(peers, me, i + 1, fmt::format("../../logs/raft{}", i + 1));
        }

        for (int i = 0; i < num; i++) {
            rafts_[i].start();
        }
        /*
         * Waiting for rafts to start
         */
        std::this_thread::sleep_for(MAX_ELECTION_TIMEOUT);
    }

    vector<int> findLeaders()
    {
        std::this_thread::sleep_for(MAX_ELECTION_TIMEOUT * 2);
        vector<int> leaders;
        for (int i = 0; i < rafts_.size(); i++) {
            auto st = getState(i);

            if (st == INVALID_RAFTSTATE)
                continue;

            if (st.state == ServerState::LEADER) {
                leaders.push_back(i);
            }
        }
        return leaders;
    }

    void TearDown() override
    {
        for (int i = 0; i < transports.size(); i++)
            transports[i]->close();
    }

private:
    RaftState getState(int i)
    {
        RaftState st;
        RaftAddr addr;
        addr.ip = "127.0.0.1";
        addr.port = ports_[i];

        try {
            auto* client = cm_.getClient(i, addr);
            client->getState(st);
        } catch (TException& tx) {
            st = INVALID_RAFTSTATE;
            LOG(WARNING) << fmt::format("Get State of {} failed! {};", to_string(addr), tx.what());
            cm_.setInvalid(i);
        }
        return st;
    }

protected:
    std::vector<RaftProcess> rafts_;
    std::vector<std::shared_ptr<TTransport>> transports;
    std::vector<int> ports_;
    ClientManager cm_;
};

TEST_F(RaftTest, SignleTest)
{
    const int RAFT_NUM = 1;
    initRafts(RAFT_NUM);

    RaftAddr addr;
    addr.ip = "127.0.0.1";
    RaftState st;
    addr.port = ports_[0];

    for (int i = 0; i < 10; i++) {
        auto start = NOW();
        auto client = cm_.getClient(0, addr);
        client->getState(st);
        int dur = std::chrono::duration_cast<std::chrono::milliseconds>(NOW() - start).count();
        EXPECT_LT(dur, 10);
    }
    
    {
        auto start = NOW();
        std::stringstream ss;
        ss << st;
        int dur = std::chrono::duration_cast<std::chrono::milliseconds>(NOW() - start).count();
        EXPECT_LT(dur, 5);
    }
}

TEST_F(RaftTest, TestInitialElection2A)
{
    const int RAFT_NUM = 3;
    initRafts(RAFT_NUM);

    RaftAddr addr;
    addr.ip = "127.0.0.1";

    for (int i = 0; i < RAFT_NUM; i++) {
        RaftState st;
        addr.port = ports_[i];
        auto client = cm_.getClient(i, addr);
        client->getState(st);
        EXPECT_EQ(st.peers.size(), RAFT_NUM - 1);
    }

    EXPECT_EQ(findLeaders().size(), 1);
}

TEST_F(RaftTest, TestReElection2A)
{
    const int RAFT_NUM = 3;
    initRafts(RAFT_NUM);

    auto leaders = findLeaders();
    EXPECT_EQ(leaders.size(), 1);
    RaftProcess* leader = &rafts_[leaders[0]];

    leader->killRaft();
    EXPECT_EQ(findLeaders().size(), 1);

    leader->start();
    leaders = findLeaders();
    EXPECT_EQ(leaders.size(), 1);

    leader = &rafts_[leaders[0]];
    leader->killRaft();
    auto* follower = &rafts_[(leaders[0] + 1) % RAFT_NUM];
    follower->killRaft();

    EXPECT_EQ(findLeaders().size(), 0);

    leader->start();
    follower->start();
    EXPECT_EQ(findLeaders().size(), 1);
}

TEST_F(RaftTest, TestManyElections2A)
{
    const int RAFT_NUM = 7;
    initRafts(RAFT_NUM);

    EXPECT_EQ(findLeaders().size(), 1);

    std::random_device rd;
    std::uniform_int_distribution<int> r(0, 6);

    for (int i = 0; i < 5; i++) {
        array<int, 3> rfs = { r(rd), r(rd), r(rd) };
        for (int j : rfs) {
            rafts_[j].killRaft();
        }

        EXPECT_EQ(findLeaders().size(), 1);

        for (int j : rfs) {
            rafts_[j].start();
        }
        std::this_thread::sleep_for(MAX_ELECTION_TIMEOUT);
    }

    EXPECT_EQ(findLeaders().size(), 1);
}