#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <memory>
#include <tgbot/tgbot.h>
#include <vector>
#include <ctime>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <json/json.h>

std::vector<std::string>bot_commands = { "start", "time", "currency" };

namespace
{
    std::size_t callback(
        const char* in,
        std::size_t size,
        std::size_t num,
        std::string* out)
    {
        const std::size_t totalBytes(size * num);
        out->append(in, totalBytes);
        return totalBytes;
    }
}

std::string get_time() {
    time_t now = time(NULL);
    tm* ptr = localtime(&now);
    char buf[32];
    strftime(buf, 32, "%c", ptr);
    return buf;
}



std::string get_request(std::string link) {
    std::string data;
    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, link.c_str());// Set remote URL.    
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);// Don't bother trying IPv6, which would increase DNS resolution time.
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);// Don't wait forever, time out after 10 seconds.
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);// Follow HTTP redirects if necessary.
    long httpCode(0);// Response information.
    std::unique_ptr<std::string> httpData(new std::string());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);// Hook up data handling function.
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, httpData.get());
    curl_easy_perform(curl);// Run our HTTP GET command, capture the HTTP response code, and clean up.
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);

    Json::Value jsonData;
    Json::Reader jsonReader;

    if (jsonReader.parse(*httpData.get(), jsonData))
    {
        data = jsonData.toStyledString();
        return data;
    }
    else return "-1";
}


float get_currency(char what) {
    auto js_obj = nlohmann::json::parse(get_request("http://www.cbr-xml-daily.ru/daily_json.js"));
    if (what == 'u') {
        return js_obj["Valute"]["USD"]["Value"].get<float>();
    }
    if (what == 'e') {
        return js_obj["Valute"]["EUR"]["Value"].get<float>();
    }
    if (what == 'k') {
        return js_obj["Valute"]["CNY"]["Value"].get<float>();
    }
    return -1;
}

int main() {
    TgBot::Bot bot("5915313447:AAH_AUH6bUYBXJwp7dnKAbp6flFOuMGEeic");
    
    TgBot::InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup);
    std::vector<TgBot::InlineKeyboardButton::Ptr> buttons;
    TgBot::InlineKeyboardButton::Ptr usd_btn(new TgBot::InlineKeyboardButton),
                                     eur_btn(new TgBot::InlineKeyboardButton),
                                     uan_btn(new TgBot::InlineKeyboardButton);
    usd_btn->text = "USD";
    usd_btn->callbackData = "USD";
    eur_btn->text = "EUR";
    eur_btn->callbackData = "EUR";
    uan_btn->text = "CNY";
    uan_btn->callbackData = "CNY";
    buttons.push_back(usd_btn);
    buttons.push_back(eur_btn);
    buttons.push_back(uan_btn);
    keyboard->inlineKeyboard.push_back(buttons);

    bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id, "Hi " + message->chat->firstName);
        });

    bot.getEvents().onCommand("time", [&bot](TgBot::Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id, "Moscow time: " + get_time());
        });
    
    bot.getEvents().onCommand("currency", [&bot, &keyboard](TgBot::Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id, "How currency?", false, 0, keyboard);
        });
    
    bot.getEvents().onCallbackQuery([&bot, &keyboard](TgBot::CallbackQuery::Ptr query) {
        if (StringTools::startsWith(query->data, "USD")) {
            bot.getApi().sendMessage(query->message->chat->id, "Dollar = " + std::to_string(get_currency('u')));
        }
        if (StringTools::startsWith(query->data, "EUR")) {
            bot.getApi().sendMessage(query->message->chat->id, "Euro = " + std::to_string(get_currency('e')));
        }
        if (StringTools::startsWith(query->data, "CNY")) {
            bot.getApi().sendMessage(query->message->chat->id, "Uan = " + std::to_string(get_currency('k')));
        }
        });
    
    bot.getEvents().onAnyMessage([&bot](TgBot::Message::Ptr message) {
        printf("User wrote %s\n", message->text.c_str());
        
        for (const auto& command : bot_commands) {
            if ("/" + command == message->text) {
                return;
            }
        }
        
        bot.getApi().sendPhoto(message->chat->id, TgBot::InputFile::fromFile("../cat.png", "image"));
        bot.getApi().sendMessage(message->chat->id, "I don't understand you :( \nSorry :(");
        });
    
    
    
    try {
        printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
        TgBot::TgLongPoll longPoll(bot);
        while (true) {
            printf("Long poll started\n");
            longPoll.start();
        }
    }
    catch (TgBot::TgException& e) {
        printf("error: %s\n", e.what());
    }
    return 0;
}