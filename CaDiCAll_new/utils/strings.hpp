#ifndef STRINGS
#define STRINGS

#include <string>
#include "types.hpp"

std::string to_string(ivec model, bool neg = false);
std::string to_string(ivvec model, bool neg = false);
std::string to_string(ivec decisions, ivec reason_to_dl, size_t dl);

#endif
