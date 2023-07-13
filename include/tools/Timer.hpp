#ifndef TIMER_HPP
#define TIMER_HPP

#include <vector>
#include <string>

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
        auto end = std::chrono::steady_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_);
        LOG(INFO) << m << " Used: " << dur.count() << "ms";
    }

    ~Timer()
    {
        auto end = std::chrono::steady_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_);
        LOG(INFO) << end_msg_ << " Used: " << dur.count() << "ms";
    }

private:
    std::chrono::steady_clock::time_point start_;
    std::string end_msg_;
};

#endif