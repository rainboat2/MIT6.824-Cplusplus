#ifndef RAFT_FLAGS
#define RAFT_FLAGS

#include <string>
#include <vector>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <rpc/kvraft/KVRaft_types.h>

DEFINE_string(peers, "", "address of other raft servers, fomat: ip:port,ip:port...");
DEFINE_string(self_addr, "", "self address, fomat ip:port");

inline std::vector<Host> getPeerAddress(Host& me)
{
    auto extractAddr = [](std::string& host) {
        LOG(INFO) << host;
        uint k = 0;
        while (k < host.size() && host[k] != ':')
            k++;
        if (k == host.size())
            LOG(FATAL) << "Bad ip address fomat: " << host << " rigth fomat: ip:port";
        Host addr;
        addr.ip = host.substr(0, k);
        addr.port = std::stoi(host.substr(k + 1));
        return addr;
    };

    std::vector<Host> rs;
    std::string peers = FLAGS_peers;
    uint i = 0, j = 0;
    while (i < peers.size()) {
        while (j < peers.size() && peers[j] != ',') {
            j++;
        }
        std::string host = peers.substr(i, j - i);
        rs.push_back(extractAddr(host));
        i = j;
    }

    me = extractAddr(FLAGS_self_addr);
    return rs;
}

#endif