#pragma once

#include "message_queue.h"
#include "tgbot/types/Message.h"
#include <functional>
#include <tgbot/Api.h>
#include <tgbot/tgbot.h>

class Utils {
public:
    static std::function<void()> make_run_maa(MsgQueue&);
    static std::function<void()> make_run_screenshot(MsgQueue&, std::int64_t chat_id);
    static bool send_from_me(TgBot::Message::Ptr);
};
