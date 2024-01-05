#include <future>

#include <shardkv/ShardGroup.h>

ShardGroup::ShardGroup(std::vector<Host>& peers, Host me, std::string persisterDir, GID gid)
    : raft_(std::make_unique<RaftHandler>(peers, me, persisterDir, this, gid))
{
}

void ShardManger::apply(ApplyMsg msg)
{
    auto args = KVArgs::deserialize(msg.command);
    LogId id = msg.commandIndex;
    switch (args.op()) {
    case KVArgsOP::GET: {
       GetParams gp;
       args.copyTo(gp); 
       auto reply = shards_[gp.sid].kv.get(gp);
       
    } break;

    case KVArgsOP::PUT: {

    } break;

    default:
        LOG(FATAL) << "Unexpected op: " << static_cast<int>(args.op());
    }
}

std::future<ShardReply>&& ShardManger::getFuture(LogId id)
{
    std::lock_guard<std::mutex> guard(lock_);
    return waits_[id].get_future();
}

void ShardManger::handlePutAppend(PutAppendReply& _return, const PutAppendParams& params)
{
    ShardId sid = params.sid;
    if (checkShard(sid, _return.code, _return.status) != ErrorCode::SUCCEED)
        return;

    auto shard = shards_[sid];
    _return = shard.kv.putAppend(params);
}

void ShardManger::handleGet(GetReply& _return, const GetParams& params)
{
    ShardId sid = params.sid;
    if (checkShard(sid, _return.code, _return.status) != ErrorCode::SUCCEED)
        return;

    auto shard = shards_[sid];
    _return = shard.kv.get(params);
}

ErrorCode::type ShardManger::checkShard(ShardId sid, ErrorCode::type& code, ShardStatus::type& status)
{
    if (shards_.find(sid) == shards_.end()) {
        code = ErrorCode::ERR_NO_SHARD;
        return;
    }

    auto& shard = shards_[sid];
    if (shard.status != ShardStatus::SERVERING) {
        status = shard.status;
        code = ErrorCode::ERR_INVALID_SHARD;
    } else {
        code = ErrorCode::SUCCEED;
    }
    return code;
}

void ShardGroup::pullShardParams(PullShardReply& _return, const PullShardParams& params)
{
    // TODO:
}

void ShardGroup::putAppend(PutAppendReply& _return, const PutAppendParams& params)
{
    KVArgs args(params);
    auto rep = sendArgsToRaft(args);
    _return.code = rep.code;
    _return.status = rep.status;
}

void ShardGroup::get(GetReply& _return, const GetParams& params)
{
    KVArgs args(params);
    auto rep = sendArgsToRaft(args);
    _return.code = rep.code;
    _return.status = rep.status;
    _return.value = std::move(rep.value);
}

ShardReply ShardGroup::sendArgsToRaft(const KVArgs& args)
{
    std::string cmd = KVArgs::serialize(args);
    StartResult rs;
    raft_->start(rs, cmd);

    ShardReply re;
    re.code = ErrorCode::SUCCEED;

    if (!rs.isLeader) {
        re.code = ErrorCode::ERR_WRONG_LEADER;
        return re;
    }

    auto logId = rs.expectedLogIndex;
    std::future<ShardReply> f = shardManger_.getFuture(logId);

    f.wait();
    return f.get();
}