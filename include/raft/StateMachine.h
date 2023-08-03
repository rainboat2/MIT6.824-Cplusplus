#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <functional>
#include <string>
#include <vector>

#include <rpc/kvraft/KVRaft_types.h>

struct ApplyMsg {
    std::string command;
    int commandIndex;
    int commandTerm;
};

class StateMachineIf {
public:
    virtual ~StateMachineIf() { }

    virtual void apply(ApplyMsg msg) = 0;

    virtual void startSnapShot(std::string filePath, std::function<void(LogId, TermId)> callback) = 0;

    virtual void installSnapShot(std::string filePath) = 0;
};

#endif