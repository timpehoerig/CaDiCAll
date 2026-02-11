#include "../../cadical/src/cadical.hpp"
#include "../../cadical/src/tracer.hpp"
#include "../../cadical/src/internal.hpp"
#include "../../cadical/src/external.hpp"
#include "../utils/argparser.hpp"
#include "../utils/strings.hpp"
#include "../utils/parser.hpp"
#include "../utils/types.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <unordered_set>
#include <cassert>


// global meta variables
bool COUNT = false;
bool VERBOSE = false;
bool VERBOSE_DC = false;
bool SHRINK = false;
bool PROFILE = false;
bool FIXED = false;
bool REASON = false;
bool HELP = false;

int count = 0;

// cnf must be global so EnumProp can use it
tcnf cnf;

// paths
const char* NEGATED_MODELS = "tmp_wbcp_negated_models.txt";
const char* PROOF_OUT = "tmp_wbcp_proof.txt";


int check_literal(int e_var, int b, ivec stack, ivec dls, ivec values, int i, ivec poss_in_stack, CaDiCaL::Internal *internal) {
    if (VERBOSE) std::cout << "c\nc check_literal:" << std::endl;
    START (wbc_check_literal);

    int e_lit = e_var * values[e_var];
    int i_var = internal->external->e2i[e_var];
    int i_lit = internal->external->vals[i_var] ? i_var : -i_var;
    if (VERBOSE) std::cout << "c checking literal: " + std::to_string(e_lit) << " internal: " << std::to_string(i_lit) << std::endl;
    if (VERBOSE) std::cout << "c watched clauses:" << std::endl;
    for (auto watch : internal->watches(i_lit)) {
        if (VERBOSE) {
            std::cout << "c ";
            for (int i = 0; i < watch.size; i++) {
                std::cout << std::to_string(watch.clause->literals[i]) << " ";
            }
        }
        int i_wl1 = watch.clause->literals[0];
        int i_wl2 = watch.clause->literals[1];

        // other is now the other watched literal
        int i_other_lit = i_lit == i_wl1 ? i_wl2 : i_wl1;
        int e_other_lit = internal->externalize(i_other_lit);
        int e_other_var = std::abs(e_other_lit);

        if (VERBOSE) std::cout << "also by: " << std::to_string(e_other_lit) << " internal: " << std::to_string(i_other_lit) << std::endl;
        // if (!(std::count(Trail.begin(), Trail.end(), e_other_var) > 0 && values[e_other_var] * e_other_lit > 0)) {
        // change search for index comparison (own map lit -> index)
        // assert(!(std::count(stack.begin(), stack.begin() + i, e_other_var) > 0 && values[e_other_var] * e_other_lit > 0) == !(poss_in_stack[e_other_var] < poss_in_stack[e_var] && values[e_other_var] * e_other_lit > 0));
        //if (!(std::count(stack.begin(), stack.begin() + i, e_other_var) > 0 && values[e_other_var] * e_other_lit > 0)) {
        if (!(poss_in_stack[e_other_var] < poss_in_stack[e_var] && values[e_other_var] * e_other_lit > 0)) { 
            if (VERBOSE) std::cout << "c b = max(" << std::to_string(b) << "," << std::to_string(dls[e_var]) << ")" << std::endl;
            b = std::max(b, dls[e_var]);
        }
    }
    STOP(wbc_check_literal);
    if (VERBOSE) std::cout << "c returning b = " << std::to_string(b) << std::endl;
    return b;
}


