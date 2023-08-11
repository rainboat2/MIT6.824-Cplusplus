#include <fmt/format.h>
#include <glog/logging.h>

#include <shardkv/KVService.h>

KVService::KVService(ShardId sid)
    : sid_(sid)
{
}

PutAppendReply KVService::putAppend(const PutAppendParams& params)
{
    LOG_IF(FATAL, params.sid != sid_) << fmt::format("Params should be sent to shard {}, current shardId: {}", params.sid, sid_);
    auto key = params.key;
    auto val = params.value;
    switch (params.op) {
    case PutOp::PUT:
        um_[key] = val;
        break;
    case PutOp::APPEND:
        um_[key] = val;
        break;
    default:
        LOG(FATAL) << "Unexpected op type: " << to_string(params.op);
    }
    
    PutAppendReply rep;
    rep.code = ErrorCode::SUCCEED;
    return rep;
}

GetReply KVService::get(const GetParams& params)
{
    LOG_IF(FATAL, params.sid != sid_) << fmt::format("Params should be sent to shard {}, current shardId: {}", params.sid, sid_);
    GetReply rep;
    if (um_.find(params.key) == um_.end()) {
        rep.code = ErrorCode::ERR_NO_KEY;
        return rep;
    }

    rep.value = um_[params.key];
    rep.code = ErrorCode::SUCCEED;
    return rep;
}