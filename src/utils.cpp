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

static std::string read_stream(bp::ipstream& stream) {
    return {std::istreambuf_iterator<char>(stream), {}};
}

std::function<void()> Utils::make_run_maa(MsgQueue &queue) {
    auto chat_id = std::stol(dotenv::getenv("my_userid"));
    return [&queue, chat_id] {
        bp::ipstream out;
        bp::ipstream err;

        bp::child process(
            "/usr/bin/flock", "-n", "/tmp/maa-daily.lock",
            "/usr/bin/fish", "/home/widsnoy/.config/maa/scripts/run-daily.fish",
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

bool Utils::send_from_me(TgBot::Message::Ptr msg) {
    auto chat_id = std::stol(dotenv::getenv("my_userid"));
    return chat_id == msg->chat->id;
}