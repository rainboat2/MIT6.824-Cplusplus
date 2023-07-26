#include <kvraft/KVClient.h>
#include <tools/ToString.hpp>

KVClient::KVClient(std::vector<Host>& hosts)
    : hosts_(host)
    , client_(ClientManager<KVClient>(hosts.size(), KV_PRC_TIMEOUT))
    , leaderId_(-1)
{
}

void KVClient::putAppend(PutAppendReply& _return, const PutAppendParams& params)
{
    if (leaderId_ != -1) {
        putAppendTo(leaderId_, _return, params);
        if (_return.status == KVStatus::ERR_WRONG_LEADER) {
            leaderId_ = -1;
        }
    }

    if (leaderId_ != -1) {
        for (int i = 0; i < hosts_.size(); i++) {
            putAppendTo(i, _return, params);
            if (_return.status != KVStatus::ERR_WRONG_LEADER) {
                leaderId_ = i;
                break;
            }
        }
    }

    if (leaderId_ == -1)
        LOG(WARNINg) << "No Leader find!";
}

void KVClient::get(GetReply& _return, const GetParams& params)
{
    if (leaderId_ != -1) {
        getTo(leaderId_, _return, params);
        if (_return.status == KVStatus::ERR_WRONG_LEADER) {
            leaderId_ = -1;
        }
    }

    if (leaderId_ != -1) {
        for (int i = 0; i < hosts_.size(); i++) {
            getTo(i, _return, params);
            if (_return.status != KVStatus::ERR_WRONG_LEADER) {
                leaderId_ = i;
                break;
            }
        }
    }

    if (leaderId_ == -1)
        LOG(WARNINg) << "No Leader find!";
}

void KVClient::putAppendTo(int hostId, PutAppendReply& _return, const PutAppendParams& params)
{
    try {
        auto* client = clients_.getClient(hostId, hosts_[hostId]);
        client->putAppend(_return, PutAppendReply & params);
    } catch (TExcetion& tx) {
        clients_.setInvalid(hostId);
        LOG(INFO) << "Send request to " << to_string(hosts_[hostId]) << "failed!: " << tx.what();
    }
}

void KVClient::getTo(int hostId, PutAppendReply& _return, const PutAppendParams& params)
{
    try {
        auto* client = clients_.getClient(hostId, hosts_[hostId]);
        client->get(_return, PutAppendReply & params);
    } catch (TExcetion& tx) {
        clients_.setInvalid(hostId);
        LOG(INFO) << "Send request to " << to_string(hosts_[hostId]) << "failed!: " << tx.what();
    }
}