int implicant_shrinking(ivec stack, bvec is_ds, ivec dls, ivec values, ivec dcpl, ivec poss_in_stack, CaDiCaL::Internal *internal) {
    if (VERBOSE) std::cout << "c\nc implicant_shrinking:" << std::endl;
    START (wbc_implicant_shrinking);

    int b = 0;
    int index = stack.size() - 1;
    while (index >= 0) {
        int v = stack[index];
        // (is_ds[v] && dcpl[dls[v] - 1] > 1) == values[v] < 0
        if (!is_ds[v] || (is_ds[v] && dcpl[dls[v]] != 1)) {
            b = std::max(b, dls[v]);
            if (VERBOSE) std::cout << "c " << std::to_string(v * values[v]) << " is not a decision -> b = max(" << std::to_string(b) << "," << std::to_string(dls[v]) << ")" << std::endl;
        } else if (dls[v] > b) {
            b = check_literal(v, b, stack, dls, values, index, poss_in_stack, internal);
        } else if (dls[v] == 0 || dls[v] == b) {
            if (VERBOSE) std::cout << "c dl of " << std::to_string(v * values[v]) << " is " << std::to_string(dls[v]) << "(0 or b)" << std::endl;
            break;
        }
        index--;
    }
    STOP(wbc_implicant_shrinking);
    return b;
}


void assign_or_append(ivec &vec, int val, int dl) {
    if (VERBOSE) std::cout << "c\nc assign_or_append: " << val << " to " << to_string(vec) << std::endl;

    if (dl < (int)vec.size()) {
        vec[dl] = val;
    } else {
        vec.push_back(val);
    }
}


void increase_or_append(ivec &vec, int dl) {
    if (VERBOSE) std::cout << "c\nc increase_or_append: " << to_string(vec) << std::endl;

    if (dl < (int)vec.size()) {
        vec[dl]++;
    } else {
        vec.push_back(1);
    }
}


void print_all(ivec stack, ivec values, ivec dls, bvec is_ds, ivec decisions, ivec decision_counts_per_level, int dl) {
    std::cout << "c\nc stack:" << std::endl;
    std::cout << to_string(stack, values, dls, is_ds);
    std::cout << "c\nc decision counts:" << std::endl;
    std::cout << to_string(decisions, decision_counts_per_level, dl);
    std::cout << "c" << std::endl;
}


void print_dc(ivec decisions, ivec decision_counts_per_level, int dl) {
    std::cout << "\033[H\033[J";
    std::cout << to_string(decisions, decision_counts_per_level, dl);
}


class EnumProp : public CaDiCaL::ExternalPropagator, public CaDiCaL::InternalTracer {

    public:

        CaDiCaL::Solver *solver;
        CaDiCaL::Internal *internal;

        // stack of assigned variables
        ivec stack;

        // arrays where index == var
        ivec dls;
        bvec is_ds;
        ivec values;

        // if a variable is assigned, this maps to its position on the stack, else its -1
        ivec poss_in_stack;

        // decision level
        int dl = 0;

        // biggest variable (inclusive)
        int max_var;

        // flag if solver is doing stuff that is redundant and needs to be undone (with a backtrack)
        bool false_backtrack = false;

        // if we backtrack in cb_decide, the solver ignores the decision and we need to save it
        bool save_decision = false;
        // the saved decision
        int saved_decision = 0;

        // counting decisions on levels
        ivec decision_counts_per_level;
        // decisions per level == decisions[dl]
        ivec decisions;
        bool is_decision = false;

        bool add_reason_clause = false;
        tmodels reason_clauses;

        tmodels all_models;

        // terminating solver is a problem
        bool shall_terminate = false;

        void push(int lit, int dl, bool is_decision) {
            if (VERBOSE) std::cout << "c push: " << lit << std::endl;
            START (wbc_push);

            int var = std::abs(lit);

            stack.push_back(var);

            values[var] = (lit > 0) ? 1 : -1;
            dls[var] = dl;
            is_ds[var] = is_decision;

            poss_in_stack[var] = stack.size() - 1;
            STOP(wbc_push);
        };

