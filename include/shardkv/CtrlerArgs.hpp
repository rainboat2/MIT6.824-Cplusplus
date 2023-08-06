#ifndef CTRLERARGS_H
#define CTRLERARGS_H

#include <sstream>

#include <fmt/format.h>
#include <glog/logging.h>

#include <shardkv/ShardCtrler.h>

class CtrlerArgs {
private:
    CtrlerArgs() = default;

public:
    explicit CtrlerArgs(const JoinArgs& args)
    {
        op_ = ShardCtrlerOP::JOIN;
        join_ = args;
    }

    explicit CtrlerArgs(const LeaveArgs& args)
    {
        op_ = ShardCtrlerOP::LEAVE;
        leave_ = args;
    }

    explicit CtrlerArgs(const MoveArgs& args)
    {
        op_ = ShardCtrlerOP::MOVE;
        move_ = args;
    }

    explicit CtrlerArgs(const QueryArgs& args)
    {
        op_ = ShardCtrlerOP::QUERY;
        query_ = args;
    }

    ShardCtrlerOP op()
    {
        return op_;
    }

    void copyTo(JoinArgs& args)
    {
        LOG_IF(FATAL, op_ != ShardCtrlerOP::JOIN) << "Expected JOIN, got " << op2str(op_);
        args = join_;
    }

    void copyTo(LeaveArgs& args)
    {
        LOG_IF(FATAL, op_ != ShardCtrlerOP::MOVE) << "Expected JOIN, got " << op2str(op_);
        args = leave_;
    }

    void copyTo(MoveArgs& args)
    {

        LOG_IF(FATAL, op_ != ShardCtrlerOP::MOVE) << "Expected JOIN, got " << op2str(op_);
        args = move_;
    }

    void copyTo(QueryArgs& args)
    {
        LOG_IF(FATAL, op_ != ShardCtrlerOP::QUERY) << "Expected JOIN, got " << op2str(op_);
        args = query_;
    }

    static std::string serialize(const CtrlerArgs args)
    {
        std::ostringstream ss;

        switch (args.op_) {
        case ShardCtrlerOP::JOIN: {
            ss << op2str(args.op_) << ' ';
            for (auto it : args.join_.servers) {
                auto& hosts = it.second;
                ss << it.first << ' ' << hosts.size() << ' ';
                for (Host& host : hosts) {
                    ss << host.ip << ' ' << host.port << ' ';
                }
            }
        } break;

        case ShardCtrlerOP::LEAVE: {
            ss << op2str(args.op_) << ' ';
            for (GID gid : args.leave_.gids) {
                ss << gid << ' ';
            }
        } break;

        case ShardCtrlerOP::MOVE: {
            ss << op2str(args.op_) << ' ';
            ss << args.move_.shard << ' ' << args.move_.gid;
        } break;

        case ShardCtrlerOP::QUERY: {
            ss << op2str(args.op_) << ' ';
            ss << args.query_.configNum;
        } break;

        default:
            LOG(FATAL) << "Unkown operator";
        }
        return ss.str();
    }

    static CtrlerArgs deserialize(std::string& cmd)
    {
        CtrlerArgs args;
        std::istringstream iss(cmd);
        std::string cmdType;
        iss >> cmdType;
        // use rfind to simulate "startswith"
        if (cmdType.rfind(op2str(ShardCtrlerOP::JOIN), 0) == 0) {
            JoinArgs join;
            GID gid;
            int hostNum;
            iss >> gid >> hostNum;
            std::vector<Host> hosts(hostNum);
            for (int i = 0; i < hostNum; i++) {
                iss >> hosts[i].ip >> hosts[i].port;
            }
            join.servers[gid] = std::move(hosts);
            args = CtrlerArgs(join);
        } else if (cmdType.rfind(op2str(ShardCtrlerOP::LEAVE), 0) == 0) {
            LeaveArgs leave;
            GID gid;
            while (iss >> gid) {
                leave.gids.push_back(gid);
            }
            args = CtrlerArgs(leave);
        } else if (cmdType.rfind(op2str(ShardCtrlerOP::MOVE), 0) == 0) {
            MoveArgs move;
            iss >> move.shard >> move.gid;
            args = CtrlerArgs(move);
        } else if (cmdType.rfind(op2str(ShardCtrlerOP::QUERY), 0) == 0) {
            QueryArgs query;
            iss >> query.configNum;
            args = CtrlerArgs(query);
        } else {
            LOG(FATAL) << "Unkonw cmd: " << cmd;
        }
        return args;
    }

private:
    ShardCtrlerOP op_;
    JoinArgs join_;
    LeaveArgs leave_;
    MoveArgs move_;
    QueryArgs query_;
};

#endif