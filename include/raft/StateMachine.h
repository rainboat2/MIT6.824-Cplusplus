#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <functional>
#include <string>
#include <vector>

struct ApplyMsg {
    std::string command;
    int commandIndex;
    int commandTerm;
};

class StateMachineIf {
public:
    virtual ~StateMachineIf() {}

    virtual void apply(ApplyMsg msg) = 0;

    virtual void startSnapShot(std::string fileName, std::function<void(LogId, TermId)> callback) = 0;
};

#endif