        std::tuple<int, int, bool> pop() {
            if (VERBOSE) std::cout << "c pop:" << std::endl;
            START (wbc_pop);

            int var = stack.back();
            stack.pop_back();

            int val = values[var];
            int level = dls[var];
            bool is_decision = is_ds[var];

            values[var] = 0;
            dls[var] = -1;
            is_ds[var] = false;

            poss_in_stack[var] = -1;

            STOP(wbc_pop);
            return {val, level, is_decision};
        }

        int highest_dl_to_flip() {
            if (VERBOSE) std::cout << "c\nc highest_dl_to_flip:" << std::endl;
            START (wbc_highest_dl_to_flip);

            int highest_dl = -1;
            for (int i = dl; i > 0; i--) {
                if (decision_counts_per_level[i] < 2) {
                    highest_dl = i;
                    break;
                }
            }

            if (VERBOSE) std::cout << "c highest dl with decision count < 2 : " + std::to_string(highest_dl) << std::endl;

            STOP(wbc_highest_dl_to_flip);
            return highest_dl;
        };

        bool cb_check_found_model (const tmodel &model) override {
            if (VERBOSE) std::cout << "c\nc cb_check_found_model:" << std::endl;

            if (shall_terminate) {
                if (VERBOSE) std::cout << "c terminating because shall_terminate is true" << std::endl;
                return false;
            }

            START (wbc_cb_check_found_model);

            if (decision_counts_per_level.back() > 2) {
                false_backtrack = true;
                if (VERBOSE) std::cout << "c decision count exceeded 2, false_backtrack = true" << std::endl;
            }

            int b = dl;
            bool found_model = false;
            if (!false_backtrack) {
                if (SHRINK) b = implicant_shrinking(stack, is_ds, dls, values, decision_counts_per_level, poss_in_stack, internal);
                if (COUNT) {
                    count++;
                } else {
                    tmodel new_model;
                    for (int lit : model) {
                        if (dls[std::abs(lit)] <= b) new_model.push_back(lit);
                    }
                    all_models.push_back(new_model);
                    if (VERBOSE) std::cout << "c " + to_string(new_model) + "\nc" << std::endl;
                }
                found_model = true;
            } else {
                if (VERBOSE) std::cout << "c ignoring model due to false backtrack\nc" << std::endl;
            }

            false_backtrack = false;

            if (found_model && b < dl) {
                found_model = false;
                if (b - 1 < 0) {
                    if (VERBOSE) std::cout << "c no more decisions to flip, terminating" << std::endl;
                    solver->terminate();
                    shall_terminate = true;
                    STOP (wbc_cb_check_found_model);
                    return false;
                }
                START (wbc_forced_backtrack_model_found);
                solver->force_backtrack(b - 1);
                STOP (wbc_forced_backtrack_model_found);

            } else {
                // finding highest decision level with positive decision
                int highest_pos_dl = highest_dl_to_flip();

                // backtrack to decisionlevel before that, so we can flip the decision
                if (highest_pos_dl - 1 < 0) {
                    if (VERBOSE) std::cout << "c no more decisions to flip, terminating" << std::endl;
                    solver->terminate();
                    shall_terminate = true;
                    STOP (wbc_cb_check_found_model);
                    return false;
                }
                START (wbc_forced_backtrack_model_found);
                solver->force_backtrack(highest_pos_dl - 1);
                STOP (wbc_forced_backtrack_model_found);
                add_reason_clause = true;
            }

            STOP (wbc_cb_check_found_model);

            // always return false -> solver can only terminate with UNSAT: no more solutions
            return false;
        };

        // always false as we DO NOT want to add clauses
        bool cb_has_external_clause (bool &is_forgettable) override {
            if (VERBOSE) std::cout << "c\nc cb_has_external_clause:" << std::endl;
            return false;
        };

        int cb_add_external_clause_lit () override {
            if (VERBOSE) std::cout << "c\nc cb_add_external_clause_lit:" << std::endl;
            return 0;
        };

