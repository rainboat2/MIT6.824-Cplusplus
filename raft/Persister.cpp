#include <cstdio>
#include <deque>
#include <fstream>
#include <iostream>
#include <sstream>

#include <fmt/format.h>
#include <glog/logging.h>

#include <raft/Persister.h>
#include <raft/raft.h>
#include <raft/rpc/raft_types.h>
#include <tools/Timer.hpp>

static std::ostream& operator<<(std::ostream& out, std::deque<LogEntry>& logs)
{
    out << '[';
    for (auto& l : logs) {
        out << l << ',';
    }
    out << ']';
    return out;
}

Persister::Persister(std::string dirName_)
    : stateFile_(dirName_ + "/state")
{
}

void Persister::saveRaftState(RaftRPCHandler* rf)
{
    std::lock_guard<std::mutex> guard(lock_);
    Timer t("Start saveRaftState!", "Finish saveRaftState!");
    std::ofstream ofs(stateFile_);
    checkState(rf);
    ofs << rf->currentTerm_ << '\n';

    if (rf->votedFor_ != NULL_ADDR)
        ofs << rf->votedFor_.ip << ' ' << rf->votedFor_.port << '\n';
    else
        ofs << "-1.-1.-1.-1" << ' ' << -1 << '\n';
    ofs << rf->logs_.size() << '\n';

    for (auto log : rf->logs_) {
        ofs << log.command << '\n'
            << log.index << ' ' << log.term << '\n';
    }
    LOG(INFO) << fmt::format("Write {} logs to disk: ", rf->logs_.size()) << rf->logs_;
}

void Persister::loadRaftState(RaftRPCHandler* rf)
{
    std::lock_guard<std::mutex> guard(lock_);
    std::ifstream ifs(stateFile_);
    if (!ifs.good())
        return;

    char newLine;
    ifs >> rf->currentTerm_;
    ifs >> rf->votedFor_.ip >> rf->votedFor_.port;
    if (rf->votedFor_.port < 0)
        rf->votedFor_ = NULL_ADDR;
    ifs.get(newLine);

    int logSize;
    ifs >> logSize;
    LOG(INFO) << fmt::format("Read {} logs from disk.", logSize);
    rf->logs_.clear();
    for (int i = 0; i < logSize; i++) {
        LogEntry log;
        std::getline(ifs, log.command);
        ifs >> log.index >> log.term;
        rf->logs_.push_back(std::move(log));
        ifs.get(newLine);
    }

    /*
     * if state_ file is invalid, remove it and reset the raft state
     */
    if (!checkState(rf)) {
        rf->currentTerm_ = 0;
        rf->votedFor_ = NULL_ADDR;
        rf->logs_.clear();
        rf->logs_.emplace_back();
        ifs.close();
        remove(stateFile_.c_str());
    }
}

bool Persister::checkState(RaftRPCHandler* rf)
{
    LOG_IF(ERROR, rf->currentTerm_ < 0) << "Invalid term: " << rf->currentTerm_;
    for (int i = 1; i < rf->logs_.size(); i++) {
        auto& prevLog = rf->logs_[i - 1];
        auto& curLog = rf->logs_[i];
        if (curLog.index != prevLog.index + 1) {
            LOG(ERROR) << "Invalid log sequence: " << rf->logs_;
        }
    }
    LOG_IF(ERROR, rf->logs_.empty()) << "Get emtpy logs from state file!" << std::endl;
    return true;
}
