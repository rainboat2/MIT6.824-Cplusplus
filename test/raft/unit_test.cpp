#include <memory>
#include <string>
#include <sys/errno.h>
#include <sys/stat.h>
#include <vector>

#include <fmt/format.h>
#include <gtest/gtest.h>

#include "RaftProcess.hpp"
#include <raft/RaftConfig.h>
#include <raft/raft.h>

class RaftUnitTest : public testing::Test {

    void SetUp() override
    {
        me_.ip = "127.0.0.1";
        me_.port = 7001;
        persisterDir_ = fmt::format("../../logs/{}", testing::UnitTest::GetInstance()->current_test_info()->name());
        FLAGS_log_dir = fmt::format("{}/raft", persisterDir_);
        if (mkdir(persisterDir_.c_str(), S_IRWXU)) {
            LOG(WARNING) << fmt::format("mkdir \"{}\" faild: {}", persisterDir_.c_str(), strerror(errno));
        }
        if (mkdir(FLAGS_log_dir.c_str(), S_IRWXU)) {
            LOG(WARNING) << fmt::format("mkdir \"{}\" faild: {}", FLAGS_log_dir, strerror(errno));
        }
        google::InitGoogleLogging(FLAGS_log_dir.c_str());
    }

    void TearDown() override
    {
        google::ShutdownGoogleLogging();
    }

protected:
    MockStateMachine sm_;
    Host me_;
    std::string persisterDir_;
    std::vector<Host> emptyPeers_;
};

TEST_F(RaftUnitTest, RaftExitTest)
{
    RaftHandler raft_(emptyPeers_, me_, persisterDir_, &sm_);
    std::this_thread::sleep_for(MAX_ELECTION_TIMEOUT);
}

TEST_F(RaftUnitTest, RaftFollowerTest)
{
    RaftHandler raft_(emptyPeers_, me_, persisterDir_, &sm_);

    // Send a heartbeat packet to put raft into follower state
    AppendEntriesResult ret;
    AppendEntriesParams params;
    params.term = 1;
    params.prevLogIndex = 0;
    params.prevLogTerm = 0;
    params.leaderCommit = 0;
    raft_.appendEntries(ret, params);
    ASSERT_TRUE(ret.success);
    ASSERT_EQ(ret.term, 0);

    RaftState rst;
    raft_.getState(rst);
    ASSERT_EQ(rst.state, ServerState::FOLLOWER);
    ASSERT_EQ(rst.currentTerm, 1);

    // send a log
    LogEntry log;
    log.index = 1;
    log.term = 1;
    params.entries.push_back(log);
    params.prevLogIndex = 0;
    params.prevLogTerm = 0;
    raft_.appendEntries(ret, params);
}