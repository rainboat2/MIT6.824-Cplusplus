#include <kvraft/KVServer.h>
#include <raft/rpc/raft_types.h>

using std::string;
using std::vector;

KVServer::KVServer(vector<RaftAddr>& peers, RaftAddr me, string persisterDir)
    : raft_(std::make_shared<RaftRPCHandler>(peers, me, persisterDir))
{

}

void KVServer::putAppend(PutAppenRely& _return, const PutAppendArgs& args)
{
}

void KVServer::get(GetReply& _return, const GetArgs& args)
{
}