        // this function is called when observed variables are assigned (either by BCP, Decide, or Learning a unit clause). It has a single read-only argument containing literals that became satisfied by the new assignment. In case the notification reports more than one literal, it is guaranteed that all of the reported literals were assigned on the same (current) decision level.
        void notify_assignment(const ivec &list) override {
            if (VERBOSE) std::cout << "c\nc notify_assignment:" << std::endl;

            if (shall_terminate) {
                if (VERBOSE) std::cout << "c terminating because shall_terminate is true" << std::endl;
                return;
            }

            START (wbc_notify_assignment);

            for (auto lit : list) {

                // true if first lit in list was a decision
                if (is_decision) {
                    is_decision = false;

                    assign_or_append(decisions, lit, dl);
                    increase_or_append(decision_counts_per_level, dl);

                    push(lit, dl, true);
                    if (VERBOSE) std::cout << "c decided: " + std::to_string(lit) + "@" + std::to_string(dl) << std::endl;

                    if (decision_counts_per_level[dl - 1] > 2) {
                        false_backtrack = true;
                        if (VERBOSE) std::cout << "c decision was already made twice on that level, false_backtrack = true" << std::endl;
                    }

                } else {
                    push(lit, dl, false);
                    if (VERBOSE) std::cout << "c forced: " + std::to_string(lit) + "@" + std::to_string(dl) << std::endl;

                    if (lit == decisions.back() || (lit == -decisions.back() && decision_counts_per_level.back() == 2)) {
                        false_backtrack = true;
                        decision_counts_per_level.pop_back();
                        decisions.pop_back();
                        if (VERBOSE) std::cout << "c forced assignment contradicts last decision before backtrack, false_backtrack = true" << std::endl;
                    }
                    if (lit == -decisions.back()) {
                        if (VERBOSE) std::cout << "c removed last element of decision count because its no longer a decision" << std::endl;
                        // remove from decision stack as it is no longer one
                        decision_counts_per_level.pop_back();
                        decisions.pop_back();
                    }
                }
            }

            STOP (wbc_notify_assignment);

            if (VERBOSE) print_all(stack, values, dls, is_ds, decisions, decision_counts_per_level, dl);
            if (VERBOSE_DC) print_dc(decisions, decision_counts_per_level, dl);
        };

        // the call of this function indicates to the user that on the trail a new decision level has started. The function does not report the actual decision that started this new level or the current decision level — it only reports that a decision happened and thus, the decision level is increased.
        void notify_new_decision_level () override {
            if (VERBOSE) std::cout << "c\nc notify_new_decision_level:" << std::endl;

            if (shall_terminate) {
                if (VERBOSE) std::cout << "c terminating because shall_terminate is true" << std::endl;
                return;
            }

            if (VERBOSE) std::cout << "c " + std::to_string(dl + 1) << std::endl;
            START (wbc_notify_new_decision_level);

            dl++;

            is_decision = true;

            STOP (wbc_notify_new_decision_level);
        };

        // this function indicates that the solver backtracked to a lower decision level. Its single argument reports the new decision level. All assignments that were made above this target decision level must be considered as unassigned.
        void notify_backtrack (size_t new_level) override {
            if (VERBOSE) std::cout << "c\nc notify_backtrack:" << std::endl;

            if (shall_terminate) {
                if (VERBOSE) std::cout << "c terminating because shall_terminate is true" << std::endl;
                return;
            }

            if (VERBOSE) std::cout << "c to level " + std::to_string(new_level) << std::endl;
            START (wbc_notify_backtrack);

            // update stack
            while (stack.size() > 0 && dls[stack.back()] > (int)new_level) {
                int var = stack.back();
                auto [val, level, is_decision] = pop();

                if (VERBOSE) std::cout << "c removed " + std::to_string(val * var) + "@" + std::to_string(level) << std::endl;
            }

            dl = new_level;
            is_decision = false;

            false_backtrack = false;

            // update decision counts
            assert(decisions.size() == decision_counts_per_level.size());
            while ((int)decision_counts_per_level.size() > dl + 2) {
                if (VERBOSE) std::cout << "c removed decision: " << std::to_string(decisions.back()) << " with count " << std::to_string(decision_counts_per_level.back()) << " @ " << std::to_string(decision_counts_per_level.size() - 1) << std::endl;
                decision_counts_per_level.pop_back();
                decisions.pop_back();
            }

            if (VERBOSE) print_all(stack, values, dls, is_ds, decisions, decision_counts_per_level, dl);
            if (VERBOSE_DC) print_dc(decisions, decision_counts_per_level, dl);

            STOP (wbc_notify_backtrack);
        };

