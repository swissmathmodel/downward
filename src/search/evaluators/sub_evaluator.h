#ifndef EVALUATORS_SUB_EVALUATOR_H
#define EVALUATORS_SUB_EVALUATOR_H

#include "../evaluator.h"

#include <memory>

namespace options {
class Options;
}

enum EvaluatorType {WA, XDP, XUP, PWXDP};

namespace sub_evaluator {
class SubEvaluator : public Evaluator {
    std::shared_ptr<Evaluator> evaluator;
    double w;
    EvaluatorType t;

public:
    explicit SubEvaluator(const options::Options &opts);
    SubEvaluator(const std::shared_ptr<Evaluator> &eval, double weight, EvaluatorType type);
    virtual ~SubEvaluator() override;

    virtual bool dead_ends_are_reliable() const override;

    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;

    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &evals) override;
};
}

#endif
