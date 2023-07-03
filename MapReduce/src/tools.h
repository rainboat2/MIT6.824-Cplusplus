#ifndef TOOLS_H
#define TOOLS_H

#include <iostream>
#include <sstream>
#include <chrono>

#include "gen-cpp/master_types.h"

constexpr auto TIME_OUT = std::chrono::seconds(5);

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