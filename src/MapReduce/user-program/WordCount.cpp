#include <mapreduce/MapReduce.h>

using std::vector;
using std::string;
using std::isalpha;

/*
 * key: file name
 * value: file content
 */
extern "C"
vector<KeyValue> wordCountMapF(string& key, string& value) {
    const int N = value.size();
    int i = 0;
    while(i < N && !isalpha(value[i])) {
        i++;
    }

    vector<KeyValue> rs;
    int j = i;
    while (i < N) {
        while (j < N && isalpha(value[j])) {
            j++;
        }
        rs.push_back({value.substr(i, j - i), "1"});
        while (j < N && !isalpha(value[j])) {
            j++;
        }
        i = j;
    }
    return rs;
}

/*
 * key: a word
 * value: a list of counts
 */
extern "C"
vector<string> wordCountReduceF(string& key, vector<string>& value) {
    return {std::to_string(value.size())};
}
