#ifndef TOSTRING_H
#define TOSTRING_H

#include <deque>
#include <iostream>
#include <string>

#include <rpc/kvraft/KVRaft_types.h>
#include <rpc/mapreduce/MapReduce_types.h>

#include <fmt/format.h>

inline std::string to_string(const Host& addr)
{
    return '(' + addr.ip + ',' + std::to_string(addr.port) + ')';
}

inline std::string logsRange(const std::deque<LogEntry>& logs)
{
    if (!logs.empty())
        return fmt::format("[{}, {}]", logs.front().index, logs.back().index);
    else
        return "[-1, -1]";
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

// inline std::ostream& operator<<(std::ostream& out, const TaskResponse& res)
// {
//     out << "{id: " << res.id << ", type: " << to_string(res.type)
//         << ", params: " << res.params << ", resultNum: " << res.id << '}';
//     return out;
// }

// inline std::ostream& operator<<(std::ostream& out, const TaskResult& res)
// {
//     out << "{id: " << res.id << ", type: " << to_string(res.type)
//         << ", rs_loc: " << res.rs_loc << '}';
//     return out;
// }

#endif