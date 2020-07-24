#include "pwxdp_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../option_parser.h"
#include "../plugin.h"

#include <cstdlib>
#include <sstream>

using namespace std;

namespace pwxdp_evaluator {
PWXDPEvaluator::PWXDPEvaluator(const Options &opts)
    : evaluator(opts.get<shared_ptr<Evaluator>>("eval")),
      w(opts.get<double>("weight")) 
      { 
}

PWXDPEvaluator::PWXDPEvaluator(const shared_ptr<Evaluator> &eval, double weight)
    : evaluator(eval), w(weight) {
}

PWXDPEvaluator::~PWXDPEvaluator() {
}

bool PWXDPEvaluator::dead_ends_are_reliable() const {
    return evaluator->dead_ends_are_reliable();
}

EvaluationResult PWXDPEvaluator::compute_result(
    EvaluationContext &eval_context) {
    // Note that this produces no preferred operators.
    int g = eval_context.get_g_value();
    EvaluationResult result;
    int value = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    if (value != EvaluationResult::INFTY) {
        if (value > g) {
            value = (g + ((2*w - 1)*value)) / w;
        }
        else {
            value += g;
        }
    }
    result.set_evaluator_value(value);
    return result;
}

void PWXDPEvaluator::get_path_dependent_evaluators(set<Evaluator *> &evals) {
    evaluator->get_path_dependent_evaluators(evals);
}

static shared_ptr<Evaluator> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "PWXDP evaluator",
        "PWXDP priority function:,"
        "f(n) = h(n) + g(n) if h(n) > g(n)"
        "Otherwise f(n) = (g + (2*w - 1)*h) / w");
    parser.add_option<shared_ptr<Evaluator>>("eval", "evaluator");
    parser.add_option<double>("weight", "weight");
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<PWXDPEvaluator>(opts);
}

static Plugin<Evaluator> _plugin("pwxdp", _parse, "evaluators_basic");
}
