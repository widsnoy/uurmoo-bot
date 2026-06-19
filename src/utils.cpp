#include <dotenv.h>
#include <functional>
#include <iterator>
#include <string>
#include <boost/process/v1.hpp>
#include <tgbot/Bot.h>
#include <tgbot/tgbot.h>

#include "utils.h"

namespace bp = boost::process::v1;

static std::string read_stream(bp::ipstream& stream) {
    return {std::istreambuf_iterator<char>(stream), {}};
}

std::function<void()> Utils::make_run_maa(TgBot::Bot& bot) {
    return [&bot] {
        const auto chat_id = std::stoll(dotenv::getenv("my_userid"));
        bp::ipstream out;
        bp::ipstream err;

        bp::child process(
            "/usr/bin/flock", "-n", "/tmp/maa-daily.lock",
            "/usr/bin/fish", "/home/widsnoy/.config/maa/scripts/run-daily.fish",
            bp::std_out > out,
            bp::std_err > err
        );

        bot.getApi().sendMessage(chat_id, "maa start!");

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

        bot.getApi().sendMessage(chat_id, res);
    };
}
