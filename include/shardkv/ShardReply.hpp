#ifndef SHARDREPLY_H
#define SHARDREPLY_H

#include <rpc/kvraft/KVRaft_types.h>
#include <rpc/kvraft/ShardKVRaft.h>

enum class ShardReplyOP {
    INVALID = 0,
    PUT,
    GET,
};

class ShardReply {
public:
    ShardReply()
        : op_(ShardReplyOP::INVALID)
    {
    }

    explicit ShardReply(const GetReply& get)
        : op_(ShardReplyOP::GET)
        , code_(get.code)
        , value_(get.value)
    {
    }

    explicit ShardReply(const PutAppendReply& put)
        : op_(ShardReplyOP::PUT)
        , code_(put.code)
    {
    }

    void copyTo(GetReply& get)
    {
        LOG_IF(FATAL, op_ != ShardReplyOP::GET) << "Expect GET, got " << static_cast<int>(op_);
        get.code = code_;
        get.value = value_;
    }

    void copyTo(PutAppendReply& put)
    {
        LOG_IF(FATAL, op_ != ShardReplyOP::PUT) << "Expect PUT, got " << static_cast<int>(op_);
        put.code = code_;
    }

private:
    ShardReplyOP op_;
    ErrorCode::type code_;
    std::string value_;
};

#endif