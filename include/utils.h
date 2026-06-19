#pragma once

#include "message_queue.h"
#include "tgbot/types/Message.h"
#include <functional>
#include <tgbot/Api.h>
#include <tgbot/tgbot.h>

class Utils {
public:
    static std::function<void()> make_run_maa(MsgQueue&);
    static bool send_from_me(TgBot::Message::Ptr);
};
