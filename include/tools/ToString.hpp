#ifndef TOSTRING_H
#define TOSTRING_H

#include <rpc/kvraft/KVRaft_types.h>
#include <rpc/mapreduce/MapReduce_types.h>
#include <string>

inline std::string to_string(const Host& addr)
{
    return '(' + addr.ip + ',' + std::to_string(addr.port) + ')';
}

inline std::ostream& operator<<(std::ostream& out, const std::vector<std::string>& v)
{
    out << '[';
    for (int i = 0; i < v.size(); i++) {
        if (i != 0)
            out << ',';
        out << v[i];
    }
    out << ']';
    return out;
}

inline std::ostream& operator<<(std::ostream& out, const TaskResponse& res)
{
    out << "{id: " << res.id << ", type: " << to_string(res.type)
        << ", params: " << res.params << ", resultNum: " << res.id << '}';
    return out;
}

inline std::ostream& operator<<(std::ostream& out, const TaskResult& res)
{
    out << "{id: " << res.id << ", type: " << to_string(res.type)
        << ", rs_loc: " << res.rs_loc << '}';
    return out;
}

#endif