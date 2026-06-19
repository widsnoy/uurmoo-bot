#include "message_queue.h"

auto MsgQueue::pop() -> std::optional<MsgNode> {
    auto _ = std::lock_guard(lock_);
    std::optional<MsgNode> res = std::nullopt;
    if (!q_.empty()) {
        res = std::optional<MsgNode>{q_.front()};
        q_.pop();
    }
    return res;
}

auto MsgQueue::push(MsgNode &&n) -> void {
    auto _ = std::lock_guard(lock_);
    q_.push(std::move(n));
}
