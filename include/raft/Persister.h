#ifndef PERSISTER_H
#define PERSISTER_H

#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

#include <rpc/kvraft/KVRaft_types.h>

const unsigned int LOG_CHUNK_BYTES = 4 * 1024 * 1024;

class RaftHandler;

struct Metadata {
    TermId term;
    Host voteFor;
};

class Persister {
public:
    Persister(std::string dirName_);

    ~Persister();

    void saveTermAndVote(TermId term, Host& host);

    void saveLogs(LogId commitIndex, std::deque<LogEntry>& logs);

    void loadRaftState(TermId& term, Host& voteFor, std::deque<LogEntry>& logs);

private:
    void flushLogBuf();

    bool checkState(TermId& term, Host& voteFor, std::deque<LogEntry>& logs);

    bool isLogBufFull(LogEntry& log);

    int loadChunks();

private:
    std::fstream metaFile_;
    Metadata md_;
    std::string logChunkDir_;
    std::ostringstream logBuf_;
    LogId lastInBufLogId_;
    std::vector<std::string> chunkNames_;
};

#endif