#include <vector>
#include <string>
#include <charconv>
#include "strings.hpp"


// to_string(model) == "x y -z"
std::string to_string(ivec model, bool neg) {
    if (model.empty()) return "";

    std::string str;
    str.reserve(model.size() * 12);

    char buffer[32];

    int lit = model[0];
    if (neg) lit = -lit;
    auto [ptr, ec] = std::to_chars(buffer, buffer + 32, lit);
    str.append(buffer, ptr);

    for (size_t i = 1; i < model.size(); i++) {
        str.push_back(' ');
        int lit = model[i];
        if (neg) lit = -lit;
        auto [ptr, ec] = std::to_chars(buffer, buffer + 32, model[i]);
        str.append(buffer, ptr);
    }

    return str;
}


std::string to_string(std::vector<size_t> model) {
    if (model.empty()) return "";

    std::string str;
    str.reserve(model.size() * 12);

    char buffer[32];

    size_t lit = model[0];
    auto [ptr, ec] = std::to_chars(buffer, buffer + 32, lit);
    str.append(buffer, ptr);

    for (size_t i = 1; i < model.size(); i++) {
        str.push_back(' ');
        auto [ptr, ec] = std::to_chars(buffer, buffer + 32, model[i]);
        str.append(buffer, ptr);
    }

    return str;
}

// to_string(models) == "model1 | model2"
std::string to_string(ivvec models, bool neg) {

    std::string str;

    for (ivec model : models) {
        str += "c " + to_string(model, neg) + "\n";
    }

    if (str.size() > 2 && str[str.size()-2] == '|') {
        str.resize(str.size()-2);
    }

    return str;
}

// to_string(values, dls, is_ds) == ..."
std::string to_string(ivec decisions, ivec reason_to_dl, size_t dl) {
    size_t l = 0;
    std::string top = "c l | " + std::to_string(l);
    std::string mid = "c d | ";
    std::string bot = "c r | ";
    for (size_t i = 0; i < reason_to_dl.size(); i++) {
        if (reason_to_dl[i] != 0) continue;
        bot += std::to_string(i) + " ";
    }

    size_t m = std::max(top.size(), std::max(mid.size(), bot.size()));
    while (top.size() < m) top += " ";
    while (mid.size() < m) mid += " ";
    while (bot.size() < m) bot += " ";

    for (int d : decisions) {
        if (d == 0) continue;
        l++;
        top += " | " + std::to_string(l) + ((l == dl) ? "*" : " ");
        mid += " | " + std::to_string(d) + " ";
        bot += " | ";
        for (size_t i = 0; i < reason_to_dl.size(); i++) {
            if (reason_to_dl[i] == (int)l) {
                bot += std::to_string(i) + " ";
            }
        }

        m = std::max(top.size(), std::max(mid.size(), bot.size()));
        while (top.size() < m) top += " ";
        while (mid.size() < m) mid += " ";
        while (bot.size() < m) bot += " ";
    }

    if (decisions.size() <= dl) {
        top += " | " + std::to_string(dl) + "*";
        mid += " | ";
        bot += " | ";
    }

    m = std::max(top.size(), std::max(mid.size(), bot.size()));
    while (top.size() < m) top += " ";
    while (mid.size() < m) mid += " ";
    while (bot.size() < m) bot += " ";

    top += " |";
    mid += " |";
    bot += " |";

    return top + "\n" + mid + "\n" + bot;
}