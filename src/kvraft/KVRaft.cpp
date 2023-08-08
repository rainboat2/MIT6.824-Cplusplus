#include <fstream>
#include <sstream>
#include <sys/wait.h>

#include <kvraft/KVRaft.h>
#include <rpc/kvraft/KVRaft_types.h>
#include <tools/Timer.hpp>

#include <fmt/format.h>
#include <glog/logging.h>

using std::string;
using std::vector;

KVRaft::KVRaft(vector<Host>& peers, Host me, string persisterDir, std::function<void()> stopListenPort)
    : raft_(std::make_shared<RaftHandler>(peers, me, persisterDir, this))
    , stopListenPort_(stopListenPort)
{
}

void KVRaft::putAppend(PutAppendReply& _return, const PutAppendParams& params)
{
    StartResult sr;
    string type = (params.op == PutOp::PUT ? "PUT" : "APPEND");
    string command = fmt::format("{} {} {}", type, params.key, params.value);
    raft_->start(sr, command);

    if (!sr.isLeader) {
        _return.status = ErrorCode::ERR_WRONG_LEADER;
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

void KVRaft::get(GetReply& _return, const GetParams& params)
{
    StartResult sr;
    string command = fmt::format("GET {}", params.key);
    raft_->start(sr, command);

    if (!sr.isLeader) {
        _return.status = ErrorCode::ERR_WRONG_LEADER;
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

void KVRaft::apply(ApplyMsg msg)
{
    LOG(INFO) << "ApplyMsg: " << msg.commandIndex;
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
            if (putWait_.find(msg.commandIndex) != putWait_.end()) {
                putWait_[msg.commandIndex].set_value(std::move(reply));
                putWait_.erase(msg.commandIndex);
            } else {
                LOG(INFO) << fmt::format("Command {} is not in the putWait_", msg.commandIndex);
            }
        }
        LOG(INFO) << "Apply put command: " << msg.command;
    } else if (cmd.rfind("GET", 0) == 0) {
        GetParams params;
        string type;
        iss >> type >> params.key;
        GetReply reply;
        get_internal(reply, params);
        {
            std::lock_guard<std::mutex> guard(lock_);
            if (getWait_.find(msg.commandIndex) != getWait_.end()) {
                getWait_[msg.commandIndex].set_value(std::move(reply));
                getWait_.erase(msg.commandIndex);
            } else {
                LOG(INFO) << fmt::format("Command {} is not in the getWait_", msg.commandIndex);
            }
        }
        LOG(INFO) << "Apply get command: " << msg.command;
    } else {
        LOG(ERROR) << fmt::format("Invaid command: {}, commandIndex: {}, commandterm: {}", msg.command, msg.commandIndex, msg.commandTerm);
    }

    {
        std::lock_guard<std::mutex> guard(lock_);
        lastApplyIndex_ = msg.commandIndex;
        lastApplyTerm_ = msg.commandTerm;
    }
}

void KVRaft::startSnapShot(std::string filePath, std::function<void(LogId, TermId)> callback)
{
    pid_t pid;
    LogId lastIndex;
    TermId lastTerm;
    {
        // stop KV operations when fork
        std::lock_guard<std::mutex> guard(lock_);
        lastIndex = lastApplyIndex_;
        lastTerm = lastApplyTerm_;
        pid = fork();
    }

    if (pid == 0) {
        stopListenPort_();
        std::ofstream ofs(filePath);
        ofs << lastApplyIndex_ << ' ' << lastApplyTerm_ << '\n';
        for (auto it : um_) {
            ofs << it.first << ' ' << it.second << '\n';
        }
        ofs.flush();
        exit(0);
    } else {
        wait(&pid);
        callback(lastIndex, lastTerm);
    }
}

void KVRaft::applySnapShot(std::string filePath)
{
    std::lock_guard<std::mutex> guard(lock_);
    um_.clear();
    std::ifstream ifs(filePath);
    ifs >> lastApplyIndex_ >> lastApplyTerm_;
    string key, val;
    while (ifs >> key >> val) {
        um_[key] = val;
    }
}

void KVRaft::putAppend_internal(PutAppendReply& _return, const PutAppendParams& params)
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
    _return.status = ErrorCode::SUCCEED;
}

void KVRaft::get_internal(GetReply& _return, const GetParams& params)
{
    if (um_.find(params.key) == um_.end()) {
        _return.status = ErrorCode::ERR_NO_KEY;
    } else {
        _return.status = ErrorCode::SUCCEED;
        _return.value = um_[params.key];
    }
}