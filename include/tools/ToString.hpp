#include <raft/rpc/raft_types.h>
#include <string>

inline std::string to_string(const RaftAddr& addr)
{
    return '(' + addr.ip + ',' + std::to_string(addr.port) + ')';
}