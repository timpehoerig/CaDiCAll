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
std::string to_string(const ivec &stack, const ivec &values, const ivec &dls, const bvec &is_ds) {
    std::string top = "c lvl (" + std::to_string(dls[0]) + ")";
    std::string mid = "c val ( " + std::to_string(values[0]) + ")";
    std::string bot = "c why ( " + std::string(is_ds[0] ? "d" : "f") + ")";

    int old_l = -1;
    for (int var : stack) {
        int v = values[var];
        int l = dls[var];
        bool d = is_ds[var];

        if (l > old_l) {
            top += " | " + std::to_string(l) + " ";
            mid += " | " + std::to_string(v * var) + " ";
            bot += " | " + std::string((d ? "d" : "f")) + " ";
            old_l = l;
        } else {
            mid += std::to_string(v * var) + " ";
            bot += std::string((d ? "d" : "f")) + " ";
        }
        size_t m = std::max(top.size(), std::max(mid.size(), bot.size()));
        while (top.size() < m) top += " ";
        while (mid.size() < m) mid += " ";
        while (bot.size() < m) bot += " ";
    }
    top += " |";
    mid += " |";
    bot += " |";

    return top + "\n" + mid + "\n" + bot + "\n";
}

std::string to_string(const ivec &decisions, const ivec &decisions_count, int dl) {
    std::string top = "c dl (0)";
    std::string mid = "c d  (" + std::to_string(decisions[0]) + ")";
    std::string bot = "c dc (" + std::to_string(decisions_count[0]) + ")";

    for (int i = 1; i < (int)decisions.size() || i < (int)decisions_count.size(); i++) {

        top += " | " + std::to_string(i) + " ";
        if (i < (int)decisions.size()) mid += " | " + std::to_string(decisions[i]) + " ";
        if (i < (int)decisions_count.size()) bot += " | " + std::to_string(decisions_count[i]) + " ";

        size_t m = std::max(top.size(), std::max(mid.size(), bot.size()));
        while (top.size() < m) top += " ";
        while (mid.size() < m) mid += " ";
        while (bot.size() < m) bot += " ";
    }
    top += " |";
    mid += " |";
    bot += " |";

    return top + "\n" + mid + "\n" + bot + "\n";
}
