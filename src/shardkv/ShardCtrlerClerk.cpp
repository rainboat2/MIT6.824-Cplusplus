
#include <fmt/format.h>
#include <glog/logging.h>
#include <thrift/Thrift.h>

#include <common.h>
#include <shardkv/ShardCtrlerClerk.h>
#include <tools/ToString.hpp>

template <class RT, class AT>
void sendRequest(RT& _return, const AT& args,
    std::vector<Host>& hosts_,
    ClientManager<ShardCtrlerClient>& cm_,
    int& leaderId_,
    std::function<void(ShardCtrlerClient*, RT&, const AT&)> handler)
{
    auto sendRequestTo = [&hosts_, &cm_, &handler](int hostId, RT& _return, const AT& args) {
        try {
            Host& host = hosts_[hostId];
            auto* client = cm_.getClient(hostId, host);
            handler(client, _return, args);
            LOG(INFO) << fmt::format("Send request to {}, result is (wrongLeader= {}, code={})",
                to_string(host), _return.wrongLeader, to_string(_return.code));
        } catch (apache::thrift::TException& tx) {
            LOG(INFO) << "Request failed!: " << tx.what();
            cm_.setInvalid(hostId);
            _return.wrongLeader = true;
            _return.code = ErrorCode::ERR_REQUEST_FAILD;
        }
    };

    if (leaderId_ != -1) {
        sendRequestTo(leaderId_, _return, args);
        if (_return.wrongLeader) {
            leaderId_ = -1;
        }
    }

    if (leaderId_ == -1) {
        for (uint i = 0; i < hosts_.size(); i++) {
            sendRequestTo(i, _return, args);
            if (!_return.wrongLeader) {
                leaderId_ = i;
                break;
            }
        }
    }

    if (leaderId_ == -1) {
        LOG(WARNING) << "Leader not find!";
    }
}

ShardctrlerClerk::ShardctrlerClerk(std::vector<Host>& hosts)
    : hosts_(hosts)
    , cm_(ClientManager<ShardCtrlerClient>(hosts.size(), KV_PRC_TIMEOUT))
    , leaderId_(-1)
{
}

void ShardctrlerClerk::join(JoinReply& _return, const JoinArgs& jargs)
{
    std::function<void(ShardCtrlerClient*, JoinReply&, const JoinArgs&)> handler = [](ShardCtrlerClient* client, JoinReply& reply, const JoinArgs& args) {
        client->join(reply, args);
    };
    sendRequest(_return, jargs, hosts_, cm_, leaderId_, handler);
}

void ShardctrlerClerk::leave(LeaveReply& _return, const LeaveArgs& largs)
{
    std::function<void(ShardCtrlerClient*, LeaveReply&, const LeaveArgs&)> handler = [](ShardCtrlerClient* client, LeaveReply& reply, const LeaveArgs& args) {
        client->leave(reply, args);
    };
    sendRequest(_return, largs, hosts_, cm_, leaderId_, handler);
}

void ShardctrlerClerk::move(MoveReply& _return, const MoveArgs& margs)
{
    std::function<void(ShardCtrlerClient*, MoveReply&, const MoveArgs&)> handler = [](ShardCtrlerClient* client, MoveReply& reply, const MoveArgs& args) {
        client->move(reply, args);
    };
    sendRequest(_return, margs, hosts_, cm_, leaderId_, handler);
}

void ShardctrlerClerk::query(QueryReply& _return, const QueryArgs& qargs)
{
    std::function<void(ShardCtrlerClient*, QueryReply&, const QueryArgs&)> handler = [](ShardCtrlerClient* client, QueryReply& reply, const QueryArgs& args) {
        client->query(reply, args);
    };
    sendRequest(_return, qargs, hosts_, cm_, leaderId_, handler);
}