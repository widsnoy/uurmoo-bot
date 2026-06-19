#pragma once

#include <functional>
#include <tgbot/Api.h>
#include <tgbot/tgbot.h>

class Utils {
public:
    static std::function<void()> make_run_maa(TgBot::Bot& bot);
};
