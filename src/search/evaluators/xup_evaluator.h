#ifndef EVALUATORS_XUP_EVALUATOR_H
#define EVALUATORS_XUP_EVALUATOR_H

#include "../evaluator.h"

#include <memory>

namespace options {
class Options;
}

namespace xup_evaluator {
class XUPEvaluator : public Evaluator {
    std::shared_ptr<Evaluator> evaluator;
    double w;

public:
    explicit XUPEvaluator(const options::Options &opts);
    XUPEvaluator(const std::shared_ptr<Evaluator> &eval, double weight);
    virtual ~XUPEvaluator() override;

    virtual bool dead_ends_are_reliable() const override;

    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;

    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &evals) override;
};
}

#endif
