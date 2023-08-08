#include <string>
#include <unordered_set>

#include <shardkv/CtrlerArgs.hpp>
#include <shardkv/ShardCtrler.h>
#include <tools/Timer.hpp>
#include <tools/ToString.hpp>

ShardCtrler::ShardCtrler(std::vector<Host> peers, Host me, std::string persisterDir, int shards)
    : raft_(std::make_shared<RaftHandler>(peers, me, persisterDir, this))
    , lastApplyIndex_(0)
    , lastApplyTerm_(0)
{
    Config init;
    init.configNum = 0;
    init.shard2gid = std::vector<GID>(shards, -1);
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
    std::lock_guard<std::mutex> guard(lock_);

    CtrlerArgs args = CtrlerArgs::deserialize(msg.command);
    switch (args.op()) {
    case ShardCtrlerOP::JOIN: {
        JoinArgs join;
        args.copyTo(join);
        handleJoin(join, msg);
    } break;

    case ShardCtrlerOP::LEAVE: {
        LeaveArgs leave;
        args.copyTo(leave);
        handleLeave(leave, msg);
    } break;

    case ShardCtrlerOP::MOVE: {
        MoveArgs move;
        args.copyTo(move);
        handleMove(move, msg);
    } break;

    case ShardCtrlerOP::QUERY: {
        QueryArgs query;
        args.copyTo(query);
        handleQuery(query, msg);
    } break;

    default:
        LOG(FATAL) << "Unkonw args type!";
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

void ShardCtrler::handleJoin(const JoinArgs& join, const ApplyMsg& msg)
{
    auto newConfig = configs_.back();
    newConfig.configNum++;

    bool noGroup = newConfig.gid2shards.empty();
    for (auto& it : join.servers) {
        newConfig.gid2shards[it.first] = std::set<ShardId>();
    }

    if (noGroup) {
        auto& gid2shards = newConfig.gid2shards;
        auto& shard2gid = newConfig.shard2gid;
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
    using GInfo = std::pair<GID, int>;
    auto less = [](const GInfo n1, const GInfo n2) {
        return n1.second < n2.second;
    };
    std::vector<GInfo> info(newConfig.gid2shards.size());
    {
        uint i = 0;
        for (auto it : newConfig.gid2shards) {
            info[i].first = it.first;
            info[i].second = it.second.size();
            i++;
        }
    }
    sort(info.begin(), info.end(), less);

    // adjust the balance util the difference bwtween max load and min load is 1
    while (info.front().second < info.back().second - 1) {
        GID maxg = info.back().first;
        GID ming = info.front().first;
        auto& gid2shards = newConfig.gid2shards;

        auto sid = *gid2shards[maxg].begin();
        gid2shards[maxg].erase(sid);
        info.front().second++;
        gid2shards[ming].insert(sid);
        info.back().second++;

        uint i = 0;
        while ((i + 1) < info.size() && !less(info[i], info[i + 1]))
            swap(info[i], info[i + 1]);

        i = info.size() - 1;
        while (i > 0 && !less(info[i - 1], info[i]))
            swap(info[i - 1], info[i]);
    }

    configs_.push_back(std::move(newConfig));
    LOG(INFO) << "After join, new config: " << to_string(newConfig);

    Reply rep;
    rep.code = ErrorCode::SUCCEED;
    rep.op = ShardCtrlerOP::JOIN;
    rep.wrongLeader = false;

    if (waits_.find(msg.commandIndex) != waits_.end()) {
        waits_[msg.commandIndex].set_value(std::move(rep));
        waits_.erase(msg.commandIndex);
    }
}

void ShardCtrler::handleLeave(const LeaveArgs& leave, const ApplyMsg& msg)
{
    std::unordered_set<ShardId> shards;
    auto newConfig = configs_.back();
    newConfig.configNum++;
    for (GID gid : leave.gids) {
        for (ShardId id : newConfig.gid2shards[gid]) {
            shards.insert(id);
        }
        newConfig.gid2shards.erase(gid);
    }

    /*
     * Add shards to existing groups as load-balanced as possible
     */
    using GInfo = std::pair<GID, int>;
    auto cmp = [](const GInfo n1, const GInfo n2) {
        return n1.second < n2.second;
    };
    std::priority_queue<GInfo, std::vector<GInfo>, decltype(cmp)> pq(cmp);
    for (auto& it : newConfig.gid2shards) {
        pq.emplace(it.first, it.second.size());
    }

    for (auto sid : shards) {
        auto gi = pq.top();
        GID gid = gi.first;
        int cnt = gi.second;

        pq.pop();
        newConfig.gid2shards[gid].insert(sid);
        newConfig.shard2gid[sid] = gid;
        pq.emplace(gid, cnt + 1);
    }

    configs_.push_back(std::move(newConfig));

    Reply rep;
    rep.code = ErrorCode::SUCCEED;
    rep.op = ShardCtrlerOP::LEAVE;
    rep.wrongLeader = false;

    if (waits_.find(msg.commandIndex) != waits_.end()) {
        waits_[msg.commandIndex].set_value(std::move(rep));
        waits_.erase(msg.commandIndex);
    }
}

void ShardCtrler::handleMove(const MoveArgs& move, const ApplyMsg& msg)
{
    Config newConfig = configs_.back();
    ShardId sid = move.shard;
    GID gid = move.gid, oldGid = newConfig.shard2gid[sid];

    newConfig.configNum++;
    newConfig.shard2gid[sid] = gid;
    newConfig.gid2shards[oldGid].erase(sid);
    newConfig.gid2shards[gid].insert(sid);

    configs_.push_back(std::move(newConfig));

    if (waits_.find(msg.commandIndex) != waits_.end()) {
        Reply rep;
        rep.code = ErrorCode::SUCCEED;
        rep.op = ShardCtrlerOP::MOVE;
        rep.wrongLeader = false;

        waits_[msg.commandIndex].set_value(std::move(rep));
        waits_.erase(msg.commandIndex);
    }
}

void ShardCtrler::handleQuery(const QueryArgs& query, const ApplyMsg& msg)
{
    if (waits_.find(msg.commandIndex) == waits_.end())
        return;

    const int cn = query.configNum;
    Reply reply { ShardCtrlerOP::QUERY, false };
    if (cn != LATEST_CONFIG_NUM && cn >= static_cast<int>(configs_.size())) {
        reply.code = ErrorCode::ERR_NO_SUCH_SHARD_CONFIG;
    } else {
        reply.code = ErrorCode::SUCCEED;

        if (cn == LATEST_CONFIG_NUM) {
            reply.config = configs_.back();
            LOG(INFO) << "Get Latest config " << reply.config;
        } else {
            reply.config = configs_[cn];
            LOG(INFO) << "Get config at " << cn << ": " << reply.config;
        }
    }

    waits_[msg.commandIndex].set_value(std::move(reply));
    waits_.erase(msg.commandIndex);
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