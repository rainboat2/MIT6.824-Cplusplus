#ifndef PERSISTER_H
#define PERSISTER_H

#include <string>
#include <mutex>
#include <fstream>

class RaftRPCHandler;

class Persister {
public:
    Persister(std::string dirName_);

    void saveRaftState(RaftRPCHandler* rf);

    void loadRaftState(RaftRPCHandler* rf);

private:
    bool checkState(RaftRPCHandler* rf);

private:
    std::string stateFile_;
    std::mutex lock_;
};

#endif