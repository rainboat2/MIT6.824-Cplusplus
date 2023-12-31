#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <gflags/gflags.h>
#include <glog/logging.h>

inline long long epochInMs()
{
    using namespace std::chrono;
    auto now = duration_cast<milliseconds>(steady_clock::now().time_since_epoch());
    return now.count();
}

class Timer {
public:
    Timer()
        : printMsg_(false)
        , start_(std::chrono::steady_clock::now())
    {
    }

    Timer(std::string start_msg, std::string end_msg, bool printMsg = true)
        : printMsg_(printMsg)
        , start_(std::chrono::steady_clock::now())
        , end_msg_(std::move(end_msg))
    {
        LOG(INFO) << start_msg;
    }

    void checkpoint(std::string m)
    {
        if (printMsg_) {
            auto dur = duration();
            LOG(INFO) << m << " Used: " << dur.count() << "ms";
        }
    }

    std::chrono::milliseconds duration()
    {
        auto cur = std::chrono::steady_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(cur - start_);
        return dur;
    }

    ~Timer()
    {
        if (printMsg_) {
            auto dur = duration();
            LOG(INFO) << end_msg_ << " Used: " << dur.count() << "ms";
        }
    }

private:
    bool printMsg_;
    std::chrono::steady_clock::time_point start_;
    std::string end_msg_;
};

#endif