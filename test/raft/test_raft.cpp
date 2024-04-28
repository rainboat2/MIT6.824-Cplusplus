#include <array>
#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <vector>

#include <fmt/format.h>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <thrift/TOutput.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include "RaftProcess.hpp"

#include <raft/RaftConfig.h>
#include <raft/StateMachine.h>
#include <tools/ClientManager.hpp>

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using std::array;
using std::string;
using std::vector;

class RaftTest : public testing::Test {
protected:
    void SetUp() override
    {
        logDir_ = fmt::format("../../logs/{}", testing::UnitTest::GetInstance()->current_test_info()->name());
        testLogDir_ = fmt::format("{}/test_raft", logDir_);
        if (mkdir(logDir_.c_str(), S_IRWXU) && errno != EEXIST) {
            LOG(WARNING) << fmt::format("mkdir \"{}\" faild: {}", logDir_, strerror(errno));
        }
        if (mkdir(testLogDir_.c_str(), S_IRWXU) && errno != EEXIST) {
            LOG(WARNING) << fmt::format("mkdir \"{}\" faild: {}", testLogDir_, strerror(errno));
        }

        ports_ = { 7001, 7002, 7003, 7004, 7005, 7006, 7007, 7008 };
        cm_ = ClientManager<RaftClient>(ports_.size(), RPC_TIMEOUT);

        FLAGS_log_dir = testLogDir_;
        google::InitGoogleLogging(testLogDir_.c_str());
        apache::thrift::GlobalOutput.setOutputFunction([](const char* msg) {
            LOG(WARNING) << msg;
        });
    }

    void TearDown() override
    {
        google::ShutdownGoogleLogging();
    }

    void initRafts(const uint num)
    {
        EXPECT_GE(ports_.size(), num);
        hosts_ = vector<Host>(num);
        for (uint i = 0; i < hosts_.size(); i++) {
            hosts_[i].ip = "127.0.0.1";
            hosts_[i].port = ports_[i];
        }

        for (uint i = 0; i < num; i++) {
            vector<Host> peers = hosts_;
            Host me = hosts_[i];
            peers.erase(peers.begin() + i);
            string dirName = fmt::format("{}/raft{}", logDir_, i + 1);
            rafts_.emplace_back(peers, me, i + 1, dirName);
            if (mkdir(dirName.c_str(), S_IRWXU) && errno != EEXIST) {
                LOG(WARNING) << fmt::format("mkdir \"{}\" faild: {}", dirName, strerror(errno));
            }
        } 

        for (uint i = 0; i < num; i++) {
            rafts_[i].start();
        }
        /*
         * Waiting for rafts to start
         */
        std::this_thread::sleep_for(MAX_ELECTION_TIMEOUT);
    }

