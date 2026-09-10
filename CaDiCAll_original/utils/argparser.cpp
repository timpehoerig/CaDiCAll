#include <map>
#include <string>
#include "argparser.hpp"


args_map parseArgs(int argc, char* argv[]) {
    args_map args;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg.size() > 0 && arg[0] == '-') {
            // remove all "-"
            while(arg.size() > 0 && arg[0] == '-') {
                arg = arg.substr(1);
            }
            if (i + 1 < argc && std::string(argv[i + 1]).rfind("-", 0) != 0) {
                args[arg] = argv[++i];
            } else {
                args[arg] = "true";
            }
        }
    }

    return args;
}
