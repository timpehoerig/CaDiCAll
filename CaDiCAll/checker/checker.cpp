#include <vector>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <set>
#include <fstream>
#include <cstdio>
#include <cmath>

#include "../utils/strings.hpp"
#include "../utils/parser.hpp"
#include "../../cadical/src/cadical.hpp"
#include "../utils/argparser.hpp"


using tclause = std::vector<int>;
using tcnf = std::vector<tclause>;


// global variables
bool HELP = false;
bool SHRINK = false;

// check sat:clause by index
bool sat_clause(tclause clause, tclause model) {
    for (int l : clause) {
        if (model[std::abs(l)-1] == l) return true;
    }
    return false;
}

bool sat(tcnf cnf, tclause model) {
    for (tclause c : cnf) {
        if (!sat_clause(c, model)) return false;
    }
    return true;
}


std::set<int> to_set(tclause model) {
    std::set<int> out;
    for (int l : model) {
        out.insert(l);
    }
    return out;
}


// check that no clause in cnf occurs twice
bool unique_models(tcnf all_models) {
    bool unique = true;
    std::set<std::set<int>> seen_models;
    for (tclause model : all_models) {
        std::set<int> model_as_set = to_set(model);

        if (model.size() != model_as_set.size()) {
            std::cout << "The following model contains one literal multiple times: ";
            std::cout << to_string(model);
        }

        if (seen_models.count(model_as_set)) {
            std::cout << "The following model occurs twice: ";
            std::cout << to_string(model);
            unique = false;
        }

        seen_models.insert(model_as_set);
    }

    return unique;
}


// chcks if c1 subsumes c2
bool subsumes(tclause c1, tclause c2) {
    // >= beacause if == 'unique_models' fails
    if (c1.size() >= c2.size()) return false;

    size_t i1 = 0;
    size_t i2 = 0;

    while (i1 < c1.size()) {
        int l1 = c1[i1];
        int l2 = c2[i2];
        
        if (l1 == l2) {
            i1++;
            i2++;
            continue;
        }
        // l and -l
        if (abs(l1) == abs(l2)) return false;
        int v1 = abs(l1);
        int v2 = abs(l2);
        if (v1 < v2) i1++;
        if (v1 > v2) i2++; 
    }
    return true;
}


// checks if any clause in cnf subsumes any other (only comparse when necessary)
bool check_subsuming(tcnf cnf) {
    bool subsuming = false;
    size_t i1 = 0;
    size_t i2 = 0;
    while (i1 < cnf.size()) {
        i2 = 0;
        while (i2 < cnf.size()) {
            if (i2 <= i1) {
                i2++;
                continue;
            };
            // std::cout << "i1:" << i1 << " i2:" << i2 << "\n";
            if (subsumes(cnf[i1], cnf[i2])) {
                std::cout << "c model " << to_string(cnf[i1]) << " subsumes " << to_string(cnf[i2]) << "\n";
                subsuming = true;
            }
            if (subsumes(cnf[i2], cnf[i1])) {
                std::cout << "c model " << to_string(cnf[i2]) << " subsumes " << to_string(cnf[i1]) << "\n";
                subsuming = true;
            }
            i2++;
        }
        i1++;
    }
    return subsuming;
}


bool check_unique_minimized_model(tclause m1, tclause m2) {
    size_t i1 = 0;
    size_t i2 = 0;
    bool unique = false;
    while (i1 < m1.size()) {
        if (i2 >= m2.size()) return unique;
        if (abs(m1[i1]) < abs(m2[i2])) i1++;
        if (abs(m1[i1]) > abs(m2[i2])) i2++;
        if (m1[i1] == -m2[i2]) return true;
        if (m1[i1] == m2[i2]) {
            i1++;
            i2++;
        }
    }
    return unique;
}


bool check_unique_minimized_models(tcnf models) {
    bool unique = true;
    size_t i1 = 0;
    size_t i2 = 0;
    while (i1 < models.size()) {
        i2 = 0;
        while (i2 < models.size()) {
            if (i2 <= i1) {
                i2++;
                continue;
            };
            if (!check_unique_minimized_model(models[i1], models[i2])) {
                unique = false;
                std::cout << "c not unique: " << to_string(models[i1]) << " & " << to_string(models[i2]) << "\n";
            }
            i2++;
        }
        i1++;
    }
    return unique;
}