    vector<int> findLeaders()
    {
        int retry = 3;
        vector<int> leaders;
        while (retry-- > 0) {
            leaders.clear();
            for (uint i = 0; i < rafts_.size(); i++) {
                auto st = getState(i);
                if (st == INVALID_RAFTSTATE)
                    continue;

                if (st.state == ServerState::LEADER) {
                    leaders.push_back(i);
                }
            }

            if (leaders.size() != 1) {
                std::this_thread::sleep_for(MAX_ELECTION_TIMEOUT);
            } else {
                return leaders;
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

    StartResult callStartOf(int raftId, string cmd)
    {
        StartResult rs;
        try {
            auto* client = cm_.getClient(raftId, hosts_[raftId]);
            client->start(rs, cmd);
        } catch (TException& tx) {
            cm_.setInvalid(raftId);
            LOG(WARNING) << tx.what();
        }
        return rs;
    }

    string uniqueCmd()
    {
        static std::atomic<int> i(0);
        int id = i.fetch_add(1);
        return "CMD" + std::to_string(id);
    }

    LogEntry getLog(vector<LogEntry>& logs, int logIndex)
    {
        int i = logIndex - logs.front().index;
        return logs[i];
    }

    int nCommitted(int index, string& cmd)
    {
        int nc = 0;
        for (uint i = 0; i < rafts_.size(); i++) {
            auto st = getState(i);
            if (st == INVALID_RAFTSTATE || st.logs.empty() || index > st.logs.back().index)
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

    LogId one(const string& cmd, int expectedServers, bool retry)
    {
        LogId logIndex = -1;

        for (uint i = 0; i < 3; i++) {
            // try all the servers, maybe one is the leader
            for (uint j = 0; j < rafts_.size(); j++) {
                StartResult rs = callStartOf(j, cmd);

                if (rs.isLeader) {
                    logIndex = rs.expectedLogIndex;
                    break;
                }
            }

            // check whether our command is submitted
            for (uint j = 0; j < 10; j++) {
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
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        return -1;
    }

    RaftState getState(int i)
    {
        RaftState st;
        try {
            auto* client = cm_.getClient(i, hosts_[i]);
            client->getState(st);
        } catch (TException& tx) {
            st = INVALID_RAFTSTATE;
            LOG(WARNING) << fmt::format("Get State of {} failed! {};", to_string(hosts_[i]), tx.what());
            cm_.setInvalid(i);
        }
        return st;
    }

protected:
    std::vector<RaftProcess> rafts_;
    std::vector<int> ports_;
    std::vector<Host> hosts_;
    std::string logDir_;
    std::string testLogDir_;
    ClientManager<RaftClient> cm_;
};

TEST_F(RaftTest, SignleTest)
{
    const uint RAFT_NUM = 1;
    initRafts(RAFT_NUM);

    Host addr;
    addr.ip = "127.0.0.1";
    RaftState st;
    addr.port = ports_[0];

    for (uint i = 0; i < 10; i++) {
        auto start = NOW();
        st = getState(0);
        int dur = std::chrono::duration_cast<std::chrono::milliseconds>(NOW() - start).count();
        EXPECT_LT(dur, 10);
    }

    for (uint i = 0; i < 10; i++) {
        auto start = NOW();
        std::stringstream ss;
        ss << st;
        int dur = std::chrono::duration_cast<std::chrono::milliseconds>(NOW() - start).count();
        EXPECT_LT(dur, 10);
    }
}

TEST_F(RaftTest, TestInitialElection2A)
{
    const uint RAFT_NUM = 3;
    initRafts(RAFT_NUM);

    for (uint i = 0; i < RAFT_NUM; i++) {
        RaftState st = getState(i);
        EXPECT_EQ(st.peers.size(), RAFT_NUM - 1);
    }

    EXPECT_EQ(findLeaders().size(), 1);
}

TEST_F(RaftTest, TestReElection2A)
{
    const uint RAFT_NUM = 3;
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
    const uint RAFT_NUM = 7;
    initRafts(RAFT_NUM);

    EXPECT_EQ(findLeaders().size(), 1);

    std::random_device rd;
    std::uniform_int_distribution<int> r(0, 6);

    for (uint i = 0; i < 10; i++) {
        array<int, 3> rfs = { r(rd), r(rd), r(rd) };
        for (int j : rfs) {
            rafts_[j].killRaft();
        }

        EXPECT_EQ(findLeaders().size(), 1);

        for (int j : rfs) {
            rafts_[j].start();
        }
        std::this_thread::sleep_for(MIN_ELECTION_TIMEOUT);
    }

    EXPECT_EQ(findLeaders().size(), 1);
}

TEST_F(RaftTest, TestBasicAgree2B)
{
    const uint RAFT_NUM = 3;
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
    const uint RAFT_NUM = 3;
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
    StartResult rs = callStartOf(leader2, cmd);
    EXPECT_TRUE(rs.isLeader);
    EXPECT_EQ(rs.expectedLogIndex, ++logIndex);

    std::this_thread::sleep_for(MAX_ELECTION_TIMEOUT);

    int n = nCommitted(rs.expectedLogIndex, cmd);
    EXPECT_EQ(n, 0);
}

TEST_F(RaftTest, TestLeaderFailure2B)
{
    const uint RAFT_NUM = 3;
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
    const uint RAFT_NUM = 3;
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

TEST_F(RaftTest, TestFailNoAgree2B)
{
    const uint RAFT_NUM = 5;
    initRafts(RAFT_NUM);

    one(uniqueCmd(), RAFT_NUM, false);

    auto leader = checkOneLeader();
    rafts_[(leader + 1) % RAFT_NUM].killRaft();
    rafts_[(leader + 2) % RAFT_NUM].killRaft();
    rafts_[(leader + 3) % RAFT_NUM].killRaft();

    StartResult rs = callStartOf(leader, uniqueCmd());
    int index = rs.expectedLogIndex;

    string cmd;
    EXPECT_LE(nCommitted(index, cmd), 0);

    rafts_[(leader + 1) % RAFT_NUM].start();
    rafts_[(leader + 2) % RAFT_NUM].start();
    rafts_[(leader + 3) % RAFT_NUM].start();

    int leader2 = checkOneLeader();
    rs = callStartOf(leader2, cmd);
    int index2 = rs.expectedLogIndex;
    EXPECT_TRUE(rs.isLeader);
    EXPECT_GE(index2, 2);
    EXPECT_LE(index2, 3);

    one(uniqueCmd(), RAFT_NUM, true);
}

TEST_F(RaftTest, TestConcurrentStarts2B)
{
    const uint RAFT_NUM = 3;
    initRafts(RAFT_NUM);

    int leader = checkOneLeader();
    vector<std::thread> threads(5);
    for (uint i = 0; i < threads.size(); i++) {
        threads[i] = std::thread([this, leader]() {
            ClientManager<RaftClient> man(hosts_.size(), RPC_TIMEOUT);
            for (int j = 0; j < 20; j++) {
                try {
                    auto* client = man.getClient(leader, hosts_[leader]);
                    StartResult rs;
                    client->start(rs, uniqueCmd());
                } catch (TException& tx) {
                    man.setInvalid(leader);
                    string errmsg = fmt::format("invoke start failed: {}", tx.what());
                    LOG(WARNING) << errmsg;
                }
            }
        });
    };

    for (uint i = 0; i < threads.size(); i++) {
        threads[i].join();
    }

    std::this_thread::sleep_for(MAX_ELECTION_TIMEOUT * 4);

    string cmd;
    int nd = nCommitted(100, cmd);
    EXPECT_EQ(nd, RAFT_NUM);
}

TEST_F(RaftTest, TestBackup2B)
{
    const uint RAFT_NUM = 5;
    initRafts(RAFT_NUM);

    one(uniqueCmd(), RAFT_NUM, true);
    int leader1 = checkOneLeader();

    rafts_[(leader1 + 2) % RAFT_NUM].killRaft();
    rafts_[(leader1 + 3) % RAFT_NUM].killRaft();
    rafts_[(leader1 + 4) % RAFT_NUM].killRaft();

    for (uint i = 0; i < 50; i++) {
        callStartOf(leader1, uniqueCmd());
    }
    std::this_thread::sleep_for(MIN_ELECTION_TIMEOUT / 2);

    rafts_[(leader1 + 2) % RAFT_NUM].killRaft();
    rafts_[(leader1 + 3) % RAFT_NUM].killRaft();
}

TEST_F(RaftTest, TestPersistIndependently2C)
{
    string longPrefix;
    for (uint i = 0; i < 1024 * 128; i++) {
        longPrefix += ('a' + (i - 'a') % 26);
    }

    TermId term, lastIncTerm;
    LogId lastIncIndex;
    Host votedFor;
    std::deque<LogEntry> logs;
    {
        Persister persister(logDir_);
        persister.loadRaftState(term, votedFor, logs, lastIncTerm, lastIncIndex);

        for (int i = 1; i <= 50; i++) {
            LogEntry log;
            log.command = longPrefix + uniqueCmd();
            log.index = (logs.empty() ? 1 : logs.back().index + 1);
            log.term = 1;
            logs.push_back(std::move(log));
        }

        term = 1;
        votedFor.ip = "127.0.0.1";
        votedFor.port = 1234;

        persister.saveTermAndVote(term, votedFor);
        persister.saveLogs(logs.size(), logs);
    }

    {
        TermId pterm;
        Host pVoteFor;
        std::deque<LogEntry> plogs;
        Persister persister(logDir_);
        persister.loadRaftState(pterm, pVoteFor, plogs, lastIncTerm, lastIncIndex);

        EXPECT_EQ(term, pterm);
        EXPECT_EQ(pVoteFor, votedFor);
        EXPECT_EQ(plogs.size(), logs.size());
        for (size_t i = 0; i < plogs.size(); i++) {
            EXPECT_EQ(plogs[i], logs[i]);
        }
    }
}

TEST_F(RaftTest, TestPersist2C)
{
    const uint RAFT_NUM = 3;
    initRafts(RAFT_NUM);
    string longPrefix;
    for (uint i = 0; i < 1024 * 128; i++) {
        longPrefix += ('a' + (i - 'a') % 26);
    }

    for (uint i = 0; i < 50; i++) {
        std::string cmd = longPrefix + uniqueCmd();
        one(cmd, RAFT_NUM, false);
    }
}