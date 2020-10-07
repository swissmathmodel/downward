#include "ios_search.h"

#include "../option_parser.h"
#include "../open_list_factory.h"
#include "../evaluation_context.h"

#include "../algorithms/ordered_set.h"
#include "../task_utils/successor_generator.h"

#include "../evaluators/sub_evaluator.h"
#include "../task_utils/successor_generator.h"
#include "../task_utils/task_properties.h"
#include "../utils/logging.h"
#include "../utils/system.h"

#include <cassert>
#include <cstdlib>
#include <memory>
#include <optional.hh>
#include <set>


using namespace std;

namespace ios_search {
    IOSSearch::IOSSearch(const Options &opts)
            : SearchEngine(opts),
              reopen_closed_nodes(opts.get<bool>("reopen_closed")),
              focal_list(opts.get < shared_ptr < OpenListFactory >> ("focal")->
                      create_state_open_list()),
              f_evaluator(opts.get < shared_ptr < Evaluator >> ("f_eval", nullptr)),
              preferred_operator_evaluators(opts.get_list < shared_ptr < Evaluator >> ("preferred")){}

    void IOSSearch::initialize() {
        utils::g_log << "Conducting IOS search"
                     << (reopen_closed_nodes ? " with" : " without")
                     << " reopening closed nodes, (real) bound = " << bound
                     << endl;
        assert(focal_list);

        set < Evaluator * > evals;
        focal_list->get_path_dependent_evaluators(evals);

        for (const shared_ptr <Evaluator> &evaluator : preferred_operator_evaluators) {
            evaluator->get_path_dependent_evaluators(evals);
        }

        if (f_evaluator) {
            f_evaluator->get_path_dependent_evaluators(evals);
        }

        //Collect path-dependent evaluators that are used in the sub_evaluator
        path_dependent_evaluators.assign(evals.begin(), evals.end());

        const GlobalState &initial_state = state_registry.get_initial_state();
        for (Evaluator *evaluator : path_dependent_evaluators) {
            evaluator->notify_initial_state(initial_state);
        }

        EvaluationContext eval_context(initial_state, 0, true, &statistics);

        statistics.inc_evaluated_states();

        if (focal_list->is_dead_end(eval_context)) {
            utils::g_log << "Initial state is a dead end." << endl;
        } else {
            if (search_progress.check_progress(eval_context))
                statistics.print_checkpoint_line(0);
            start_f_value_statistics(eval_context);
            SearchNode node = search_space.get_node(initial_state);
            node.open_initial();

            focal_list->insert(eval_context, initial_state.get_id());
        }

        print_initial_evaluator_values(eval_context);
    }

    void IOSSearch::print_statistics() const {
        statistics.print_detailed_statistics();
        search_space.print_statistics();
    }

