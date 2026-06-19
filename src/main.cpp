#include <cstdlib>
#include <csignal>
#include <cstdio>
#include <dotenv.h>
#include <fstream>
#include <format>
#include <optional>
#include <string>
#include <thread>
#include <tgbot/Bot.h>
#include <tgbot/net/HttpClient.h>
#include <tgbot/tgbot.h>
#include <tgbot/types/InputFile.h>

#include "../include/daily_scheduler.h"
#include "../include/utils.h"
#include "message_queue.h"

static MsgQueue msg_queue;

void load_dotenv() {
    if (std::ifstream(".env")) {
        dotenv::init(".env");
        return;
    }
    if (std::ifstream("../.env")) {
        dotenv::init("../.env");
    }
}

void register_commands(TgBot::Bot &bot) {
    bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message) {
        const auto user_id = message->chat->id;
        const std::string name = message->chat->username.empty()
            ? std::format("用户 {}", user_id)
            : message->chat->username;

        const std::string msg = std::format(
            "你好，{}！\n\n"
            "我是 uurmoo，用来远程管理 Redroid 上的 MAA 日常任务。\n\n"
            "可用命令：\n"
            "/start — 显示本介绍\n"
            "/maa_once — 立即执行一次 MAA 日常（仅主人）\n"
            "/redroid_screenshot — 截取 Redroid 当前画面（仅主人）\n\n"
            "定时任务（上海时区）：每天 04:10、12:10、16:10、20:10 自动执行 MAA，完成后推送结果。",
            name
        );
        bot.getApi().sendMessage(user_id, msg);
    });

    bot.getEvents().onCommand("maa_once", [&bot](TgBot::Message::Ptr message) {
        if (!Utils::send_from_me(message)) {
            bot.getApi().sendMessage(message->chat->id, "You cannot use this command!!!");
            return;
        }
        auto thread = std::thread(Utils::make_run_maa(msg_queue));
        thread.detach();
    });

    bot.getEvents().onCommand("redroid_screenshot", [&bot](TgBot::Message::Ptr message) {
        if (!Utils::send_from_me(message)) {
            bot.getApi().sendMessage(message->chat->id, "You cannot use this command!!!");
            return;
        }
        auto thread = std::thread(Utils::make_run_screenshot(msg_queue, message->chat->id));
        thread.detach();
    });
}

void register_daily_scheduler(TgBot::Bot& bot) {
    static auto scheduler = DailyScheduler();
    const auto run_maa = Utils::make_run_maa(msg_queue);

    // 04:10 日刷新后：基建换班、清体力、公招
    scheduler.add({4, 10, TimeZone::Shanghai}, run_maa);
    // 12:10 八小时基建轮次、清体力
    scheduler.add({12, 10, TimeZone::Shanghai}, run_maa);
    // 16:10 信用商店 16:00 刷新后购物
    scheduler.add({16, 10, TimeZone::Shanghai}, run_maa);
    // 20:10 晚间换班、清体力
    scheduler.add({20, 10, TimeZone::Shanghai}, run_maa);

    scheduler.start();
}

int main() {
    load_dotenv();

    const std::string token = dotenv::getenv("uurmoo_token");
    const std::string http_proxy = dotenv::getenv("http_proxy");

    if (token.empty()) {
        fprintf(stderr, "uurmoo_token is not set (use .env or environment)\n");
        return 1;
    }

    TgBot::CurlHttpClient http_client;
    http_client.setProxy(http_proxy.c_str());

    auto bot = TgBot::Bot(token, http_client);

    register_commands(bot);
    register_daily_scheduler(bot);

    signal(SIGINT, [](int s) {
        printf("SIGINT got\n");
        exit(0);
    });

    try {
        printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
        bot.getApi().deleteWebhook();

        TgBot::TgLongPoll longPoll(bot);
        while (true) {
            longPoll.start();
            std::optional<MsgNode> msg_opt = msg_queue.pop();
            while (msg_opt != std::nullopt) {
               MsgNode msg = msg_opt.value();
               if (msg.photo_path) {
                   bot.getApi().sendPhoto(
                       msg.chat_id,
                       TgBot::InputFile::fromFile(*msg.photo_path, "image/jpeg")
                   );
               } else {
                   bot.getApi().sendMessage(msg.chat_id, msg.msg);
               }
               msg_opt = msg_queue.pop();
            }
        }
    } catch (std::exception& e) {
        printf("error: %s\n", e.what());
    }
}