#ifndef TIMER_H
#define TIMER_H

#include <string>
#include <vector>

#include <fmt/format.h>
#include <gflags/gflags.h>
#include <glog/logging.h>

class Timer {
public:
    Timer(std::string start_msg, std::string end_msg)
        : end_msg_(std::move(end_msg))
    {
        start_ = std::chrono::steady_clock::now();
        LOG(INFO) << start_msg;
    }

    void msg(std::string m)
    {
        auto dur = duration();
        LOG(INFO) << m << " Used: " << dur.count() << "ms";
    }

    std::chrono::milliseconds duration() {
        auto cur = std::chrono::steady_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(cur - start_);
        return dur;
    }

    ~Timer()
    {
        auto dur = duration();
        LOG(INFO) << end_msg_ << " Used: " << dur.count() << "ms";
    }

private:
    std::chrono::steady_clock::time_point start_;
    std::string end_msg_;
};

#endif