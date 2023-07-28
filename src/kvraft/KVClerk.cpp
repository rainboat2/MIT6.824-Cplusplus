#include <kvraft/KVClerk.h>
#include <tools/ToString.hpp>

#include <glog/logging.h>
#include <thrift/Thrift.h>

KVClerk::KVClerk(std::vector<Host>& hosts)
    : hosts_(hosts)
    , clients_(ClientManager<KVRaftClient>(hosts.size(), KV_PRC_TIMEOUT))
    , leaderId_(-1)
{
}

void KVClerk::putAppend(PutAppendReply& _return, const PutAppendParams& params)
{
    if (leaderId_ != -1) {
        putAppendTo(leaderId_, _return, params);
        if (_return.status == KVStatus::ERR_WRONG_LEADER) {
            leaderId_ = -1;
        }
    }

    if (leaderId_ == -1) {
        for (int i = 0; i < hosts_.size(); i++) {
            putAppendTo(i, _return, params);
            if (_return.status != KVStatus::ERR_WRONG_LEADER) {
                leaderId_ = i;
                break;
            }
        }
    }

    if (leaderId_ == -1)
        LOG(WARNING) << "No Leader find!";
}

void KVClerk::get(GetReply& _return, const GetParams& params)
{
    if (leaderId_ != -1) {
        getTo(leaderId_, _return, params);
        if (_return.status == KVStatus::ERR_WRONG_LEADER) {
            leaderId_ = -1;
        }
    }

    if (leaderId_ == -1) {
        for (int i = 0; i < hosts_.size(); i++) {
            getTo(i, _return, params);
            if (_return.status != KVStatus::ERR_WRONG_LEADER) {
                leaderId_ = i;
                break;
            }
        }
    }

    if (leaderId_ == -1)
        LOG(WARNING) << "No Leader find!";
}

void KVClerk::putAppendTo(int hostId, PutAppendReply& _return, const PutAppendParams& params)
{
    try {
        auto* client = clients_.getClient(hostId, hosts_[hostId]);
        client->putAppend(_return, params);
    } catch (apache::thrift::TException& tx) {
        clients_.setInvalid(hostId);
        _return.status = KVStatus::ERR_WRONG_LEADER;
        // LOG(INFO) << "Send request to " << to_string(hosts_[hostId]) << "failed!: " << tx.what();
    }
}

void KVClerk::getTo(int hostId, GetReply& _return, const GetParams& params)
{
    try {
        auto* client = clients_.getClient(hostId, hosts_[hostId]);
        client->get(_return, params);
    } catch (apache::thrift::TException& tx) {
        clients_.setInvalid(hostId);
        _return.status = KVStatus::ERR_WRONG_LEADER;
        // LOG(INFO) << "Send request to " << to_string(hosts_[hostId]) << "failed!: " << tx.what();
    }
}