        // called before the solver makes a decision. Return your decision or 0 (solver makes one).
        int cb_decide () override {
            if (VERBOSE) std::cout << "c\nc cb_decide:" << std::endl;

            if (shall_terminate) {
                if (VERBOSE) std::cout << "c terminating because shall_terminate is true" << std::endl;
                return 0;
            }

            START (wbc_cb_decide);

            // if backtracked decision is not forced negated, TODO: can this also happen when the decisions count was on one? is it then unnoticed?
            // should not be a problem, if this would happen, the decision count was one, so the next decision is fixed.
            if (decision_counts_per_level.back() > 2) {
                false_backtrack = true;
                if (VERBOSE) std::cout << "c decision count exceeded 2, false_backtrack = true" << std::endl;
            }

            if (save_decision) {
                if (VERBOSE) std::cout << "c found saved decision: " + std::to_string(saved_decision) << std::endl;
                save_decision = false;
                int lit = saved_decision;
                saved_decision = 0;

                int val = values[std::abs(lit)];

                // saved decision is assigned
                if (val != 0) {
                    // it is fulfilled
                    if ((val == 1 && lit > 0) || (val == -1 && lit < 0)) {
                        if (VERBOSE) std::cout << "c saved decision already satisfied, let solver decide next" << std::endl;
                    } else {
                        // it is falsified
                        false_backtrack = true;
                        if (VERBOSE) std::cout << "c saved decision already falsified, backtracking to avoid duplication" << std::endl;
                    }
                } else {
                    if (VERBOSE) std::cout << "c returning saved decision: " + std::to_string(lit) << std::endl;
                    STOP(wbc_cb_decide);
                    return lit;
                }
            }

            if (false_backtrack) {
                int highest_pos_dl = highest_dl_to_flip();

                if (highest_pos_dl - 1 < 0) {
                    if (VERBOSE) std::cout << "c no positive decisions to flip - finished" << std::endl;
                    solver->terminate();
                    shall_terminate = true;
                    STOP(wbc_cb_decide);
                    return 0;
                }

                if (VERBOSE) std::cout << "c backtracking to: " + std::to_string(highest_pos_dl - 1) + " to avoid duplication" << std::endl;
                saved_decision = -decisions[highest_pos_dl];
                save_decision = true;
                solver->force_backtrack(highest_pos_dl - 1);
                false_backtrack = false;
                STOP (wbc_cb_decide);
                if (VERBOSE) std::cout << "c end false_backtrack - the next decision will be ignored" << std::endl;
                return 0;
            }

            // check if decisions is already fixed
            if (dl + 1 < (int)decisions.size() && decision_counts_per_level[dl + 1] < 2) {
                if (VERBOSE) std::cout << "c decision is already fixed: " + std::to_string(-decisions[dl + 1]) << std::endl;
                STOP(wbc_cb_decide);
                return -decisions[dl + 1];
            }

            if (VERBOSE) print_all(stack, values, dls, is_ds, decisions, decision_counts_per_level, dl);
            if (VERBOSE_DC) print_dc(decisions, decision_counts_per_level, dl);

            if (FIXED) {
                for (int var = 1; var <= max_var; var++) {
                    if (values[var] == 0) {
                        if (VERBOSE) std::cout << "c returning decision: " + std::to_string(var) << std::endl;
                        STOP(wbc_cb_decide);
                        return var;
                    }
                }
            }

            STOP (wbc_cb_decide);

            if (VERBOSE) std::cout << "c let the solver decide" << std::endl;
            return 0;
        };