    SearchStatus IOSSearch::step() {
        tl::optional <SearchNode> node;
        while (true) {
            if (focal_list->empty()) {
                utils::g_log << "Completely explored state space -- no solution!" << endl;
                return FAILED;
            }
            StateID id = focal_list->remove_min();
            GlobalState s = state_registry.lookup_state(id);
            node.emplace(search_space.get_node(s));

            if (node->is_closed())
                continue;

            /*
              We can pass calculate_preferred=false here since preferred
              operators are computed when the state is expanded.
            */
            EvaluationContext eval_context(s, node->get_g(), false, &statistics);

            node->close();
            assert(!node->is_dead_end());
            update_f_value_statistics(eval_context);
            statistics.inc_expanded();
            break;
        }

        GlobalState s = node->get_state();
        if (check_goal_and_switch_to_open(s))
            return SOLVED; //HERE

        vector <OperatorID> applicable_ops;
        successor_generator.generate_applicable_ops(s, applicable_ops);

        // This evaluates the expanded state (again) to get preferred ops
        EvaluationContext eval_context(s, node->get_g(), false, &statistics, true);
        ordered_set::OrderedSet<OperatorID> preferred_operators;
        for (const shared_ptr <Evaluator> &preferred_operator_evaluator : preferred_operator_evaluators) {
            collect_preferred_operators(eval_context,
                                        preferred_operator_evaluator.get(),
                                        preferred_operators);
        }

        for (OperatorID op_id : applicable_ops) {
            OperatorProxy op = task_proxy.get_operators()[op_id];
            if ((node->get_real_g() + op.get_cost()) >= bound)
                continue;

            GlobalState succ_state = state_registry.get_successor_state(s, op);
            statistics.inc_generated();
            bool is_preferred = preferred_operators.contains(op_id);

            SearchNode succ_node = search_space.get_node(succ_state);

            for (Evaluator *evaluator : path_dependent_evaluators) {
                evaluator->notify_state_transition(s, op_id, succ_state);
            }

            // Previously encountered dead end. Don't re-evaluate.
            if (succ_node.is_dead_end())
                continue;

            if (succ_node.is_new()) {
                // We have not seen this state before.
                // Evaluate and create a new node.

                int succ_g = node->get_g() + get_adjusted_cost(op);

                EvaluationContext succ_eval_context(
                        succ_state, succ_g, is_preferred, &statistics);
                statistics.inc_evaluated_states();

                if (focal_list->is_dead_end(succ_eval_context)) {
                    succ_node.mark_as_dead_end();
                    statistics.inc_dead_ends();
                    continue;
                }
                succ_node.open(*node, op, get_adjusted_cost(op));

                focal_list->insert(succ_eval_context, succ_state.get_id());
                if (search_progress.check_progress(succ_eval_context)) {
                    statistics.print_checkpoint_line(succ_node.get_g());
                    reward_progress();
                }
            } else if (succ_node.get_g() > node->get_g() + get_adjusted_cost(op)) {

                if (reopen_closed_nodes) {
                    if (succ_node.is_closed()) {
                        statistics.inc_reopened();
                    }
                    succ_node.reopen(*node, op, get_adjusted_cost(op));

                    EvaluationContext succ_eval_context(
                            succ_state, succ_node.get_g(), is_preferred, &statistics);

                    /*
                      Note: our old code used to retrieve the h value from
                      the search node here. Our new code recomputes it as
                      necessary, thus avoiding the incredible ugliness of
                      the old "set_evaluator_value" approach, which also
                      did not generalize properly to settings with more
                      than one evaluator.

                      Reopening should not happen all that frequently, so
                      the performance impact of this is hopefully not that
                      large. In the medium term, we want the evaluators to
                      remember evaluator values for states themselves if
                      desired by the user, so that such recomputations
                      will just involve a look-up by the Evaluator object
                      rather than a recomputation of the evaluator value
                      from scratch.
                    */
                    focal_list->insert(succ_eval_context, succ_state.get_id());
                } else {
                    // If we do not reopen closed nodes, we just update the parent pointers.
                    // Note that this could cause an incompatibility between
                    // the g-value and the actual path that is traced back.
                    succ_node.update_parent(*node, op, get_adjusted_cost(op));
                }
            }
        }

        return IN_PROGRESS;
    }

    void IOSSearch::reward_progress() {
        focal_list->boost_preferred();
    }

    void IOSSearch::dump_search_space() const {
        search_space.dump(task_proxy);
    }

    void IOSSearch::start_f_value_statistics(EvaluationContext &eval_context) {
        if (f_evaluator) {
            int f_value = eval_context.get_evaluator_value(f_evaluator.get());
            statistics.report_f_value_progress(f_value);
        }
    }

    void IOSSearch::update_f_value_statistics(EvaluationContext &eval_context) {
        if (f_evaluator) {
            int f_value = eval_context.get_evaluator_value(f_evaluator.get());
            statistics.report_f_value_progress(f_value);
        }
    }

    void add_options_to_parser(OptionParser &parser) {
        SearchEngine::add_options_to_parser(parser);
    }

    bool IOSSearch::check_goal_and_switch_to_open(const GlobalState &state) {
        if (task_properties::is_goal_state(task_proxy, state)) {
            utils::g_log << "Solution found!" << endl;
            Plan plan;
            search_space.trace_path(state, plan);
            set_plan(plan);
            return true;
        }
        return false;
    }
}
