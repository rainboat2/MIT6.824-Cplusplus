#include <kvraft/KVServer.h>
#include <rpc/kvraft/KVRaft_types.h>

using std::string;
using std::vector;

KVServer::KVServer(vector<Host>& peers, Host me, string persisterDir)
    : raft_(std::make_shared<RaftHandler>(peers, me, persisterDir))
{
}

void KVServer::putAppend(PutAppenRely& _return, const PutAppendParams& args)
{
}

void KVServer::get(GetReply& _return, const GetParams& params)
{
}

void KVServer::apply(ApplyMsg& msg)
{
}

void KVServer::startSnapShot(std::string fileName, std::function<void()> callback)
{
}