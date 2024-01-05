#include <string>
#include <unordered_set>

#include <shardkv/CtrlerArgs.hpp>
#include <shardkv/ShardCtrler.h>
#include <tools/Timer.hpp>
#include <tools/ToString.hpp>

#include <glog/logging.h>
#include <glog/stl_logging.h>

struct GInfo {
    GID gid;
    int shardCnt;

    GInfo() = default;
    GInfo(GID id, int cnt)
        : gid(id)
        , shardCnt(cnt)
    {
    }

    bool operator<(const GInfo& g) const
    {
        return shardCnt < g.shardCnt;
    }

public:
    struct Cmp {
        bool operator()(const GInfo& lhs, const GInfo& rhs)
        {
            return lhs.gid < rhs.gid;
        }
    };
};

static std::ostream& operator<<(std::ostream& out, const GInfo& info)
{
    out << fmt::format("({}, {})", info.gid, info.shardCnt);
    return out;
}

ShardCtrler::ShardCtrler(std::vector<Host> peers, Host me, std::string persisterDir, int shards)
    : raft_(std::make_unique<RaftHandler>(peers, me, persisterDir, this))
    , lastApplyIndex_(0)
    , lastApplyTerm_(0)
{
    Config init;
    init.configNum = 0;
    init.shard2gid = std::vector<GID>(shards, INVALID_GID);
    configs_.push_back(std::move(init));
}

void ShardCtrler::join(JoinReply& _return, const JoinArgs& jargs)
{
    CtrlerArgs args(jargs);
    auto reply = sendArgsToRaft(args);
    _return.code = reply.code;
    _return.wrongLeader = reply.wrongLeader;
}

void ShardCtrler::leave(LeaveReply& _return, const LeaveArgs& largs)
{
    CtrlerArgs args(largs);
    auto reply = sendArgsToRaft(args);
    _return.code = reply.code;
    _return.wrongLeader = reply.wrongLeader;
}

void ShardCtrler::move(MoveReply& _return, const MoveArgs& margs)
{
    CtrlerArgs args(margs);
    auto reply = sendArgsToRaft(args);
    _return.code = reply.code;
    _return.wrongLeader = reply.wrongLeader;
}

void ShardCtrler::query(QueryReply& _return, const QueryArgs& qargs)
{
    CtrlerArgs args(qargs);
    auto reply = sendArgsToRaft(args);
    _return.code = reply.code;
    _return.wrongLeader = reply.wrongLeader;
    _return.config = std::move(reply.config);
}

void ShardCtrler::apply(ApplyMsg msg)
{
    Timer t(fmt::format("Apply cmd {}", msg.command), fmt::format("Finsh cmd {}", msg.command));
    std::lock_guard<std::mutex> guard(lock_);

    CtrlerArgs args = CtrlerArgs::deserialize(msg.command);
    Reply rep;
    switch (args.op()) {
    case ShardCtrlerOP::JOIN: {
        JoinArgs join;
        args.copyTo(join);
        rep = handleJoin(join, msg);
    } break;

    case ShardCtrlerOP::LEAVE: {
        LeaveArgs leave;
        args.copyTo(leave);
        rep = handleLeave(leave, msg);
    } break;

    case ShardCtrlerOP::MOVE: {
        MoveArgs move;
        args.copyTo(move);
        rep = handleMove(move, msg);
    } break;

    case ShardCtrlerOP::QUERY: {
        QueryArgs query;
        args.copyTo(query);
        rep = handleQuery(query, msg);
    } break;

    default:
        LOG(FATAL) << "Unkonw args type!";
    }

    if (waits_.find(msg.commandIndex) != waits_.end()) {
        waits_[msg.commandIndex].set_value(std::move(rep));
        waits_.erase(msg.commandIndex);
    }

    LOG_IF(FATAL, lastApplyIndex_ + 1 != msg.commandIndex) << "Violation of linear consistency!";
    lastApplyIndex_ = msg.commandIndex;
    lastApplyTerm_ = msg.commandTerm;
}

void ShardCtrler::startSnapShot(std::string filePath, std::function<void(LogId, TermId)> callback)
{
    // do nothing
}

void ShardCtrler::applySnapShot(std::string filePath)
{
    // do nothing
}

