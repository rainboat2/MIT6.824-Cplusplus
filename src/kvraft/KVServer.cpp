#include <sstream>

#include <kvraft/KVServer.h>
#include <rpc/kvraft/KVRaft_types.h>

#include <fmt/format.h>
#include <glog/logging.h>

using std::string;
using std::vector;

KVServer::KVServer(vector<Host>& peers, Host me, string persisterDir, std::function<void()> stopListenPort)
    : raft_(std::make_shared<RaftHandler>(peers, me, persisterDir), this)
    , stopListenPort_(stopListenPort)
{
}

void KVServer::putAppend(PutAppendReply& _return, const PutAppendParams& params)
{
    StartResult sr;
    string type = (params.op == PutOp::PUT ? "PUT" : "APPEND");
    string command = fmt::format("{} {} {}", type, params.key, params.value);
    raft_->start(sr, command);

    if (!sr.isLeader) {
        _return.status = KVStatus::ERR_WRONG_LEADER;
        return;
    }

    auto logId = sr.expectedLogIndex;
    std::future<PutAppendReply> f;

    {
        std::lock_guard<std::mutex> guard(lock_);
        f = putWait_[logId].get_future();
    }

    f.wait();
    _return = std::move(f.get());
}

void KVServer::get(GetReply& _return, const GetParams& params)
{
    StartResult sr;
    string command = fmt::format("GET {}", params.key);
    raft_->start(sr, command);

    if (!sr.isLeader) {
        _return.status = KVStatus::ERR_WRONG_LEADER;
        return;
    }

    auto logId = sr.expectedLogIndex;
    std::future<GetReply> f;

    {
        std::lock_guard<std::mutex> guard(lock_);
        f = getWait_[logId].get_future();
    }

    f.wait();
    _return = std::move(f.get());
}

void KVServer::apply(ApplyMsg msg)
{
    auto& cmd = msg.command;
    std::istringstream iss(cmd.c_str());

    // use rfind to simulate "startswith"
    if (cmd.rfind("APPEND", 0) == 0 || cmd.rfind("PUT", 0) == 0) {
        PutAppendParams params;
        string type;
        iss >> type >> params.key >> params.value;
        params.op = (cmd[0] == 'A' ? PutOp::APPEND : PutOp::PUT);

        PutAppendReply reply;
        putAppend_internal(reply, params);
        {
            std::lock_guard<std::mutex> guard(lock_);
            putWait_[msg.commandIndex].set_value(std::move(reply));
            putWait_.erase(msg.commandIndex);
        }
        LOG(INFO) << "apply put command: " << msg.command;
    } else if (cmd.rfind("GET", 0) == 0) {
        GetParams params;
        string type;
        iss >> type >> params.key;
        GetReply reply;
        get_internal(reply, params);
        {
            std::lock_guard<std::mutex> guard(lock_);
            getWait_[msg.commandIndex].set_value(std::move(reply));
            getWait_.erase(msg.commandIndex);
        }
        LOG(INFO) << "apply get command: " << msg.command;
    } else {
        LOG(ERROR) << "Invaild command: " << cmd;
    }
    lastApply_ = msg.commandIndex;
}

void KVServer::startSnapShot(std::string fileName, std::function<void()> callback)
{
}

void KVServer::putAppend_internal(PutAppendReply& _return, const PutAppendParams& params)
{
    switch (params.op) {
    case PutOp::PUT:
        um_[params.key] = params.value;
        break;

    case PutOp::APPEND:
        um_[params.key] = params.value;
        break;

    default:
        LOG(FATAL) << "Unexpected operation: " << params.op;
        break;
    }
    _return.status = KVStatus::OK;
}

void KVServer::get_internal(GetReply& _return, const GetParams& params)
{
    if (um_.find(params.key) == um_.end()) {
        _return.status = KVStatus::ERR_NO_KEY;
    } else {
        _return.status = KVStatus::OK;
        _return.value = um_[params.key];
    }
}