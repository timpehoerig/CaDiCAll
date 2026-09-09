#ifndef PARSER
#define PARSER

#include "types.hpp"

bool get_projected_vars(char* path, tclause& vars);

int parse_cnf(char* path, tcnf& cnf);

void parse_icnf(char* path, tcnf& cnf, bool q = false);

#endif
