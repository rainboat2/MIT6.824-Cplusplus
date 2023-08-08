#ifndef TOSTRING_H
#define TOSTRING_H

#include <deque>
#include <iostream>
#include <string>

#include <rpc/kvraft/KVRaft_types.h>
#include <rpc/mapreduce/MapReduce_types.h>

#include <fmt/format.h>
#include <fmt/ranges.h>

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

inline std::string to_string(const Config& config)
{
    return fmt::format("(configNum = {}, shard2gid = {}, gid2shards = {})", config.configNum, config.shard2gid, config.gid2shards);
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