        int cb_propagate () override {
            if (VERBOSE) std::cout << "c\nc cb_propagate:" << std::endl;
            if (!REASON) return 0;

            START (wbc_cb_propagate);
            if (!add_reason_clause) {
                STOP (wbc_cb_propagate);
                return 0;
            }

            add_reason_clause = false;

            int propagated_lit = -decisions.back();
            int propagated_var = std::abs(propagated_lit);

            reason_clauses[propagated_var] = ivec();

            // build reason clause
            reason_clauses[propagated_var].push_back(-decisions.back());
            for (auto var : stack) {
                if (is_ds[var]) {
                    int lit = values[var] * var;
                    reason_clauses[propagated_var].push_back(-lit);
                    if (VERBOSE) std::cout << "c adding lit: " << std::to_string(-lit) << std::endl;
                }
            }

            if (VERBOSE) std::cout << "c propagated: " << std::to_string(propagated_lit) << std::endl;
            STOP (wbc_cb_propagate);
            return propagated_lit;
        };

        int cb_add_reason_clause_lit (int propagated_lit) override {
            if (VERBOSE) std::cout << "c\nc cb_add_reason_clause_lit:" << std::endl;
            if (!REASON) return 0;

            START (wbc_cb_add_reason_clause_lit);
            if (VERBOSE) std::cout << "c for: " << std::to_string(propagated_lit) << std::endl;
            int var = std::abs(propagated_lit);
            if (reason_clauses[var].size()) {
                int lit = reason_clauses[var].back();
                reason_clauses[var].pop_back();
                STOP (wbc_cb_add_reason_clause_lit);
                if (VERBOSE) std::cout << "c adding: " << std::to_string(lit) << std::endl;
                return lit;
            }
            STOP (wbc_cb_add_reason_clause_lit);
            if (VERBOSE) std::cout << "c finished returning 0" << std::endl;
            return 0;
        };

        void connect_internal (CaDiCaL::Internal *internal_p) {
            internal = internal_p;
        };
};


void arg_parser(int argc, char* argv[], bool& count, bool& verbose, bool& verbose_dc, bool& profile, bool& fixed, bool& shrink, bool& reason, bool& help) {
    std::map<std::string, std::string> parsedArgs = parseArgs(argc, argv);
    count = parsedArgs.count("count") || parsedArgs.count("c");
    verbose = parsedArgs.count("verbose") || parsedArgs.count("v");
    verbose_dc = parsedArgs.count("verbose_dc") || parsedArgs.count("d");
    profile = parsedArgs.count("profile") || parsedArgs.count("p");
    fixed = parsedArgs.count("fixed") || parsedArgs.count("f");
    shrink = parsedArgs.count("shrink") || parsedArgs.count("s");
    reason = parsedArgs.count("reason") || parsedArgs.count("r");
    help = parsedArgs.count("help") || parsedArgs.count("h");
}


