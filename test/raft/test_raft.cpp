#include <gtest/gtest.h>
#include <vector>

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

using std::vector;

class RaftTest : public testing::Test {
protected:
    void SetUp() override
    {
        ports_ = { 8001, 8002, 8003, 8004, 8005 };
        GlobalOutput.setOutputFunction([](const char* msg){
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
    }

    vector<int> findLeaders()
    {
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
        }
        return st;
    }

protected:
    std::vector<RaftProcess> rafts_;
    ClientManager cm_;
    std::vector<std::shared_ptr<TTransport>> transports;
    std::vector<int> ports_;
};

TEST_F(RaftTest, TestInitialElection2A)
{
    const int RAFT_NUM = 3;
    initRafts(RAFT_NUM);
    std::this_thread::sleep_for(MAX_ELECTION_TIMEOUT);

    RaftAddr addr;
    addr.ip = "127.0.0.1";

    for (int i = 0; i < RAFT_NUM; i++) {
        RaftState st;
        addr.port = ports_[i];
        auto client = cm_.getClient(i, addr);
        client->getState(st);
        EXPECT_EQ(st.peers.size(), RAFT_NUM - 1);
    }

    std::this_thread::sleep_for(MAX_ELECTION_TIMEOUT * 2);
    EXPECT_EQ(findLeaders().size(), 1);
}

TEST_F(RaftTest, TestReElection2A)
{
    const int RAFT_NUM = 3;
    initRafts(RAFT_NUM);
    std::this_thread::sleep_for(MAX_ELECTION_TIMEOUT * 2);
    auto leaders = findLeaders();
    EXPECT_EQ(leaders.size(), 1);
    RaftProcess* leader = &rafts_[leaders[0]];

    leader->killRaft();
    std::this_thread::sleep_for(MAX_ELECTION_TIMEOUT * 2);
    EXPECT_EQ(findLeaders().size(), 1);

    leader->start();
    std::this_thread::sleep_for(HEART_BEATS_INTERVAL * 2);
    leaders = findLeaders();
    EXPECT_EQ(leaders.size(), 1);

    leader = &rafts_[leaders[0]];
    leader->killRaft();
    auto* follower = &rafts_[(leaders[0] + 1) % RAFT_NUM];
    follower->killRaft();
    std::this_thread::sleep_for(MAX_ELECTION_TIMEOUT);
    EXPECT_EQ(findLeaders().size(), 0);

    leader->start();
    follower->start();
    std::this_thread::sleep_for(MAX_ELECTION_TIMEOUT);
    EXPECT_EQ(findLeaders().size(), 1);
}