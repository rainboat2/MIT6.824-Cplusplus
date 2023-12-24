#ifndef RAFT_CONFIG_H
#define RAFT_CONFIG_H

#include <chrono>
#include <rpc/kvraft/KVRaft_types.h>

constexpr auto NOW = std::chrono::steady_clock::now;
constexpr auto MIN_ELECTION_TIMEOUT = std::chrono::milliseconds(150);
constexpr auto MAX_ELECTION_TIMEOUT = std::chrono::milliseconds(300);
constexpr auto HEART_BEATS_INTERVAL = std::chrono::milliseconds(50);
constexpr auto APPLY_MSG_INTERVAL = std::chrono::milliseconds(10);
constexpr auto RPC_TIMEOUT = std::chrono::milliseconds(250);
constexpr int MAX_LOGS_PER_REQUEST = 20;
constexpr int MAX_LOGS_BEFORE_SNAPSHOT = 100;
constexpr int HEART_BEATS_LOG_COUNT = 10;
const Host NULL_HOST;
const RaftState INVALID_RAFTSTATE;
const TermId INVALID_TERM_ID = -1;

#endif