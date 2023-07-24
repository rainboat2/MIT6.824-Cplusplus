#ifndef KVSERVER_HH
#define KVSERVER_HH

#include <memory>
#include <string>
#include <vector>
#include <mutex>

#include <kvraft/rpc/KVRaft.h>
#include <raft/raft.h>

class KVServer : KVRaftIf {
public:
    KVServer(std::vector<RaftAddr>& peers, RaftAddr me, std::string persisterDir);

    void putAppend(PutAppenRely& _return, const PutAppendArgs& args) override;

    void get(GetReply& _return, const GetArgs& args) override;

private:
    std::shared_ptr<RaftRPCHandler> raft_;
};

#endif