
#include <glog/logging.h>
#include <thrift/Thrift.h>

#include <common.h>
#include <shardkv/ShardCtrlerClerk.h>

template <class RT, class AT>
void sendRequest(RT& _return, const AT& args,
    std::vector<Host>& hosts_,
    ClientManager<ShardctrlerClient>& cm_,
    int& leaderId_,
    std::function<void(ShardctrlerClient*, RT&, const AT&)> handler)
{
    auto sendRequestTo = [&hosts_, &cm_, &leaderId_, &handler](RT& _return, const AT& args) {
        try {
            auto* client = cm_.getClient(leaderId_, hosts_[leaderId_]);
            handler(client, _return, args);
        } catch (apache::thrift::TException& tx) {
            cm_.setInvalid(leaderId_);
            _return.wrongLeader = true;
        }
    };

    if (leaderId_ != -1) {
        sendRequestTo(_return, args);
        if (_return.wrongLeader) {
            leaderId_ = -1;
        }
    }

    if (leaderId_ == -1) {
        for (int i = 0; i < hosts_.size(); i++) {
            sendRequestTo(_return, args);

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
    , cm_(ClientManager<ShardctrlerClient>(hosts.size(), KV_PRC_TIMEOUT))
    , leaderId_(-1)
{
}

void ShardctrlerClerk::join(JoinReply& _return, const JoinArgs& jargs)
{
    std::function<void(ShardctrlerClient*, JoinReply&, const JoinArgs&)> handler = [](ShardctrlerClient* client, JoinReply& reply, const JoinArgs& args) {
        client->join(reply, args);
    };
    sendRequest(_return, jargs, hosts_, cm_, leaderId_, handler);
}

void ShardctrlerClerk::leave(LeaveReply& _return, const LeaveArgs& largs)
{
    std::function<void(ShardctrlerClient*, LeaveReply&, const LeaveArgs&)> handler = [](ShardctrlerClient* client, LeaveReply& reply, const LeaveArgs& args) {
        client->leave(reply, args);
    };
    sendRequest(_return, largs, hosts_, cm_, leaderId_, handler);
}

void ShardctrlerClerk::move(MoveReply& _return, const MoveArgs& margs)
{
    std::function<void(ShardctrlerClient*, MoveReply&, const MoveArgs&)> handler = [](ShardctrlerClient* client, MoveReply& reply, const MoveArgs& args) {
        client->move(reply, args);
    };
    sendRequest(_return, margs, hosts_, cm_, leaderId_, handler);
}

void ShardctrlerClerk::query(QueryReply& _return, const QueryArgs& qargs)
{
    std::function<void(ShardctrlerClient*, QueryReply&, const QueryArgs&)> handler = [](ShardctrlerClient* client, QueryReply& reply, const QueryArgs& args) {
        client->query(reply, args);
    };
    sendRequest(_return, qargs, hosts_, cm_, leaderId_, handler);
}