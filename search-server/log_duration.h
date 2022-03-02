#pragma once

#include <chrono>
#include <iostream>

#define PROFILE_CONCAT_INTERNAL(X, Y) X##Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION(x) LogDuration UNIQUE_VAR_NAME_PROFILE(x)
#define LOG_DURATION_STREAM(x, out) LogDuration UNIQUE_VAR_NAME_PROFILE(x, out)

class LogDuration {
public:
    // заменим имя типа std::chrono::steady_clock
    // с помощью using для удобства
    using Clock = std::chrono::steady_clock;
    
    LogDuration(const std::string& id, std::ostream& out=std::cerr);

    ~LogDuration();

private:
    const std::string id_;
    std::ostream& out_;
    const Clock::time_point start_time_ = Clock::now();
};

LogDuration::LogDuration(const std::string& id, std::ostream& out)
        : id_(id), out_(out) {
        }
        
LogDuration::~LogDuration() {
        using namespace std::chrono;
        using namespace std::literals;

        const auto end_time = Clock::now();
        const auto dur = end_time - start_time_;
        out_ << "Operation time: " << id_ << " = "s << duration_cast<milliseconds>(dur).count() << " ms"s << std::endl;
    }
