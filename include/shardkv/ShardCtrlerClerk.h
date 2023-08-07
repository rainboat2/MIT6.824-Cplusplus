#ifndef SHARDCTRLERCLIENT_H
#define SHARDCTRLERCLIENT_H

#include <vector>

#include <rpc/kvraft/KVRaft_types.h>
#include <rpc/kvraft/Shardctrler.h>
#include <tools/ClientManager.hpp>

class ShardctrlerClerk {
public:
    ShardctrlerClerk(std::vector<Host>& hosts);

    void join(JoinReply& _return, const JoinArgs& jargs);

    void leave(LeaveReply& _return, const LeaveArgs& largs);

    void move(MoveReply& _return, const MoveArgs& margs);

    void query(QueryReply& _return, const QueryArgs& qargs);

private:
    std::vector<Host> hosts_;
    ClientManager<ShardctrlerClient> cm_;
    int leaderId_;
};

#endif