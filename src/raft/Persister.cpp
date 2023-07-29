#include <cstdio>
#include <deque>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string.h>
#include <sys/stat.h>

#include <fmt/format.h>
#include <glog/logging.h>

#include <raft/Persister.h>
#include <raft/raft.h>
#include <rpc/kvraft/KVRaft_types.h>
#include <rpc/kvraft/Raft.h>
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

std::ostream& operator<<(std::ostream& ost, const Metadata& md)
{
    ost << md.term << '\n';
    if (md.voteFor == NULL_HOST) {
        ost << "-1" << ' ' << 0 << '\n';
    } else {
        ost << md.voteFor.ip << ' ' << md.voteFor.port << '\n';
    }
    return ost;
}

std::istream& operator>>(std::istream& ist, Metadata& md)
{
    ist >> md.term
        >> md.voteFor.ip >> md.voteFor.port;

    if (md.voteFor.ip == "-1")
        md.voteFor = NULL_HOST;
    return ist;
}

Persister::Persister(std::string dirName_)
    : metaFile_(std::fstream(dirName_ + "/meta.dat", std::ios::in | std::ios::out))
    , md_({ 0, NULL_HOST })
    , logChunkDir_(dirName_ + "/logChunks")
    , lastInBufLogId_(0)
{
    std::string metaf = dirName_ + "/meta.dat";
    if (access(metaf.c_str(), F_OK))
        metaFile_ >> md_;
}

Persister::~Persister() {
    flushLogBuf();
}

void Persister::saveTermAndVote(TermId term, Host& voteFor)
{
    md_.term = term;
    md_.voteFor = voteFor;
    metaFile_.seekg(0);
    metaFile_ << md_;
    metaFile_.flush();
}

void Persister::saveLogs(LogId commitIndex, std::deque<LogEntry>& logs)
{
    LOG(INFO) << fmt::format("Logs need to save: ({}, {}), total logs: ({}, {})",
        lastInBufLogId_, commitIndex, logs.front().index, logs.back().index);
    for (LogId i = lastInBufLogId_ + 1; i <= commitIndex; i++) {
        int pos = i - logs.front().index;
        LogEntry& log = logs[pos];
        if (isLogBufFull(log)) {
            flushLogBuf();
        }
        logBuf_ << log.term << ' ' << log.index << '\n'
                << log.command << '\n';
        lastInBufLogId_ = log.index;
    }
}

void Persister::loadRaftState(TermId& term, Host& votedFor, std::deque<LogEntry>& logs)
{
    term = md_.term;
    votedFor = md_.voteFor;
    int chunksNum = logChunksNum();
    for (int i = 0; i < chunksNum; i++) {
        std::string name = fmt::format("{}/chunk{}.dat", logChunkDir_, chunksNum);
        std::ifstream ifs(name);
        char newLine;
        LogEntry log;
        ifs >> log.term >> log.index >> newLine;
        std::getline(ifs, log.command);
        logs.push_back(std::move(log));
    }

    /*
     *  We avoid dealing with the "empty logs_" situation by adding an invalid log
     *  in which the term and index are both 0.
     */
    if (chunksNum == 0)
        logs.emplace_back();

    if (!checkState(term, votedFor, logs)) {
        term = 0;
        votedFor = NULL_HOST;
        logs.clear();
        logs.emplace_back();
    }
}

void Persister::flushLogBuf()
{
    int chunksNum = logChunksNum();
    std::string name = fmt::format("{}/chunk{}.dat", logChunkDir_, chunksNum);
    std::string tmpName = fmt::format("{}/tmp-chunk{}.dat", logChunkDir_, chunksNum);
    {
        std::ofstream ofs(tmpName, std::ios::out);
        ofs << logBuf_.str();
    }
    rename(tmpName.c_str(), name.c_str());
    LOG(INFO) << fmt::format("Log chunk {} have been written to: {}", chunksNum, name.c_str());

    logBuf_.str(""); // clear buffer
}

bool Persister::checkState(TermId& term, Host& voteFor, std::deque<LogEntry>& logs)
{
    LOG_IF(FATAL, term < 0) << "Invalid term: " << term;
    for (int i = 1; i < logs.size(); i++) {
        auto& prevLog = logs[i - 1];
        auto& curLog = logs[i];
        if (curLog.index != prevLog.index + 1) {
            LOG(FATAL) << "Invalid log sequence: " << logs;
        }
    }
    return true;
}

bool Persister::isLogBufFull(LogEntry& log)
{
    int size = sizeof(TermId) + sizeof(LogId);
    size += 3; // 1 spaces and 2 newline
    size += log.command.size();
    return (size + logBuf_.tellp() >= LOG_CHUNK_BYTES);
}

int Persister::logChunksNum()
{
    DIR* dirp = opendir(logChunkDir_.c_str());
    if (dirp == nullptr) {
        if (errno == ENOENT) {
            mkdir(logChunkDir_.c_str(), S_IRWXU);
            LOG(INFO) << "Open log directory failed: " << strerror(errno) << ", create it!";
        } else {
            LOG(FATAL) << "Open log directory failed: " << strerror(errno);
        }
        return 0;
    }

    dirent* dp;
    int cnt = 0;
    while ((dp = readdir(dirp)) != nullptr) {
        std::string name = dp->d_name;
        if (name == "." || name == "..") {
            continue;
        } else if (name.rfind("tmp", 0) != 0) {
            unlink(dp->d_name);
            LOG(INFO) << "Delete useless file: " << name;
        }
        cnt++;
    }
    closedir(dirp);
    return cnt;
}