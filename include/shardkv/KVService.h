#ifndef KVSERVICE_H
#define KVSERVICE_H

#include <string>
#include <unordered_map>

#include <rpc/kvraft/KVRaft_types.h>

class KVService {
public:
    explicit KVService(ShardId sid_);

    PutAppendReply putAppend(const PutAppendParams& params);

    GetReply get(const GetParams& params);

private:
    ShardId sid_;
    std::unordered_map<std::string, std::string> um_;
};

#endif