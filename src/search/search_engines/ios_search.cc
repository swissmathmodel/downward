#include "ios_search.h"

#include "../option_parser.h"
#include "../evaluation_context.h"

#include "../open_lists/best_first_open_list.h"
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
      weight(opts.get<double>("weight")),
      heuristic(opts.get<shared_ptr<Evaluator>>("heuristic")),
      focal_search_space(utils::make_unique_ptr<SearchSpace>(state_registry)) {
    // Set up focal list.
    double focal_weight = 2 * weight - 1;
    EvaluatorType priority_function_type(opts.get<EvaluatorType>("type"));
    shared_ptr<Evaluator> focal_eval = make_shared<sub_evaluator::SubEvaluator>(
        heuristic, focal_weight, priority_function_type);
    Options focal_options;
    focal_options.set<shared_ptr<Evaluator>>("eval", focal_eval);
    focal_options.set<bool>("pref_only", false);
    focal_list = utils::make_unique_ptr<standard_scalar_open_list::BestFirstOpenListFactory>(
        focal_options)->create_state_open_list();

    // Set up open list.
    Options open_options;
    open_options.set<shared_ptr<Evaluator>>(
        "eval", make_shared<sub_evaluator::SubEvaluator>(heuristic, 1, EvaluatorType::WA));
    open_options.set<bool>("pref_only", false);
    open_list = utils::make_unique_ptr<standard_scalar_open_list::BestFirstOpenListFactory>(
        open_options)->create_state_open_list();
}

void IOSSearch::initialize() {
    utils::g_log << "Conducting IOS search"
                 << (reopen_closed_nodes ? " with" : " without")
                 << " reopening closed nodes, (real) bound = " << bound
                 << endl;

    const GlobalState &initial_state = state_registry.get_initial_state();
    EvaluationContext eval_context(initial_state, 0, true, &statistics);

    statistics.inc_evaluated_states();

    if (focal_list->is_dead_end(eval_context)) {
        utils::g_log << "Initial state is a dead end." << endl;
    } else {
        if (search_progress.check_progress(eval_context))
            statistics.print_checkpoint_line(0);

        SearchNode node_focal = focal_search_space->get_node(initial_state);
        node_focal.open_initial();
        focal_list->insert(eval_context, initial_state.get_id());

        SearchNode node_open = search_space.get_node(initial_state);
        node_open.open_initial();
        open_list->insert(eval_context, initial_state.get_id());
    }

    print_initial_evaluator_values(eval_context);
}

void IOSSearch::print_statistics() const {
    statistics.print_detailed_statistics();
    if (focal_search_space)
        focal_search_space->print_statistics();
    search_space.print_statistics();
}

bool IOSSearch::check_goal_and_set_incumbent_plan(const GlobalState &state) {
    if (task_properties::is_goal_state(task_proxy, state)) {
        utils::g_log << "Incumbent solution found!" << endl;
        // Only the first plan will be found in the focal search space.
        if (!incumbent_plan) {
            incumbent_plan = utils::make_unique_ptr<Plan>();
            focal_search_space->trace_path(state, *incumbent_plan);
        } else {
            incumbent_plan->clear();
            search_space.trace_path(state, *incumbent_plan);
        }
        return true;
    }
    return false;
}

SearchStatus IOSSearch::do_focal_list_step() {
    tl::optional <SearchNode> node;
    while (true) {
        if (focal_list->empty()) {
            utils::g_log << "Completely explored state space -- no solution!" << endl;
            return FAILED;
        }
        StateID id = focal_list->remove_min();
        GlobalState s = state_registry.lookup_state(id);
        node.emplace(focal_search_space->get_node(s));

        if (node->is_closed())
            continue;

        node->close();
        assert(!node->is_dead_end());
        statistics.inc_expanded();
        break;
    }

    GlobalState s = node->get_state();
    if (check_goal_and_set_incumbent_plan(s))
        return SOLVED;

    vector<OperatorID> applicable_ops;
    successor_generator.generate_applicable_ops(s, applicable_ops);

    for (OperatorID op_id : applicable_ops) {
        OperatorProxy op = task_proxy.get_operators()[op_id];
        if ((node->get_real_g() + op.get_cost()) >= bound)
            continue;

        GlobalState succ_state = state_registry.get_successor_state(s, op);
        statistics.inc_generated();
        bool is_preferred = true;

        SearchNode succ_node = focal_search_space->get_node(succ_state);

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

SearchStatus IOSSearch::do_open_list_step() {
    tl::optional <SearchNode> node;
    while (true) {
        if (open_list->empty()) {
            utils::g_log << "Completely explored state space -- no solution!" << endl;
            return FAILED;
        }
        StateID id = open_list->remove_min();
        GlobalState s = state_registry.lookup_state(id);
        node.emplace(search_space.get_node(s));

        if (node->is_closed())
            continue;

        node->close();
        assert(!node->is_dead_end());
        statistics.inc_expanded();
        break;
    }

    GlobalState s = node->get_state();
    if (check_goal_and_set_incumbent_plan(s))
        return SOLVED;

    // Compute fmin.
    int g = node->get_real_g();
    EvaluationContext eval_context(s, g, true, &statistics);
    assert(eval_context.is_evaluator_value_infinite());
    int fmin = g + eval_context.get_evaluator_value(heuristic.get());

    // TODO: We could compute plan cost only when a new plan is found.
    int plan_cost = calculate_plan_cost(*incumbent_plan, task_proxy);
    if (plan_cost <= weight * fmin) {
        cout << "Proved solution to be w-suboptimal." << endl;
        return SOLVED;
    }

    vector<OperatorID> applicable_ops;
    successor_generator.generate_applicable_ops(s, applicable_ops);

    for (OperatorID op_id : applicable_ops) {
        OperatorProxy op = task_proxy.get_operators()[op_id];
        if ((node->get_real_g() + op.get_cost()) >= bound)
            continue;

        GlobalState succ_state = state_registry.get_successor_state(s, op);
        statistics.inc_generated();
        bool is_preferred = true;

        SearchNode succ_node = search_space.get_node(succ_state);

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

            if (open_list->is_dead_end(succ_eval_context)) {
                succ_node.mark_as_dead_end();
                statistics.inc_dead_ends();
                continue;
            }
            succ_node.open(*node, op, get_adjusted_cost(op));

            open_list->insert(succ_eval_context, succ_state.get_id());
            if (search_progress.check_progress(succ_eval_context)) {
                statistics.print_checkpoint_line(succ_node.get_g());
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
                open_list->insert(succ_eval_context, succ_state.get_id());
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

SearchStatus IOSSearch::step() {
    bool debug = false;
    if (debug) {
        cout << "Incumbent plan: " << incumbent_plan.get() << endl;
    }
    if (!incumbent_plan) {
        SearchStatus status = do_focal_list_step();
        if (debug) {
            cout << "status: " << status << endl;
        }
        if (status == FAILED) {
            return FAILED;
        }
    } else {
        if (focal_list) {
            focal_list = nullptr;
            focal_search_space = nullptr;
        }

        SearchStatus status = do_open_list_step();
        if (debug) {
            cout << "status: " << status << endl;
        }
        if (status == SOLVED) {
            set_plan(*incumbent_plan);
            return SOLVED;
        } else if (status == FAILED) {
            return FAILED;
        }
    }
    return IN_PROGRESS;
}

void IOSSearch::dump_search_space() const {
    if (focal_search_space)
        focal_search_space->dump(task_proxy);
    search_space.dump(task_proxy);
}

void add_options_to_parser(OptionParser &parser) {
    SearchEngine::add_options_to_parser(parser);
}
}