ShardCtrler::Reply ShardCtrler::handleJoin(const JoinArgs& join, const ApplyMsg& msg)
{
    auto newConfig = configs_.back();
    newConfig.configNum++;

    auto& gid2shards = newConfig.gid2shards;
    auto& shard2gid = newConfig.shard2gid;

    bool noGroup = gid2shards.empty();
    for (auto& it : join.servers) {
        newConfig.gid2shards[it.first] = std::set<ShardId>();
    }

    // add all shared to the new group if there is no group exist
    if (noGroup) {
        int shardNum = shard2gid.size();
        ShardId sid = 0;
        while (sid < shardNum) {
            for (auto& it : gid2shards) {
                if (sid >= shardNum)
                    break;

                it.second.insert(sid);
                shard2gid[sid] = it.first;
                sid++;
            }
        }
    }

    // balance the load
    std::vector<GInfo> info(gid2shards.size());
    {
        uint i = 0;
        for (auto it : newConfig.gid2shards) {
            info[i].gid = it.first;
            info[i].shardCnt = it.second.size();
            i++;
        }
    }
    sort(info.begin(), info.end());

    // adjust the balance util the difference bwtween max load and min load is 1
    while (true) {
        int diff = info.front().shardCnt - info.back().shardCnt;
        if (abs(diff) <= 1)
            break;

        GID maxg = info.back().gid;
        GID ming = info.front().gid;

        auto sid = *gid2shards[maxg].begin();
        gid2shards[maxg].erase(sid);
        info.front().shardCnt++;
        gid2shards[ming].insert(sid);
        info.back().shardCnt--;

        uint i = 0;
        while ((i + 1) < info.size() && info[i + 1] < info[i])
            std::swap(info[i], info[i + 1]);

        i = info.size() - 1;
        while (i > 0 && info[i] < info[i - 1])
            std::swap(info[i - 1], info[i]);
    }

    configs_.push_back(std::move(newConfig));
    LOG(INFO) << "After join, new config: " << to_string(newConfig) << ", groupInfo: " << info;

    return defaultReply(ShardCtrlerOP::JOIN);
}

ShardCtrler::Reply ShardCtrler::handleLeave(const LeaveArgs& leave, const ApplyMsg& msg)
{
    auto newConfig = configs_.back();
    newConfig.configNum++;

    auto& gid2shards = newConfig.gid2shards;
    auto& shard2gid = newConfig.shard2gid;

    std::unordered_set<ShardId> shards;
    for (GID gid : leave.gids) {
        for (ShardId id : gid2shards[gid]) {
            shards.insert(id);
        }
        gid2shards.erase(gid);
    }

    if (gid2shards.empty()) {
        for (auto sid : shards) {
            shard2gid[sid] = INVALID_GID;
        }
    } else {
        /*
         * Add shards to existing groups as load-balanced as possible
         */
        std::priority_queue<GInfo, std::vector<GInfo>, GInfo::Cmp> pq;
        for (auto& it : gid2shards) {
            pq.emplace(it.first, it.second.size());
        }

        for (auto sid : shards) {
            auto gi = pq.top();
            GID gid = gi.gid;
            int cnt = gi.shardCnt;

            pq.pop();
            gid2shards[gid].insert(sid);
            shard2gid[sid] = gid;
            pq.emplace(gid, cnt + 1);
        }
    }

    configs_.push_back(std::move(newConfig));

    return defaultReply(ShardCtrlerOP::LEAVE);
}

ShardCtrler::Reply ShardCtrler::handleMove(const MoveArgs& move, const ApplyMsg& msg)
{
    Config newConfig = configs_.back();
    ShardId sid = move.shard;
    GID gid = move.gid, oldGid = newConfig.shard2gid[sid];

    newConfig.configNum++;
    newConfig.shard2gid[sid] = gid;
    newConfig.gid2shards[oldGid].erase(sid);
    newConfig.gid2shards[gid].insert(sid);

    configs_.push_back(std::move(newConfig));

    return defaultReply(ShardCtrlerOP::MOVE);
}

ShardCtrler::Reply ShardCtrler::handleQuery(const QueryArgs& query, const ApplyMsg& msg)
{
    Reply rep = defaultReply(ShardCtrlerOP::QUERY);

    if (waits_.find(msg.commandIndex) == waits_.end())
        return rep;

    const int cn = query.configNum;

    if (cn != LATEST_CONFIG_NUM && cn >= static_cast<int>(configs_.size())) {
        rep.code = ErrorCode::ERR_NO_SUCH_SHARD_CONFIG;
    } else {
        rep.code = ErrorCode::SUCCEED;

        if (cn == LATEST_CONFIG_NUM) {
            rep.config = configs_.back();
            LOG(INFO) << "Get Latest config " << rep.config;
        } else {
            rep.config = configs_[cn];
            LOG(INFO) << "Get config at " << cn << ": " << rep.config;
        }
    }
    return rep;
}

ShardCtrler::Reply ShardCtrler::sendArgsToRaft(const CtrlerArgs& args)
{
    std::string cmd = CtrlerArgs::serialize(args);
    StartResult rs;
    raft_->start(rs, cmd);

    Reply re;
    re.wrongLeader = !rs.isLeader;
    re.code = ErrorCode::SUCCEED;

    if (re.wrongLeader) {
        re.code = ErrorCode::ERR_WRONG_LEADER;
        return re;
    }

    auto logId = rs.expectedLogIndex;
    std::future<Reply> f;

    {
        std::lock_guard<std::mutex> guard(lock_);
        f = waits_[logId].get_future();
    }

    f.wait();
    return f.get();
}