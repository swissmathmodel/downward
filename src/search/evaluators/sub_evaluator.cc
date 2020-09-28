#include "sub_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../option_parser.h"
#include "../plugin.h"

#include <cstdlib>
#include <sstream>
#include <cmath>

using namespace std;
namespace sub_evaluator {
    SubEvaluator::SubEvaluator(const Options &opts)
            : evaluator(opts.get < shared_ptr < Evaluator >> ("eval")),
              w(opts.get<double>("weight")),
              type(opts.get<EvaluatorType>("type")) {
    }

    SubEvaluator::SubEvaluator(const shared_ptr <Evaluator> &eval, double weight, EvaluatorType type)
            : evaluator(eval), w(weight), type(type) {
    }

    SubEvaluator::~SubEvaluator() {
    }

    bool SubEvaluator::dead_ends_are_reliable() const {
        return evaluator->dead_ends_are_reliable();
    }

    EvaluationResult SubEvaluator::compute_result(
            EvaluationContext &eval_context) {
        // Note that this produces no preferred operators.
        int g = eval_context.get_g_value();
        EvaluationResult result;
        int h = eval_context.get_evaluator_value_or_infinity(evaluator.get());
        int value = -1;
        if (h == EvaluationResult::INFTY) {
            value = h;
        } else {
            if (type == WA) {
                value = g / w + h;
            }

            if (type == XDP) {
                value = ((1 / (2 * w)) * (((2 * w - 1) * h) + g + sqrt((pow((g - h), 2)) + (4 * w * g * h))));
            }

            if (type == XUP) {
                value = ((1 / (2 * w)) * (h + g + sqrt((pow((g + h), 2)) + (4 * w * (w - 1) * h * h))));
            }

            if (type == PWXDP) {
                if (h > g) {
                    value = g + h;
                } else {
                    value = (g + ((2 * w - 1) * h)) / w;
                }
            } else {
                ABORT("invalid type");
            }
    }

    result.set_evaluator_value(value);
    return result;
}

    void SubEvaluator::get_path_dependent_evaluators(set<Evaluator *> &evals) {
        evaluator->get_path_dependent_evaluators(evals);
    }

    static shared_ptr <Evaluator> _parse(OptionParser &parser) {
        vector <string> evaluator_type;
        vector <string> evaluator_type_doc;
        evaluator_type.push_back("WA");
        evaluator_type_doc.push_back(
                "WA evaluator: weighted priority function: f(n) = w * h(n) + g(n)");
        evaluator_type.push_back("XDP");
        evaluator_type_doc.push_back(
                "XDP evaluator: Convex downward parabola priority function: f(n) = ((1/(2*w))*((2*w -1)h(n) + g(n) + sqrt((g(n)-h(n))^2 + 4*w*g(n)*h(n))");
        evaluator_type.push_back("XUP");
        evaluator_type_doc.push_back(
                "XUP evaluator: Convex upward parabola priority function: f(n) = ((1/(2*w))*(h(n) + g(n) + sqrt((g(n)+h(n))^2 + 4*w*(w-1)*g(n)^2)");
        evaluator_type.push_back("PWXDP");
        evaluator_type_doc.push_back(
                "PWXDP priority function: f(n) = h(n) + g(n) if h(n) > g(n), otherwise f(n) = (g + (2*w - 1)*h) / w");
        parser.document_synopsis(
                "WA/XDP/XUP/PWXDP evaluator",
                "Different priority functions for suboptimal search");
        parser.add_option < shared_ptr < Evaluator >> ("eval", "evaluator");
        parser.add_option<double>("weight", "weight");
        parser.add_enum_option<EvaluatorType>(
                "type", evaluator_type, "pick evaluator", "WA", evaluator_type_doc);
        Options opts = parser.parse();
        if (parser.dry_run())
            return nullptr;
        else
            return make_shared<SubEvaluator>(opts);
    }

    static Plugin<Evaluator> _plugin("sub", _parse, "evaluators_basic");
}
