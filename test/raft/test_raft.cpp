#include <array>
#include <fstream>
#include <gtest/gtest.h>
#include <random>
#include <sstream>
#include <string>
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

using std::array;
using std::string;
using std::vector;

static void outputErrmsg(const char* msg)
{
    static std::ofstream ofs("../../logs/test_raft/errmsg.txt", std::ios::app);
    static std::mutex m;
    std::lock_guard<std::mutex> guard(m);
    ofs << msg << std::endl;
}

class RaftTest : public testing::Test {
protected:
    void SetUp() override
    {
        ports_ = { 7001, 7002, 7003, 7004, 7005, 7006, 7007, 7008 };
        cm_ = ClientManager(ports_.size(), RPC_TIMEOUT);
        GlobalOutput.setOutputFunction(outputErrmsg);
    }

    void initRafts(int num)
    {
        EXPECT_GE(ports_.size(), num);
        addrs_ = vector<RaftAddr>(num);
        for (int i = 0; i < num; i++) {
            addrs_[i].ip = "127.0.0.1";
            addrs_[i].port = ports_[i];
        }

        for (int i = 0; i < num; i++) {
            vector<RaftAddr> peers = addrs_;
            RaftAddr me = addrs_[i];
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

    int checkOneLeader()
    {
        auto leaders = findLeaders();
        EXPECT_EQ(leaders.size(), 1);
        return leaders.front();
    }

    string uniqueCmd()
    {
        static int i = 0;
        i++;
        return "CMD" + std::to_string(i);
    }

    LogEntry getLog(vector<LogEntry>& logs, int logIndex)
    {
        int i = logIndex - logs.front().index;
        return logs[i];
    }

    int nCommitted(int index, string& cmd)
    {
        int nc = 0;
        for (int i = 0; i < rafts_.size(); i++) {
            auto st = getState(i);
            if (st == INVALID_RAFTSTATE || index > st.logs.back().index)
                continue;

            auto ilog = getLog(st.logs, index);
            if (nc > 0) {
                EXPECT_EQ(cmd, ilog.command);
            } else {
                cmd = ilog.command;
            }

            if (st.commitIndex >= index)
                nc++;
        }
        return nc;
    }

    int one(string&& cmd, int expectedServers, bool retry)
    {
        string cmd1 = cmd;
        return one(cmd1, expectedServers, retry);
    }

    int one(const string& cmd, int expectedServers, bool retry)
    {
        int logIndex = -1;

        for (int i = 0; i < 3; i++) {
            // try all the servers, maybe one is the leader
            for (int j = 0; j < rafts_.size(); j++) {
                StartResult rs;
                try {
                    auto client = cm_.getClient(j, addrs_[j]);
                    client->start(rs, cmd);
                } catch (TException& tx) {
                    cm_.setInvalid(j);
                    outputErrmsg(tx.what());
                }

                if (rs.isLeader) {
                    logIndex = rs.expectedLogIndex;
                    break;
                }
            }

            // check whether our command is submitted
            for (int j = 0; j < 10; j++) {
                if (logIndex != -1) {
                    string cmd1;
                    int nd = nCommitted(logIndex, cmd1);
                    if (nd >= expectedServers && cmd1 == cmd) {
                        return logIndex;
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(HEART_BEATS_INTERVAL));
            }

            if (retry == false) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
        return -1;
    }

    void TearDown() override
    {
    }

private:
    RaftState getState(int i)
    {
        RaftState st;
        try {
            auto* client = cm_.getClient(i, addrs_[i]);
            client->getState(st);
        } catch (TException& tx) {
            st = INVALID_RAFTSTATE;
            auto err = fmt::format("Get State of {} failed! {};", to_string(addrs_[i]), tx.what());
            outputErrmsg(err.c_str());
            cm_.setInvalid(i);
        }
        return st;
    }

protected:
    std::vector<RaftProcess> rafts_;
    std::vector<int> ports_;
    std::vector<RaftAddr> addrs_;
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

TEST_F(RaftTest, TestBasicAgree2B)
{
    const int RAFT_NUM = 3;
    initRafts(RAFT_NUM);

    for (int i = 1; i <= 3; i++) {
        string cmd = uniqueCmd();
        int nd = nCommitted(i, cmd);
        EXPECT_EQ(nd, 0);

        int xindex = one(cmd, RAFT_NUM, false);
        EXPECT_EQ(xindex, i);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
}

TEST_F(RaftTest, TestFollowerFailure2B)
{
    const int RAFT_NUM = 3;
    initRafts(RAFT_NUM);
    int logIndex = 0;

    string cmd = uniqueCmd();
    int xindex = one(cmd, RAFT_NUM, false);
    EXPECT_EQ(xindex, ++logIndex);

    auto leaders = findLeaders();
    EXPECT_EQ(leaders.size(), 1);
    int leader1 = leaders.front();
    rafts_[(leader1 + 1) % RAFT_NUM].killRaft();

    cmd = uniqueCmd();
    xindex = one(cmd, RAFT_NUM - 1, false);
    EXPECT_EQ(xindex, ++logIndex);

    std::this_thread::sleep_for(MAX_ELECTION_TIMEOUT);

    cmd = uniqueCmd();
    xindex = one(cmd, RAFT_NUM - 1, false);
    EXPECT_EQ(xindex, ++logIndex);

    leaders = findLeaders();
    EXPECT_EQ(leaders.size(), 1);
    int leader2 = leaders.front();
    rafts_[(leader2 + 1) % RAFT_NUM].killRaft();
    rafts_[(leader2 + 2) % RAFT_NUM].killRaft();

    cmd = uniqueCmd();
    StartResult rs;
    try {
        auto* client = cm_.getClient(leader2, addrs_[leader2]);
        client->start(rs, cmd);
    } catch (TException& tx) {
        LOG(INFO) << tx.what();
        cm_.setInvalid(leader2);
    }
    EXPECT_TRUE(rs.isLeader);
    EXPECT_EQ(rs.expectedLogIndex, ++logIndex);

    std::this_thread::sleep_for(MAX_ELECTION_TIMEOUT);

    int n = nCommitted(rs.expectedLogIndex, cmd);
    EXPECT_EQ(n, 0);
}

TEST_F(RaftTest, TestLeaderFailure2B)
{
    const int RAFT_NUM = 3;
    initRafts(RAFT_NUM);

    string cmd = uniqueCmd();
    int xindex = one(cmd, RAFT_NUM, false);
    int logIndex = 0;

    EXPECT_EQ(xindex, ++logIndex);

    auto leaders = findLeaders();
    EXPECT_EQ(leaders.size(), 1);
    auto leader = leaders.front();

    rafts_[leader].killRaft();
    std::this_thread::sleep_for(MAX_ELECTION_TIMEOUT * 2);

    cmd = uniqueCmd();
    xindex = one(cmd, RAFT_NUM - 1, false);
    EXPECT_EQ(xindex, ++logIndex);

    std::this_thread::sleep_for(MAX_ELECTION_TIMEOUT);

    cmd = uniqueCmd();
    xindex = one(cmd, RAFT_NUM - 1, false);
    EXPECT_EQ(xindex, ++logIndex);
}

/*
 * test that a follower participates after
 * disconnect and re-connect.
 */
TEST_F(RaftTest, TestFailAgree2B)
{
    const int RAFT_NUM = 3;
    initRafts(RAFT_NUM);

    one(uniqueCmd(), RAFT_NUM, false);
    int leader = checkOneLeader();
    rafts_[(leader + 1) % RAFT_NUM].killRaft();

    one(uniqueCmd(), RAFT_NUM - 1, false);
    one(uniqueCmd(), RAFT_NUM - 1, false);
    std::this_thread::sleep_for(MAX_ELECTION_TIMEOUT);
    one(uniqueCmd(), RAFT_NUM - 1, false);
    one(uniqueCmd(), RAFT_NUM - 1, false);

    rafts_[(leader + 1) % RAFT_NUM].start();

    one(uniqueCmd(), RAFT_NUM - 1, false);
    std::this_thread::sleep_for(MAX_ELECTION_TIMEOUT);
    one(uniqueCmd(), RAFT_NUM - 1, false);
}