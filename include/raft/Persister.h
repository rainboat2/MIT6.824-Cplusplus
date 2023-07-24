#ifndef PERSISTER_H
#define PERSISTER_H

#include <string>
#include <mutex>
#include <fstream>

class RaftHandler;

class Persister {
public:
    Persister(std::string dirName_);

    void saveRaftState(RaftHandler* rf);

    void loadRaftState(RaftHandler* rf);

private:
    bool checkState(RaftHandler* rf);

private:
    std::string stateFile_;
    std::mutex lock_;
};

#endif