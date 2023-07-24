#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <functional>
#include <string>
#include <vector>

struct ApplyMsg {
    bool commandValid;
    std::string command;
    int commandIndex;
    bool snapshotValid;
    std::vector<char> bytes;
    int snapshotTerm;
    int snapshotIndex;
};

class StateMachine {
public:
    virtual void apply(ApplyMsg& msg) = 0;

    virtual void startSnapShot(std::string fileName, std::function<void()> callback) = 0;
};

#endif