#include <vector>
#include <unordered_map>

using Model = std::vector<int>;

bool models_overlap(const Model& m1, const Model& m2) {
    std::unordered_map<int, int> assignment;

    // store assignments from m1
    for (int lit : m1) {
        int var = std::abs(lit);
        assignment[var] = lit;
    }

    // check m2 against m1
    for (int lit : m2) {
        int var = std::abs(lit);

        if (assignment.count(var)) {
            if (assignment[var] == -lit) {
                return false; // conflict → no overlap
            }
        }
    }

    return true; // no conflicts → overlap
}


bool all_models_disjoint(const std::vector<Model>& models) {
    for (size_t i = 0; i < models.size(); i++) {
        for (size_t j = i + 1; j < models.size(); j++) {
            if (models_overlap(models[i], models[j])) {
                return false; // found overlap → bad
            }
        }
    }
    return true;
}



void sort(tcnf& cnf) {
    for (tclause& model : cnf) std::sort(model.begin(), model.end(), [](int x, int y){return std::abs(x) < std::abs(y);});
    std::sort(cnf.begin(), cnf.end());
}


std::string run_dualiza_with_units(const std::string& input_cnf_path,
                                   const std::vector<int>& units) {
    std::ifstream in(input_cnf_path);
    if (!in) {
        throw std::runtime_error("Could not open input CNF file");
    }

    std::string line;
    std::stringstream cnf_body;
    int vars = 0, clauses = 0;

    // Parse DIMACS
    while (std::getline(in, line)) {
        if (line.empty()) continue;

        if (line[0] == 'c') {
            continue; // skip comments
        } else if (line[0] == 'p') {
            std::stringstream ss(line);
            std::string tmp;
            ss >> tmp >> tmp >> vars >> clauses;
        } else {
            cnf_body << line << "\n";
        }
    }
    in.close();

    // New file path
    std::string new_path = input_cnf_path + ".tmp.cnf";

    std::ofstream out(new_path);
    if (!out) {
        throw std::runtime_error("Could not open output CNF file");
    }

    // Write header (update clause count)
    int new_clauses = clauses + units.size();
    out << "p cnf " << vars << " " << new_clauses << "\n";

    // Write original clauses
    out << cnf_body.str();

    // Add unit clauses
    for (int lit : units) {
        out << lit << " 0\n";
    }

    out.close();

    // Run dualiza
    std::string command = "../../dualiza/dualiza " + new_path;

    std::string result;
    char buffer[256];

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("Failed to run dualiza");
    }

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

    pclose(pipe);

    return result;
}


int extract_number_of_models(const std::string& output) {
    std::stringstream ss(output);
    std::string token;
    int number = -1;

    while (ss >> token) {
        try {
            number = std::stoi(token);
        } catch (...) {
            // ignore non-integer tokens
        }
    }

    return number; // last integer found
}


void arg_parser(int argc, char* argv[], bool& shrink, bool& help) {
    std::map<std::string, std::string> parsedArgs = parseArgs(argc, argv);
    shrink = parsedArgs.count("shrink") || parsedArgs.count("s");
    help = parsedArgs.count("help") || parsedArgs.count("h");
}

