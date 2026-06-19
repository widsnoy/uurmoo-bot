#pragma once

#include <boost/asio/system_timer.hpp>
#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>
#include <vector>

enum class TimeZone {
    Shanghai,
};

auto to_string(TimeZone tz) -> std::string;

struct DailyTime {
    std::uint8_t hour;
    std::uint8_t minute;
    TimeZone timezone;
};

auto next_local_time_utc(const DailyTime& time) -> std::chrono::system_clock::time_point;

class DailyScheduler {
public:
    using TaskCallback = std::function<void()>;

    DailyScheduler();
    ~DailyScheduler();

    DailyScheduler(const DailyScheduler&) = delete;
    DailyScheduler& operator=(const DailyScheduler&) = delete;

    void add(DailyTime time, TaskCallback callback);
    void start();

private:
    struct Task {
        DailyTime time;
        TaskCallback callback;
        std::chrono::system_clock::time_point next_fire{};
    };

    void arm_next();
    void on_timer(const boost::system::error_code& ec);

    boost::asio::io_context io_;
    boost::asio::system_timer timer_;
    std::vector<Task> tasks_;
    std::thread io_thread_;
    bool started_ = false;
};
