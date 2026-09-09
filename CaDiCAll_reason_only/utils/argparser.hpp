#ifndef ARGPARSER
#define ARGPARSER

#include <map>
#include <string>

using args_map = std::map<std::string, std::string>;

args_map parseArgs(int argc, char* argv[]);

#endif
