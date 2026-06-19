#include <dotenv.h>
#include <functional>
#include <iterator>
#include <string>
#include <boost/process/v1.hpp>
#include <tgbot/Bot.h>
#include <tgbot/tgbot.h>

#include "utils.h"
#include "message_queue.h"

namespace bp = boost::process::v1;

static constexpr const char* RUN_DAILY_SCRIPT = "scripts/run-daily.fish";
static constexpr const char* SCREENSHOT_SCRIPT = "scripts/redroid_screenshot.sh";

static std::string read_stream(bp::ipstream& stream) {
    return {std::istreambuf_iterator<char>(stream), {}};
}

static std::string trim_trailing_newline(std::string s) {
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) {
        s.pop_back();
    }
    return s;
}

std::function<void()> Utils::make_run_maa(MsgQueue &queue) {
    auto chat_id = std::stol(dotenv::getenv("my_userid"));
    return [&queue, chat_id] {
        bp::ipstream out;
        bp::ipstream err;

        bp::child process(
            "/usr/bin/flock", "-n", "/tmp/maa-daily.lock",
            "/usr/bin/fish", RUN_DAILY_SCRIPT,
            bp::std_out > out,
            bp::std_err > err
        );

        process.wait();

        const int exit_code = process.exit_code();
        const std::string out_text = read_stream(out);
        const std::string err_text = read_stream(err);

        std::string res;
        if (exit_code == 0) {
            res = std::format("maa ok: {}\n", out_text.c_str());
        } else {
            res = std::format("maa fail ({}): {}\n", exit_code, err_text.c_str());
        }
        queue.push(MsgNode {.chat_id = chat_id, .msg = std::move(res)});
    };
}

std::function<void()> Utils::make_run_screenshot(MsgQueue &queue, std::int64_t chat_id) {
    return [&queue, chat_id] {
        bp::ipstream out;
        bp::ipstream err;

        bp::child process(
            "/usr/bin/bash", SCREENSHOT_SCRIPT,
            bp::std_out > out,
            bp::std_err > err
        );

        process.wait();

        const int exit_code = process.exit_code();
        if (exit_code == 0) {
            queue.push(MsgNode {
                .chat_id = chat_id,
                .photo_path = trim_trailing_newline(read_stream(out)),
            });
            return;
        }

        const std::string err_text = read_stream(err);
        queue.push(MsgNode {
            .chat_id = chat_id,
            .msg = std::format("screenshot fail ({}): {}", exit_code, err_text),
        });
    };
}

bool Utils::send_from_me(TgBot::Message::Ptr msg) {
    auto chat_id = std::stol(dotenv::getenv("my_userid"));
    return chat_id == msg->chat->id;
}