int main(int argc, char* argv[]) {
    // arg parser
    arg_parser(argc, argv, COUNT, VERBOSE, VERBOSE_DC, PROFILE, FIXED, SHRINK, REASON, HELP);

    if (HELP) {
        std::string msg =
            "\n\n"
            "Welcome to Projected Enumeration using IPASIR-UP without Blocking Clauses\n"
            "\n"
            "USAGE:\n"
            "\twbcp_enum [--Option (-o)] <path to cnf>\n"
            "\n"
            "OPTIONS:\n"
            "\n"
            "\t-h --help \t Show this menu\n"
            "\t-c --count \t Returns number of models (does not store the models - no exp memory usage)\n"
            "\t-v --verbose \t Returns the log and all models\n"
            "\t-d --verbose_dc \t Returns the logs only for decision counts\n"
            "\t-p --profile \t Returns statistic about where time was spent\n"
            "\t-f --fixed \t Decision order is 1...n\n"
            "\t-s --shrink \t Performs implicant shrinking on found models\n"
            "\t-r --reason \t Uses propagation and reasons for backtracking after found model\n";
        std::cout << msg;
        return 0;
    }

    std::cout << "c Runnning the solver with the following options:" << std::endl;
    if (COUNT) std::cout << "c \tCOUNT" << std::endl;
    if (VERBOSE) std::cout << "c \tVERBOSE" << std::endl;
    if (VERBOSE_DC) std::cout << "c \tVERBOSE_DC" << std::endl;
    if (FIXED) std::cout << "c \tFIXED" << std::endl;
    if (SHRINK) std::cout << "c \tSHRINK" << std::endl;
    if (PROFILE) std::cout << "c \tPROFILE" << std::endl;
    if (REASON) std::cout << "c \tREASON" << std::endl;

    // create a new solver instance
    CaDiCaL::Solver *solver = new CaDiCaL::Solver;

    // setting options for chronological backtracking
    // how to disable preprocessing?
    solver->set("chronoalways", true);
    solver->set("restart", false);
    solver->set("inprocessing", false);
    solver->set("rephase", false);
    solver->set("log", true);

    // default is 2
    // solver->set("profile", 2);

    // create a new EnumProp instance
    EnumProp *ep = new EnumProp;

    ep->solver = solver;

    // connect EnumProp to solver instance
    solver->connect_external_propagator(ep);

    // connect tracer and delete proof (so maybe no overhead) only need pointer to watches
    solver->connect_proof_tracer(ep, false);
    solver->disconnect_proof_tracer(ep);
    delete ep->internal->proof;
    ep->internal->proof = nullptr;

    // extract num of variables from dimacs file
    int numVariables = 0;

    solver->read_dimacs(argv[argc - 1], numVariables);

    // add dummy at index 0 so indices match variable numbers
    ep->values.push_back(0);
    ep->dls.push_back(-1);
    ep->is_ds.push_back(false);
    ep->decision_counts_per_level.push_back(0);
    ep->decisions.push_back(0);

    ep->poss_in_stack.push_back(-1);

    ep->reason_clauses.push_back(ivec());

    tclause vars;
    // mark all variables as relevant for observing
    for (int var = 1; var <= numVariables; var++) {
        solver->add_observed_var(var);

        // initialize all values, dls and is_ds
        ep->values.push_back(0);
        ep->dls.push_back(-1);
        ep->is_ds.push_back(false);

        ep->poss_in_stack.push_back(-1);

        ep->reason_clauses.push_back(ivec());
    }

    ep->max_var = numVariables;

    // run solver
    if (VERBOSE) std::cout << "c start solving\nc" << std::endl;
    int res = solver->solve();

    if (PROFILE) solver->statistics();

    if (VERBOSE) {
        std::cout << "c all models: " + to_string(ep->all_models) << std::endl;

        std::cout << res;
        std::cout << "" << std::endl;
    }

    if (COUNT) {
        std::cout << "NUMBER SATISFYING ASSIGNMENTS" << std::endl;
        std::cout << count;
        std::cout << "" << std::endl;
    }

    // write negated models to file
    std::ofstream file(NEGATED_MODELS);

    if (file.is_open()) {
        int i = 1;
        for (auto model : ep->all_models) {
            file << "i " << i << " " << to_string(model, true) << " 0" << std::endl;
            i++;
        }
        file.close();
    } else {
        std::cerr << "Unable to open " << NEGATED_MODELS << "." << std::endl;
    }

    // disconnect EnumProp
    solver->disconnect_external_propagator();

    // delete the solver instance
    delete solver;

    return 0;
}
