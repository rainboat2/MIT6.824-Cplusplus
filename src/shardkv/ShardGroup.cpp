#include <future>

#include <shardkv/ShardGroup.h>

ShardGroup::ShardGroup(std::vector<Host>& peers, Host me, std::string persisterDir, StateMachineIf* stateMachine, GID gid)
    : raft_(RaftHandler(peers, me, persisterDir, this, gid))
{
}

void ShardGroup::pullShardParams(PullShardReply& _return, const PullShardParams& params)
{
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

void ShardGroup::handlePutAppend(PutAppendReply& _return, const PutAppendParams& params)
{
    ShardId sid = params.sid;
    if (checkShard(sid, _return.code, _return.status) != ErrorCode::SUCCEED)
        return;

    auto shard = shards_[sid];
    _return = shard.kv.putAppend(params);
}

void ShardGroup::handleGet(GetReply& _return, const GetParams& params)
{
    ShardId sid = params.sid;
    if (checkShard(sid, _return.code, _return.status) != ErrorCode::SUCCEED)
        return;

    auto shard = shards_[sid];
    _return = shard.kv.get(params);
}

ErrorCode::type ShardGroup::checkShard(ShardId sid, ErrorCode::type& code, ShardStatus::type& status)
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

ShardGroup::Reply ShardGroup::sendArgsToRaft(const KVArgs& args)
{
    std::string cmd = KVArgs::serialize(args);
    StartResult rs;
    raft_.start(rs, cmd);

    Reply re;
    re.code = ErrorCode::SUCCEED;

    if (!rs.isLeader) {
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