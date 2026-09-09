#include "../../cadical/src/cadical.hpp"
#include "../../cadical/src/tracer.hpp"
#include "../../cadical/src/internal.hpp"
// #include "../../cadical/src/external.hpp"
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
#include <charconv>
#include <cmath>


// global meta variables
bool COUNT = false;
bool VERBOSE = false;
bool VERBOSE_DC = false;
bool SHRINK = false;
bool PROFILE = false;
bool FIXED = false;
bool REASON = false;
bool HELP = false;

uint64_t count = 0;

// cnf must be global so EnumProp can use it
tcnf cnf;

// paths
const char* NEGATED_MODELS = "tmp_cadicall_negated_models.txt";


int check_literal(int e_var, int b, ivec stack, ivec dls, ivec values, int i, ivec poss_in_stack, CaDiCaL::Internal *internal) {
    if (VERBOSE) std::cout << "c\nc check_literal:" << std::endl;
    START (cadicall_check_literal);

    int e_lit = e_var * values[e_var];
    int i_var = internal->external->e2i[e_var];
    int i_lit = internal->external->vals[i_var] ? i_var : -i_var;
    if (VERBOSE) std::cout << "c checking literal: " + std::to_string(e_lit) << " internal: " << std::to_string(i_lit) << std::endl;
    if (VERBOSE) std::cout << "c watched clauses:" << std::endl;
    for (auto watch : internal->watches(i_lit)) {
        if (VERBOSE) {
            std::cout << "c ";
            for (int i = 0; i < watch.size; i++) {
                std::cout << std::to_string(internal->externalize(watch.clause->literals[i])) << " ";
            }
        }
        int i_wl1 = watch.clause->literals[0];
        int i_wl2 = watch.clause->literals[1];

        // other is now the other watched literal
        int i_other_lit = i_lit == i_wl1 ? i_wl2 : i_wl1;
        int e_other_lit = internal->externalize(i_other_lit);
        int e_other_var = std::abs(e_other_lit);

        if (VERBOSE) std::cout << "also by: " << std::to_string(e_other_lit) << " internal: " << std::to_string(i_other_lit) << std::endl;
        if ((poss_in_stack[e_other_var] >= poss_in_stack[e_var]) || (values[e_other_var] * e_other_lit <= 0)) { 
            if (VERBOSE) std::cout << "c b = max(" << std::to_string(b) << "," << std::to_string(dls[e_var]) << ")" << std::endl;
            b = std::max(b, dls[e_var]);
        } else if (VERBOSE) {
            std::cout << "c clause is still satisfied" << std::endl;
        }
    }
    STOP(cadicall_check_literal);
    if (VERBOSE) std::cout << "c returning b = " << std::to_string(b) << std::endl;
    return b;
}