int main(int argc, char* argv[]) {

    arg_parser(argc, argv, SHRINK, HELP);

    if (HELP) {
        std::string msg =
            "\n\n"
            "Welcome to the Checker for CaDiCAll using IPASIR-UP\n"
            "\n"
            "USAGE:\n"
            "\tchecker [--Option (-o)] <cnf> <CaDiCAll_out>\n"
            "\n"
            "OPTIONS:\n"
            "\n"
            "\t-s --shrink \t Allows models to be shrunken by implicant shrinking\n"
            "\t-h --help \t Show this menu\n";
        std::cout << msg;
        return 0;
    }

    bool verified = true;

    // parse cnf and store it in 'cnf'
    tcnf cnf;
    size_t size = parse_cnf(argv[argc - 2], cnf);
    tclause projected_vars;
    get_projected_vars(argv[argc - 2], projected_vars);
    sort(cnf);

    // parse CaDiCAll_terminal_out and store it in 'CaDiCAll_terminal_out'
    tcnf CaDiCAll_terminal_out;
    parse_icnf(argv[argc - 1], CaDiCAll_terminal_out);
    sort(CaDiCAll_terminal_out);

    std::cout << "c cnf:\nc ";
    std::cout << to_string(cnf) + "\n";
    std::cout << "c CaDiCAll_terminal_out:\nc ";
    std::cout << to_string(CaDiCAll_terminal_out) + "\n";

    // exit if cnf has no models
    if (CaDiCAll_terminal_out.size() == 0) {
        std::cout << "c cnf has no models\n";
        std::cout << "s VERIFIED\n";
        exit(1);
    }

    // check that no models occur twice
    if (!unique_models(CaDiCAll_terminal_out)) {
        std::cout << "c MODELS ARE NOT UNIQUE\n";
        verified = false;
    } else {
        std::cout << "c MODELS ARE UNIQUE\n";
    }

    // check that models differ in at least one literal
    if (SHRINK) {
        if (check_unique_minimized_models(CaDiCAll_terminal_out)) {
            std::cout << "c unique minimized models\n";
        } else {
            std::cout << "c minimized models are not unique\n";
            verified = false;
        }

        // exponential check
        if (false && !all_models_disjoint(CaDiCAll_terminal_out)) {
            std::cout << "c models are not disjoint\n";
            verified = false;
        }
    }

    // minimized models are not total
    if (!SHRINK) {
        // check that models are total
        bool total = true;
        if (projected_vars.size()) size = projected_vars.size();
        for (tclause model : CaDiCAll_terminal_out) {
            if (model.size() != size) {
                std::cout << "c " << to_string(model) << " is not total\n";
                verified = false;
                total = false;
            }
        }
        if (total) std::cout << "c all models are total\n";
    }

    // if not projected
    if (projected_vars.size() == 0 && !SHRINK) {
        std::cout << "c No projected variables found, running in NORMALE MODE\n";
        // check that all found models are actually models
        for (tclause model : CaDiCAll_terminal_out) {
            if (!sat(cnf, model)) {
                std::cout << "c NOT A MODEL: " << to_string(model) << "\n";
                verified = false;
            }
        }

    // if projected
    } else {
        if (true) {
            for (tclause model : CaDiCAll_terminal_out) {
                int dualiza_count = extract_number_of_models(run_dualiza_with_units(argv[argc - 2], model));
                int model_size_count = pow(2, size - model.size());
                if (dualiza_count != model_size_count) {
                    std::cout << "Shrinking incorrect - dualiza: " << std::to_string(dualiza_count) << " CaDiCAll: " << std::to_string(model_size_count) << " model size: " << std::to_string(model.size()) << std::endl;
                    std::cout << to_string(model) << std::endl;
                    verified = false;
                    std::cout << "s PROBLEM" << std::endl;
                    return 1;
                }
            }
        } else {
            std::cout << "c shrunken models found, running in SHRUNKEN MODE\n"; //Projected variables found, running in PROJECTED MODE\n";
            CaDiCaL::Solver *solver = new CaDiCaL::Solver;
            int numVariables;
            solver->read_dimacs(argv[argc - 2], numVariables);
            for (tclause model : CaDiCAll_terminal_out) {
                // projected case
                for (int lit : model) {
                    solver->assume(lit);
                }
                int res = solver->solve();
                std::cout << "c cnf and " << to_string(model) << " is ";
                if (res == 10) {
                    std::cout << "SAT\n";
                } else {
                    std::cout << "UNSAT\n";
                    verified = false;
                }
            }
        }
    }

    if (verified) {
        std::cout << "s VERIFIED\n";
    } else {
        std::cout << "s NOT VERIFIED\n";
        return 1;
    }

    return 0;
}
