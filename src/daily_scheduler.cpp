#include "../include/daily_scheduler.h"

#include <algorithm>
#include <stdexcept>

using namespace std::chrono;

auto to_string(TimeZone tz) -> std::string {
    switch (tz) {
        case TimeZone::Shanghai:
            return "Asia/Shanghai";
    }
    return "Unknown";
}

auto next_local_time_utc(const DailyTime& time) -> system_clock::time_point {
    const auto* tz = locate_zone(to_string(time.timezone));

    const auto utc_now = system_clock::now();
    const auto local_now = tz->to_local(utc_now);

    const auto today = floor<days>(local_now);
    auto target_local = today + hours{time.hour} + minutes{time.minute};

    if (target_local <= local_now) {
        target_local += days{1};
    }

    return tz->to_sys(target_local);
}

DailyScheduler::DailyScheduler()
    : timer_(io_) {}

DailyScheduler::~DailyScheduler() {
    io_.stop();
    if (io_thread_.joinable()) {
        io_thread_.join();
    }
}

void DailyScheduler::add(DailyTime time, TaskCallback callback) {
    if (started_) {
        throw std::logic_error("cannot add task after scheduler started");
    }

    tasks_.push_back(Task{
        .time = time,
        .callback = std::move(callback),
    });
}

void DailyScheduler::arm_next() {
    if (tasks_.empty()) {
        return;
    }

    auto wake_at = system_clock::time_point::max();
    for (auto& task : tasks_) {
        task.next_fire = next_local_time_utc(task.time);
        wake_at = std::min(wake_at, task.next_fire);
    }

    timer_.expires_at(wake_at);
    timer_.async_wait([this](const boost::system::error_code& ec) {
        on_timer(ec);
    });
}

void DailyScheduler::on_timer(const boost::system::error_code& ec) {
    if (ec) {
        return;
    }

    const auto wake_at = timer_.expiry();

    for (auto& task : tasks_) {
        if (task.next_fire == wake_at) {
            task.callback();
        }
    }

    arm_next();
}

void DailyScheduler::start() {
    if (started_) {
        return;
    }
    started_ = true;

    arm_next();

    io_thread_ = std::thread([this] {
        io_.run();
    });
}
