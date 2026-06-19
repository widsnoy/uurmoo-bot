#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <queue>
#include <string>

struct MsgNode {
    std::int64_t chat_id;
    std::string msg;
    std::optional<std::string> photo_path;
};

class MsgQueue {
public:
    auto pop() -> std::optional<MsgNode>;
    auto push(MsgNode &&n) -> void;

private:
    std::mutex lock_;
    std::queue<MsgNode> q_;
};