int implicant_shrinking(ivec stack, bvec is_ds, ivec dls, ivec values, ivec dcpl, ivec poss_in_stack, CaDiCaL::Internal *internal) {
    if (VERBOSE) std::cout << "c\nc implicant_shrinking:" << std::endl;
    START (cadicall_implicant_shrinking);

    int b = 0;
    int index = stack.size() - 1;
    while (index >= 0) {
        int v = stack[index];
        // (is_ds[v] && dcpl[dls[v] - 1] > 1) == values[v] < 0
        if (!is_ds[v] || (is_ds[v] && dcpl[dls[v]] == 2)) {
            if (VERBOSE) std::cout << "c " << std::to_string(v * values[v]) << " is not a decision or dc == 2 -> b = max(" << std::to_string(b) << "," << std::to_string(dls[v]) << ")" << std::endl;
            b = std::max(b, dls[v]);
        } else if (dls[v] > b) {
            b = check_literal(v, b, stack, dls, values, index, poss_in_stack, internal);
        } else if (dls[v] == 0 || dls[v] == b) {
            if (VERBOSE) std::cout << "c dl of " << std::to_string(v * values[v]) << " is " << std::to_string(dls[v]) << "(0 or b)" << std::endl;
            break;
        }
        index--;
    }
    STOP(cadicall_implicant_shrinking);
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


void write_models(const char* name, tmodels models) {
    std::ofstream file(NEGATED_MODELS);
    char buffer[32];

    std::size_t counter = 1;

    for (const auto& model : models) {
        if (model.empty()) continue;

        // Write prefix: "i <counter> "
        file.put('i');
        file.put(' ');
        auto [cptr, cec] = std::to_chars(buffer, buffer + 32, counter++);
        file.write(buffer, cptr - buffer);
        file.put(' ');

        // Write first literal
        auto [ptr, ec] = std::to_chars(buffer, buffer + 32, -model[0]);
        file.write(buffer, ptr - buffer);

        // Write remaining literals
        for (size_t i = 1; i < model.size(); i++) {
            file.put(' ');
            auto [ptr2, ec2] = std::to_chars(buffer, buffer + 32, -model[i]);
            file.write(buffer, ptr2 - buffer);
        }

        file.put(' ');
        file.put('0');
        file.put('\n');
    }
}


int power(int base, int exp) {
    int result = 1;
    while (exp > 0) {
        if (exp % 2 == 1)
            result *= base;
        base *= base;
        exp /= 2;
    }
    return result;
}


class EnumProp : public CaDiCaL::ExternalPropagator, public CaDiCaL::InternalTracer {

    private:
        // stack of assigned variables
        ivec stack;

        // arrays where index == var
        ivec values;
        ivec dls;
        bvec is_ds;

        // biggest variable (inclusive)
        int max_var;

        // if a variable is assigned, this maps to its position on the stack, else its -1
        ivec poss_in_stack;

        // decision level
        int dl = 0;

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

        bool propagate_lit = false;

        int reason_clause_idx = 0;
        bool reason_clause_flag = true;

    public:

        void init(int n) {
            int size = n + 1;
            max_var = n;

            values.assign(size, 0);
            dls.assign(size, -1);
            is_ds.assign(size, false);
            poss_in_stack.assign(size, -1);

            decision_counts_per_level.reserve(size);
            decisions.reserve(size);

            decision_counts_per_level.push_back(0);
            decisions.push_back(0);

            stack.reserve(size);
        }

        CaDiCaL::Solver *solver;
        CaDiCaL::Internal *internal;

        tmodels all_models;

        // terminating solver is a problem
        bool shall_terminate = false;

        void push(int lit, int dl, bool is_decision) {
            if (VERBOSE) std::cout << "c push: " << lit << std::endl;
            START (cadicall_push);

            int var = std::abs(lit);

            stack.push_back(var);

            values[var] = (lit > 0) ? 1 : -1;
            dls[var] = dl;
            is_ds[var] = is_decision;

            poss_in_stack[var] = stack.size() - 1;
            STOP(cadicall_push);
        };

        std::tuple<int, int, bool> pop() {
            if (VERBOSE) std::cout << "c pop:" << std::endl;
            START (cadicall_pop);

            int var = stack.back();
            stack.pop_back();

            int val = values[var];
            int level = dls[var];
            bool is_decision = is_ds[var];

            values[var] = 0;
            dls[var] = -1;
            is_ds[var] = false;

            poss_in_stack[var] = -1;

            STOP(cadicall_pop);
            return {val, level, is_decision};
        }

        // if no start is given, starting with current dl
        int highest_dl_to_flip(int start = -1) {
            if (VERBOSE) std::cout << "c\nc highest_dl_to_flip:" << std::endl;
            START (cadicall_highest_dl_to_flip);

            if (start == -1) start = dl;

            int highest_dl = -1;
            for (int i = start; i > 0; i--) {
                if (decision_counts_per_level[i] < 2) {
                    highest_dl = i;
                    break;
                }
            }

            if (VERBOSE) std::cout << "c highest dl with decision count < 2 : " + std::to_string(highest_dl) << std::endl;

            STOP(cadicall_highest_dl_to_flip);
            return highest_dl;
        };

        bool cb_check_found_model (const tmodel &model) override {
            if (VERBOSE) std::cout << "c\nc cb_check_found_model:" << std::endl;

            if (shall_terminate) {
                if (VERBOSE) std::cout << "c terminating because shall_terminate is true" << std::endl;
                return false;
            }

            START (cadicall_cb_check_found_model);

            if (decision_counts_per_level.back() > 2) {
                false_backtrack = true;
                if (VERBOSE) std::cout << "c decision count exceeded 2, false_backtrack = true" << std::endl;
            }

            int b = dl;
            bool found_model = false;
            if (!false_backtrack) {
                if (SHRINK) b = implicant_shrinking(stack, is_ds, dls, values, decision_counts_per_level, poss_in_stack, internal);
                if (COUNT) {
                    count += power(2, dl - b);
                } else {
                    tmodel new_model;
                    new_model.reserve(model.size());
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
                b = highest_dl_to_flip(b);
                if (VERBOSE) std::cout << "c highest decision level <= b with count < 2: " << std::to_string(b) << std::endl;
                found_model = false;
                if (b - 1 < 0) {
                    if (VERBOSE) std::cout << "c no more decisions to flip, terminating" << std::endl;
                    solver->terminate();
                    shall_terminate = true;
                    STOP (cadicall_cb_check_found_model);
                    return false;
                }
                START (cadicall_forced_backtrack_model_found);
                solver->force_backtrack(b - 1);
                STOP (cadicall_forced_backtrack_model_found);

            } else {

                // finding highest decision level with positive decision
                int highest_pos_dl = highest_dl_to_flip();

                // backtrack to decisionlevel before that, so we can flip the decision
                if (highest_pos_dl - 1 < 0) {
                    if (VERBOSE) std::cout << "c no more decisions to flip, terminating" << std::endl;
                    solver->terminate();
                    shall_terminate = true;
                    STOP (cadicall_cb_check_found_model);
                    return false;
                }
                START (cadicall_forced_backtrack_model_found);
                solver->force_backtrack(highest_pos_dl - 1);
                STOP (cadicall_forced_backtrack_model_found);
            }

            propagate_lit = true;
            STOP (cadicall_cb_check_found_model);

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

            START (cadicall_notify_assignment);

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
                        assert(false);
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
                        propagate_lit = false;
                    }
                }
            }

            STOP (cadicall_notify_assignment);

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
            START (cadicall_notify_new_decision_level);

            dl++;

            is_decision = true;

            STOP (cadicall_notify_new_decision_level);
        };

        // this function indicates that the solver backtracked to a lower decision level. Its single argument reports the new decision level. All assignments that were made above this target decision level must be considered as unassigned.
        void notify_backtrack (size_t new_level) override {
            if (VERBOSE) std::cout << "c\nc notify_backtrack:" << std::endl;

            if (shall_terminate) {
                if (VERBOSE) std::cout << "c terminating because shall_terminate is true" << std::endl;
                return;
            }

            if (VERBOSE) std::cout << "c to level " + std::to_string(new_level) << std::endl;
            START (cadicall_notify_backtrack);

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

            // in theory here we can also set propagate_lit = true. However, this also propagates the last decision if the solver made a decision and instantly runs into a conflict. In this case the user propagator is not informed about the decision but still propagates the old one. Thus, it must be dependent of the decision level. Also, if the old decision is already forced. it would propagate the one older one. Handled in notify assignment.

            if (new_level < decision_counts_per_level.size() - 1) propagate_lit = true;

            STOP (cadicall_notify_backtrack);
        };

        // called before the solver makes a decision. Return your decision or 0 (solver makes one).
        int cb_decide () override {
            if (VERBOSE) std::cout << "c\nc cb_decide:" << std::endl;

            if (shall_terminate) {
                if (VERBOSE) std::cout << "c terminating because shall_terminate is true" << std::endl;
                return 0;
            }

            START (cadicall_cb_decide);

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
                    STOP(cadicall_cb_decide);
                    return lit;
                }
            }

            if (false_backtrack) {
                int highest_pos_dl = highest_dl_to_flip();

                if (highest_pos_dl - 1 < 0) {
                    if (VERBOSE) std::cout << "c no positive decisions to flip - finished" << std::endl;
                    solver->terminate();
                    shall_terminate = true;
                    STOP(cadicall_cb_decide);
                    return 0;
                }

                if (VERBOSE) std::cout << "c backtracking to: " + std::to_string(highest_pos_dl - 1) + " to avoid duplication" << std::endl;
                saved_decision = -decisions[highest_pos_dl];
                save_decision = true;
                solver->force_backtrack(highest_pos_dl - 1);
                false_backtrack = false;
                STOP (cadicall_cb_decide);
                if (VERBOSE) std::cout << "c end false_backtrack - the next decision will be ignored" << std::endl;
                return 0;
            }

            // check if decisions is already fixed
            if (dl + 1 < (int)decisions.size() && decision_counts_per_level[dl + 1] < 2) {
                if (VERBOSE) std::cout << "c decision is already fixed: " + std::to_string(-decisions[dl + 1]) << std::endl;
                STOP(cadicall_cb_decide);
                return -decisions[dl + 1];
            }

            if (VERBOSE) print_all(stack, values, dls, is_ds, decisions, decision_counts_per_level, dl);
            if (VERBOSE_DC) print_dc(decisions, decision_counts_per_level, dl);

            if (FIXED) {
                for (int var = 1; var <= max_var; var++) {
                    if (values[var] == 0) {
                        if (VERBOSE) std::cout << "c returning decision: " + std::to_string(var) << std::endl;
                        STOP(cadicall_cb_decide);
                        return var;
                    }
                }
            }

            STOP (cadicall_cb_decide);

            if (VERBOSE) std::cout << "c let the solver decide" << std::endl;
            return 0;
        };

        int cb_propagate () override {
            if (VERBOSE) std::cout << "c\nc cb_propagate:" << std::endl;
            if (!REASON) return 0;
            if (!propagate_lit) return 0;
            if (false_backtrack) return 0;

            START (cadicall_cb_propagate);

            propagate_lit = false;

            if (decision_counts_per_level.back() >= 2) {
                if (VERBOSE) std::cout << "c decision count == 2 - thus " << std::to_string(decisions.back()) << " was already flipped" << std::endl;
                STOP (cadicall_cb_propagate);
                return 0;
            }

            if (VERBOSE) std::cout << "c propagated: " << std::to_string(-decisions.back()) << std::endl;

            STOP (cadicall_cb_propagate);
            return -decisions.back();
        };

        int cb_add_reason_clause_lit (int propagated_lit) override {
            if (VERBOSE) std::cout << "c\nc cb_add_reason_clause_lit:" << std::endl;
            if (!REASON) return 0;
            if (false_backtrack) return 0;

            START (cadicall_cb_add_reason_clause_lit);
            if (VERBOSE) std::cout << "c for: " << std::to_string(propagated_lit) << std::endl;
            int var = std::abs(propagated_lit);

            if (reason_clause_flag) {
                reason_clause_idx = dls[var];
                reason_clause_flag = false;
                STOP (cadicall_cb_add_reason_clause_lit);
                if (VERBOSE) std::cout << "c adding: " << std::to_string(propagated_lit) << std::endl;
                return propagated_lit;
            }

            if (0 < reason_clause_idx) {
                int lit = -decisions[reason_clause_idx];
                reason_clause_idx--;
                STOP (cadicall_cb_add_reason_clause_lit);
                if (VERBOSE) std::cout << "c adding: " << std::to_string(lit) << std::endl;
                return lit;
            }

            reason_clause_flag = true;
            STOP (cadicall_cb_add_reason_clause_lit);
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
            "Welcome to CaDiCAll a model enumerator using IPASIR-UP without Blocking Clauses\n"
            "\n"
            "USAGE:\n"
            "\tcadicall [--Option (-o)] <path to cnf>\n"
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
    solver->set("chrono", true);
    solver->set("chronoalways", true);
    solver->set("chronostrict", true);
    solver->set("restart", false);
    solver->set("inprocessing", false);
    solver->set("rephase", true);
    solver->set("log", false);

    // default is 2
    if (PROFILE) {
        solver->set("profile", 2);
    } else {
        solver->set("profile", 0);
    }

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

    ep->are_reasons_forgettable = true;

    // extract num of variables from dimacs file
    int numVariables = 0;

    solver->read_dimacs(argv[argc - 1], numVariables);

    // initialise all vectors
    ep->init(numVariables);

    // mark all variables as relevant for observing
    for (int var = 1; var <= numVariables; var++) {
        solver->add_observed_var(var);
    }

    // run solver
    if (VERBOSE) std::cout << "c start solving\nc" << std::endl;
    int res = solver->solve();

    if (PROFILE) solver->statistics();

    if (VERBOSE) {
        std::cout << "c all models:\n" + to_string(ep->all_models) << std::endl;

        std::cout << res;
        std::cout << "" << std::endl;
    }

    if (COUNT) {
        std::cout << "NUMBER SATISFYING ASSIGNMENTS" << std::endl;
        std::cout << count;
        std::cout << "" << std::endl;
    }

    // write negated models to file
    write_models(NEGATED_MODELS, ep->all_models);

    // disconnect EnumProp
    solver->disconnect_external_propagator();

    // delete the solver instance
    delete solver;

    return 0;
}
