#ifndef STRINGS
#define STRINGS

#include <string>
#include "types.hpp"

std::string to_string(ivec model, bool neg = false);
std::string to_string(ivvec model, bool neg = false);
std::string to_string(const ivec &stack, const ivec &values, const ivec &dls, const bvec &is_ds);
std::string to_string(const ivec &decisions, const ivec &decisions_count, int dl);

#endif
