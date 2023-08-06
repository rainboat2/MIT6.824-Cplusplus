#ifndef SHARDCTRLERCLIENT_H
#define SHARDCTRLERCLIENT_H

#include <rpc/kvraft/KVRaft_types.h>
#include <rpc/kvraft/Shardctrler.h>

class ShardctrlerClerk {
public:
    void join(JoinReply& _return, const JoinArgs& jargs);

    void leave(LeaveReply& _return, const LeaveArgs& largs);

    void move(MoveReply& _return, const MoveArgs& margs);

    void query(QueryReply& _return, const QueryArgs& qargs);

private:
    
};

#endif