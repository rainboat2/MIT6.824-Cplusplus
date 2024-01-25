#include <future>

#include <shardkv/ShardGroup.h>

ShardGroup::ShardGroup(std::vector<Host>& peers, Host me, std::string persisterDir, GID gid)
    : raft_(std::make_unique<RaftHandler>(peers, me, persisterDir, &shardManger_, gid))
{
}

void ShardManger::apply(ApplyMsg msg)
{
    auto args = KVArgs::deserialize(msg.command);
    ShardReply rep;

    switch (args.op()) {
    case KVArgsOP::GET: {
        GetReply reply;
        GetParams gp;
        args.copyTo(gp);
        handleGet(reply, gp);
        rep = ShardReply(reply);
    } break;

    case KVArgsOP::PUT: {
        PutAppendReply reply;
        PutAppendParams pp;
        args.copyTo(pp);
        handlePutAppend(reply, pp);
        rep = ShardReply(reply);
    } break;

    default:
        LOG(FATAL) << "Unexpected op: " << static_cast<int>(args.op());
    }

    std::lock_guard<std::mutex> guard(lock_);
    LogId id = msg.commandIndex;
    if (waits_.find(id) != waits_.end()) {
        waits_[id].set_value(rep);
        waits_.erase(id);
    }
}

std::future<ShardReply> ShardManger::getFuture(LogId id)
{
    std::lock_guard<std::mutex> guard(lock_);
    return waits_[id].get_future();
}

void ShardManger::handlePutAppend(PutAppendReply& _return, const PutAppendParams& params)
{
    ShardId sid = params.sid;
    if (checkShard(sid, _return.code) != ErrorCode::SUCCEED)
        return;

    auto shard = shards_[sid];
    _return = shard.kv.putAppend(params);
}

void ShardManger::handleGet(GetReply& _return, const GetParams& params)
{
    ShardId sid = params.sid;
    if (checkShard(sid, _return.code) != ErrorCode::SUCCEED)
        return;

    auto shard = shards_[sid];
    _return = shard.kv.get(params);
}

ErrorCode::type ShardManger::checkShard(ShardId sid, ErrorCode::type& code)
{
    if (shards_.find(sid) == shards_.end()) {
        code = ErrorCode::ERR_NO_SHARD;
        return code;
    }

    auto& shard = shards_[sid];
    switch (shard.status) {
    case ShardStatus::PULLING:
    case ShardStatus::PUSHING:
        code = ErrorCode::ERR_SHARD_MIGRATING;
        break;
    case ShardStatus::STOP:
        code = ErrorCode::ERR_SHARD_STOP;
    case ShardStatus::SERVERING:
        code = ErrorCode::SUCCEED;
    default:
        LOG(FATAL) << "Unexpected shard status: " << shard.status;
        break;
    }
    return code;
}

void ShardManger::startSnapShot(std::string filePath, std::function<void(LogId, TermId)> callback)  {
    // TODO:
}

void ShardManger::applySnapShot(std::string filePath) {
    // TODO:
}

void ShardGroup::pullShardParams(PullShardReply& _return, const PullShardParams& params)
{
    // TODO:
}

void ShardGroup::putAppend(PutAppendReply& _return, const PutAppendParams& params)
{
    KVArgs args(params);
    std::future<ShardReply> f;
    bool isLeader = sendArgsToRaft(f, args);

    if (isLeader) {
        f.wait();
        auto rep = f.get();
        rep.copyTo(_return);
    } else {
        _return.code = ErrorCode::ERR_WRONG_LEADER;
    }
}

void ShardGroup::get(GetReply& _return, const GetParams& params)
{
    KVArgs args(params);
    std::future<ShardReply> f;
    bool isLeader = sendArgsToRaft(f, args);

    if (isLeader) {
        f.wait();
        auto rep = f.get();
        rep.copyTo(_return);
    } else {
        _return.code = ErrorCode::ERR_WRONG_LEADER;
    }
}

bool ShardGroup::sendArgsToRaft(std::future<ShardReply>& f, const KVArgs& args)
{
    std::string cmd = KVArgs::serialize(args);
    StartResult rs;
    raft_->start(rs, cmd);
    if (rs.isLeader) {
        auto logId = rs.expectedLogIndex;
        f = shardManger_.getFuture(logId);
    }

    return rs.isLeader;
}