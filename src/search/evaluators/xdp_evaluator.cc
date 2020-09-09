#include "xdp_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../option_parser.h"
#include "../plugin.h"

#include <cstdlib>
#include <sstream>
#include <cmath>

using namespace std;

namespace xdp_evaluator {
    XDPEvaluator::XDPEvaluator(const Options &opts)
            : evaluator(opts.get<shared_ptr<Evaluator>>("eval")),
              w(opts.get<double>("weight"))
    {
    }

    XDPEvaluator::XDPEvaluator(const shared_ptr<Evaluator> &eval, double weight)
            : evaluator(eval), w(weight) {
    }

    XDPEvaluator::~XDPEvaluator() {
    }

    bool XDPEvaluator::dead_ends_are_reliable() const {
        return evaluator->dead_ends_are_reliable();
    }

    EvaluationResult XDPEvaluator::compute_result(
            EvaluationContext &eval_context) {
        // Note that this produces no preferred operators.
        int g = eval_context.get_g_value();
        EvaluationResult result;
        int value = eval_context.get_evaluator_value_or_infinity(evaluator.get());
        if (value != EvaluationResult::INFTY) {
            value = ((2*w - 1) * value) + g + sqrt(((g - value)^2) + (4*w*g*value));
        }
        result.set_evaluator_value(value);
        return result;
    }

    void XDPEvaluator::get_path_dependent_evaluators(set<Evaluator *> &evals) {
        evaluator->get_path_dependent_evaluators(evals);
    }

    static shared_ptr<Evaluator> _parse(OptionParser &parser) {
        parser.document_synopsis(
                "XDP evaluator",
                "Convex downward parabola priority function:,"
                "f(n) = (2*w -1)h(n) + g(n) + sqrt((g(n)-h(n))^2 + 4*w*g(n)*h(n)");
        parser.add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        parser.add_option<double>("weight", "weight");
        Options opts = parser.parse();
        if (parser.dry_run())
            return nullptr;
        else
            return make_shared<XDPEvaluator>(opts);
    }

    static Plugin<Evaluator> _plugin("xdp", _parse, "evaluators_basic");
}