#include <string>
#include <unordered_set>

#include <shardkv/CtrlerArgs.hpp>
#include <shardkv/ShardCtrler.h>

ShardCtrler::ShardCtrler(std::vector<Host> peers, Host me, std::string persisterDir)
    : raft_(peers, me, persisterDir, this)
    , lastApplyIndex_(0)
    , lastApplyTerm_(0)
{
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
}

void ShardCtrler::apply(ApplyMsg msg)
{
    CtrlerArgs args = CtrlerArgs::deserialize(msg.command);
    switch (args.op()) {
    case ShardCtrlerOP::JOIN: {
        JoinArgs join;
        args.copyTo(join);
    } break;

    case ShardCtrlerOP::LEAVE: {
        LeaveArgs leave;
        args.copyTo(leave);
    } break;

    case ShardCtrlerOP::MOVE: {
        MoveArgs move;
        args.copyTo(move);
    } break;

    case ShardCtrlerOP::QUERY: {
        QueryArgs query;
        args.copyTo(query);
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
    std::lock_guard<std::mutex> guard(lock_);

    auto newConfig = configs_.back();
    for (auto& it : join.servers) {
        newConfig.gid2shards_[it.first] = std::set<ShardId>();
    }

    // balance the load
    using GInfo = std::pair<GID, int>;
    auto less = [](const GInfo n1, const GInfo n2) {
        return n1.second < n2.second;
    };
    std::vector<GInfo> info(newConfig.gid2shards_.size());
    {
        int i = 0;
        for (auto it : newConfig.gid2shards_) {
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
        auto& gid2shards = newConfig.gid2shards_;

        auto sid = *gid2shards[maxg].begin();
        gid2shards[maxg].erase(sid);
        info.front().second++;
        gid2shards[ming].insert(sid);
        info.back().second++;

        int i = 0;
        while ((i + 1) < info.size() && !less(info[i], info[i + 1]))
            swap(info[i], info[i + 1]);

        i = info.size() - 1;
        while (i > 0 && !less(info[i - 1], info[i]))
            swap(info[i - 1], info[i]);
    }

    configs_.push_back(newConfig);

    Reply rep;
    rep.code = ErrorCode::SUCCEED;
    rep.op = ShardCtrlerOP::JOIN;
    rep.wrongLeader = false;

    if (waits_.find(msg.commandIndex) != waits_.end()) {
        waits_[msg.commandIndex].set_value(std::move(rep));
        waits_.erase(msg.commandIndex);
    }
}

void ShardCtrler::handLeave(const LeaveArgs& leave, const ApplyMsg& msg)
{
    std::lock_guard<std::mutex> guard(lock_);

    std::unordered_set<ShardId> shards;
    auto newConfig = configs_.back();
    newConfig.configNum++;
    for (GID gid : leave.gids) {
        for (ShardId id : newConfig.gid2shards_[gid]) {
            shards.insert(id);
        }
        newConfig.gid2shards_.erase(gid);
    }

    /*
     * Add shards to existing groups as load-balanced as possible
     */
    using GInfo = std::pair<GID, int>;
    auto cmp = [](const GInfo n1, const GInfo n2) {
        return n1.second < n2.second;
    };
    std::priority_queue<GInfo, std::vector<GInfo>, decltype(cmp)> pq(cmp);
    for (auto& it : newConfig.gid2shards_) {
        pq.emplace(it.first, it.second.size());
    }

    for (auto sid : shards) {
        auto gi = pq.top();
        GID gid = gi.first;
        int cnt = gi.second;

        pq.pop();
        newConfig.gid2shards_[gid].insert(sid);
        newConfig.shard2gid[sid] = gid;
        pq.emplace(gid, cnt + 1);
    }

    configs_.push_back(newConfig);

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
    std::lock_guard<std::mutex> guard(lock_);

    Config newConfig = configs_.back();
    ShardId sid = move.shard;
    GID gid = move.gid, oldGid = newConfig.shard2gid[sid];

    newConfig.configNum++;
    newConfig.shard2gid[sid] = gid;
    newConfig.gid2shards_[oldGid].erase(sid);
    newConfig.gid2shards_[gid].insert(sid);

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
    {
        std::lock_guard<std::mutex> guard(lock_);
        if (waits_.find(msg.commandIndex) == waits_.end())
            return;
    }

    Reply reply { ShardCtrlerOP::QUERY, false };
    if (query.configNum >= configs_.size()) {
        reply.code = ErrorCode::ERR_NO_SUCH_SHARD_CONFIG;
    } else {
        reply.code = ErrorCode::SUCCEED;
        reply.config = configs_[query.configNum];
    }

    {
        std::lock_guard<std::mutex> guard(lock_);
        waits_[msg.commandIndex].set_value(std::move(reply));
        waits_.erase(msg.commandIndex);
    }
}

ShardCtrler::Reply ShardCtrler::sendArgsToRaft(const CtrlerArgs& args)
{
    std::string cmd = CtrlerArgs::serialize(args);
    StartResult rs;
    raft_.start(rs, cmd);

    Reply re;
    re.wrongLeader = !rs.isLeader;

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