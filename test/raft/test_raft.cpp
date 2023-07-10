#include <gtest/gtest.h>
#include <vector>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include "RaftProcess.hpp"

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using std::vector;

class RaftTest : public testing::Test {
protected:
    void SetUp() override
    {
        ports_ = { 8001, 8002, 8003, 8004, 8005 };
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

        std::this_thread::sleep_for(std::chrono::microseconds(1000));

        for (int i = 0; i < num; i++) {
            std::shared_ptr<TTransport> socket(new TSocket("localhost", ports_[i]));
            std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
            std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
            clients_.push_back(RaftRPCClient(protocol));
            transports.push_back(transport);
            transport->open();
        }
    }

    bool cheackOneLeader() {
        int leader_cnt = 0;
        for (int i = 0; i < clients_.size(); i++) {
            RaftState st;
            clients_[i].getState(st);
            if (st.state == ServerState::LEADER) {
                leader_cnt++;
            }
        }
        return leader_cnt == 1;
    }

    void TearDown() override
    {
        for (int i = 0; i < transports.size(); i++)
            transports[i]->close();
    }

protected:
    std::vector<RaftProcess> rafts_;
    std::vector<RaftRPCClient> clients_;
    std::vector<std::shared_ptr<TTransport>> transports;
    std::vector<int> ports_;
};


TEST_F(RaftTest, TestInitialElection2A) {
    const int RAFT_NUM = 3;
    initRafts(RAFT_NUM);

    for (int i = 0; i < RAFT_NUM; i++) {
        RaftState st;
        clients_[i].getState(st);
        EXPECT_EQ(st.peers.size(), RAFT_NUM - 1);
    }

    std::this_thread::sleep_for(MAX_ELECTION_TIMEOUT * 2);
    EXPECT_TRUE(cheackOneLeader());
}