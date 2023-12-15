#ifndef KVARGS_H
#define KVARGS_H

#include <sstream>

#include <glog/logging.h>

#include <rpc/kvraft/KVRaft_types.h>

enum KVArgsOP {
    PUT = 0,
    GET
};

inline constexpr char* opStr(KVArgsOP op)
{
    switch (op) {
    case KVArgsOP::PUT:
        return "PUT";
    case KVArgsOP::GET:
        return "GET";
    default:
        LOG(FATAL) << "Unexpected op: " << op;
    }
}

class KVArgs {
private:
    KVArgs() = default;

public:
    explicit KVArgs(const PutAppendParams& put)
    {
        op_ = KVArgsOP::PUT;
        put_ = put;
    }

    explicit KVArgs(const GetParams& get)
    {
        op_ = KVArgsOP::GET;
        get_ = get;
    }

    KVArgsOP op()
    {
        return op_;
    }

    void copyTo(PutAppendParams& put)
    {
        LOG_IF(FATAL, op_ != KVArgsOP::PUT) << "Expect PUT, got " << opStr(op_);
        put = put_;
    }

    void copyTo(GetParams& get)
    {
        LOG_IF(FATAL, op_ != KVArgsOP::PUT) << "Expect PUT, got " << opStr(op_);
        get = get_;
    }

    static std::string serialize(const KVArgs& args)
    {
        std::ostringstream ss;

        switch (args.op_) {
        case KVArgsOP::PUT: {
            auto& put = args.put_;

            ss << opStr(args.op_) << ' '
               << put.key << ' ' << put.value << ' '
               << static_cast<int>(put.op) << ' '
               << put.gid << put.sid;
        } break;

        case KVArgsOP::GET: {
            auto& get = args.get_;

            ss << opStr(args.op_)
               << get.key << ' '
               << get.gid << ' ' << get.sid;
        } break;

        default:
            LOG(FATAL) << "Unexpected state" << opStr(args.op_);
        }
        return ss.str();
    }

    static KVArgs deserialize(std::string cmd)
    {
        KVArgs args;
        std::istringstream iss(cmd);
        std::string cmdType;
        iss >> cmdType;
        if (cmdType.rfind(opStr(KVArgsOP::PUT), 0) == 0) {
            PutAppendParams params;
            int putOp;
            iss >> params.key >> params.value
                >> putOp >> params.gid >> params.sid;
            params.op = static_cast<PutOp::type>(putOp);

            args.op_ = KVArgsOP::PUT;
            args.put_ = params;
        } else if (cmdType.rfind(opStr(KVArgsOP::GET), 0) == 0) {
            GetParams params;
            iss >> params.key >> params.gid >> params.sid;

            args.op_ = KVArgsOP::GET;
            args.get_ = params;
        } else {
            LOG(FATAL) << "Unkonw cmd: " << cmd;
        }
        return args;
    }

private:
    KVArgsOP op_;
    PutAppendParams put_;
    GetParams get_;
};

#endif