#include <cstdlib>
#include <csignal>
#include <cstdio>
#include <dotenv.h>
#include <fstream>
#include <format>
#include <string>
#include <tgbot/Bot.h>
#include <tgbot/net/HttpClient.h>
#include <tgbot/tgbot.h>

#include "../include/daily_scheduler.h"
#include "../include/utils.h"

static void load_dotenv() {
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
        auto user_id = message->chat->id;
        auto user_name = message->chat->username;
        std::string msg = std::format("Hello, {}({}) !", user_name, user_id); 
        bot.getApi().sendMessage(user_id, msg);
    });

    bot.getEvents().onCommand("maa_once", [&bot](TgBot::Message::Ptr) {
        Utils::make_run_maa(bot)();
    });
}

void register_daily_scheduler(TgBot::Bot& bot) {
    static auto scheduler = DailyScheduler();
    const auto run_maa = Utils::make_run_maa(bot);

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
            // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    } catch (std::exception& e) {
        printf("error: %s\n", e.what());
    }
}