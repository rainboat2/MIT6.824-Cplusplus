#ifndef MAPREDUCE_H
#define MAPREDUCE_H

#include <string>
#include <vector>

struct KeyValue
{
    std::string key;
    std::string val;
};

using MapFunc = std::vector<KeyValue> (*)(std::string& key, std::string& value);
using ReduceFunc = std::vector<std::string> (*)(std::string& key, std::vector<std::string>& value);

#endif