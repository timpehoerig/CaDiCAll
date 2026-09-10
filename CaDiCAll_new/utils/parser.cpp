#include "parser.hpp"
#include <fstream>
#include <iostream>
#include <regex>


bool get_projected_vars(char* path, tclause& vars) {
    std::ifstream file(path);

    if (!file) {
        std::cerr << "Failed to open cnf\n";
    }

    std::string fst_line;
    std::getline(file, fst_line);

    std::regex pattern(R"(c\s*(-?\d+)(,-?\d+)*)");

    if (!std::regex_match(fst_line, pattern)) {
        return false;
    }

    fst_line = fst_line.substr(2);

    std::stringstream ss(fst_line);
    std::string var;

    while (std::getline(ss, var, ',')) {
        vars.push_back(std::stoi(var));
    }
    return true;
}


int parse_cnf(char* path, tcnf& cnf) {
    int size = 0;

    std::ifstream file(path);

    if (!file) {
        std::cerr << "Failed to open cnf: " << path;
        exit(1);
    }

    bool p = false;
    std::string line;

    while (std::getline(file, line)) {

        if (line.size() && line[0] == 'p') {
            std::istringstream spline(line);
            std::string s1, s2;
            spline >> s1 >> s2 >> size;
            p = true;
            continue;
        }

        if (!p) continue;

        std::istringstream sline(line);
        tclause clause;

        int lit;
        while (sline >> lit) {
            if (lit == 0) break;
            clause.push_back(lit);
        }

        cnf.push_back(clause);
    }
    return size;
}


void parse_icnf(char* path, tcnf& cnf, bool q) {
    std::ifstream file(path);

    if (!file) {
        std::cerr << "Failed to open cnf: " << path;
        exit(1);
    }

    bool is_q = !q;
    std::string line;

    while (std::getline(file, line)) {

        if (line.size() && line[0] == 'q') {
            is_q = true;
            continue;
        }

        if (!is_q) continue;

        std::istringstream sline(line);
        tclause clause;

        char a;
        sline >> a;
        if (a != 'i') continue;

        // get rid of the id
        int n;
        sline >> n;

        int lit;
        while (sline >> lit) {
            if (lit == 0) break;
            clause.push_back(-lit);
        }

        cnf.push_back(clause);
    }
}
