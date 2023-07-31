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

static std::ostream& operator<<(std::ostream& ost, const Metadata& md)
{
    ost << md.term << '\n';
    if (md.voteFor == NULL_HOST) {
        ost << "-1" << ' ' << 0 << '\n';
    } else {
        ost << md.voteFor.ip << ' ' << md.voteFor.port << '\n';
    }
    return ost;
}

static std::istream& operator>>(std::istream& ist, Metadata& md)
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
    , estimateLogBufSize_(0)
    , lastInBufLogId_(0)
    , snapshotDir_(dirName_ + "/snapshots")
{
    std::string metaf = dirName_ + "/meta.dat";
    if (access(metaf.c_str(), F_OK))
        metaFile_ >> md_;
    loadChunks();
}

Persister::~Persister()
{
    flushLogBuf();
}

void Persister::saveTermAndVote(TermId term, Host& voteFor)
{
    Timer t("Start save meta data!", "Finish save meta data");
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
        uint es = estmateSize(log);
        if (estimateLogBufSize_ + es > LOG_CHUNK_BYTES) {
            flushLogBuf();
        }
        logBuf_.push_back(log);
        estimateLogBufSize_ += es;
        lastInBufLogId_ = log.index;
    }
}

void Persister::loadRaftState(TermId& term, Host& votedFor, std::deque<LogEntry>& logs)
{
    term = md_.term;
    votedFor = md_.voteFor;
    for (auto& chunk : chunkNames_) {
        auto path = logChunkDir_ + "/" + chunk;
        std::ifstream ifs(path);
        LogId chunkStart, chunkEnd;
        ifs >> chunkStart >> chunkEnd;
        LOG(INFO) << fmt::format("Start read logs [{}, {}] from chunk file {}", chunkStart, chunkEnd, path);
        char newLine;
        LogEntry log;
        while (ifs >> log.term >> log.index >> newLine) {
            std::getline(ifs, log.command);
            logs.push_back(std::move(log));
        }
    }

    auto ss = loadLatestSnapshot();
    if (ss != "") {
        applySnapshot(snapshotDir_);
    }

    /*
     *  We avoid dealing with the "empty logs_" situation by adding an invalid log
     *  in which the term and index are both 0.
     */
    if (logs.empty())
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
    Timer t("Start flush log buffer!", "Finish flush log buffer!");
    std::string tmpName = fmt::format("{}/tmp-chunk.dat", logChunkDir_);
    {
        std::ofstream ofs(tmpName, std::ios::out | std::ios::trunc);
        ofs << logBuf_.front().index << ' ' << logBuf_.back().index;
        for (auto& log : logBuf_) {
            ofs << log.term << ' ' << log.index << '\n';
            ofs << log.command << '\n';
        }
    }

    std::string name = fmt::format("{}/{}.dat", logChunkDir_, epochInMs());
    rename(tmpName.c_str(), name.c_str());
    LOG(INFO) << fmt::format("Log chunk {} have been written to: {}", chunkNames_.size(), name.c_str());
    chunkNames_.push_back(std::move(name));

    logBuf_.clear();
    estimateLogBufSize_ = 0;
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

uint Persister::estmateSize(LogEntry& log)
{
    uint size = std::to_string(log.term).size() + std::to_string(log.index).size();
    size += 3; // 1 spaces and 2 newline
    size += log.command.size();
    return size;
}

int Persister::loadChunks()
{
    auto files = filesIn(logChunkDir_);
    for (auto file : files) {
        if (file.rfind("tmp", 0) == 0) {
            auto chunkPath = logChunkDir_ + "/" + file;
            LOG(INFO) << "Remove useless file: " << file;
        } else {
            chunkNames_.push_back(file);
        }
    }
    sort(chunkNames_.begin(), chunkNames_.end());
    return chunkNames_.size();
}

std::string Persister::loadLatestSnapshot()
{
    auto snapshots = filesIn(snapshotDir_);
    std::string lastestSnapshot;
    for (auto ss : snapshots) {
        if (ss.rfind("tmp", 0) != std::string::npos) {
            continue;
        }
        if (lastestSnapshot.empty() || lastestSnapshot < ss) {
            lastestSnapshot = ss;
        }
    }
    return lastestSnapshot;
}

void Persister::commitSnapshot(std::string tmpName, TermId lastIncTerm, LogId lastIncIndex)
{
    auto snapshotName = fmt::format("{}/{}.dat", snapshotDir_, epochInMs());
    rename(tmpName.c_str(), snapshotName.c_str());
    compactLogs(lastIncIndex);
}

void Persister::applySnapshot(std::string snapshot)
{
    auto path = snapshotDir_ + "/" + snapshot;
    std::ifstream ifs(path);

    if (!ifs.good()) {
        LOG(INFO) << "Apply snapshot failed! No snapshot file: " << path;
        return;
    }

    LogId lastTerm, lastIndex;
    ifs >> lastTerm >> lastIndex;
    compactLogs(lastIndex);
}

void Persister::compactLogs(LogId lastIncIndex)
{
    while (chunkNames_.empty()) {
        LogId chunkStart, chunkEnd;
        std::string chunk = chunkNames_.front();
        std::ifstream ifs(chunk);
        ifs >> chunkStart >> chunkEnd;

        if (chunkEnd <= lastIncIndex) {
            unlink(chunk.c_str());
            LOG(INFO) << "Remove chunk file: " << chunk;
            chunkNames_.pop_front();
        } else {
            break;
        }
    }

    while (!logBuf_.empty()) {
        if (logBuf_.front().index <= lastIncIndex) {
            LOG(INFO) << "Remove log: " << logBuf_.front().index;
            logBuf_.pop_front();
        } else {
            break;
        }
    }
}

std::vector<std::string> Persister::filesIn(std::string& dir)
{
    std::vector<std::string> files;
    DIR* dirp = opendir(dir.c_str());
    if (dirp == nullptr) {
        if (errno == ENOENT) {
            mkdir(dir.c_str(), S_IRWXU);
            LOG(INFO) << "Open log directory failed: " << strerror(errno) << ", create it!";
        } else {
            LOG(FATAL) << "Open log directory failed: " << strerror(errno);
        }
        return {};
    }

    dirent* dp;
    while ((dp = readdir(dirp)) != nullptr) {
        std::string name = dp->d_name;
        if (name == "." || name == ".." || dp->d_type == DT_REG) {
            continue;
        }
        files.push_back(std::move(name));
    }
    closedir(dirp);
    return files;
}