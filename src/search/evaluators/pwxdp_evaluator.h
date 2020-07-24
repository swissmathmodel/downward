#ifndef EVALUATORS_PWXDP_EVALUATOR_H
#define EVALUATORS_PWXDP_EVALUATOR_H

#include "../evaluator.h"

#include <memory>

namespace options {
class Options;
}

namespace pwxdp_evaluator {
class PWXDPEvaluator : public Evaluator {
    std::shared_ptr<Evaluator> evaluator;
    double w;

public:
    explicit PWXDPEvaluator(const options::Options &opts);
    PWXDPEvaluator(const std::shared_ptr<Evaluator> &eval, double weight);
    virtual ~PWXDPEvaluator() override;

    virtual bool dead_ends_are_reliable() const override;

    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;

    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &evals) override;
};
}

#endif
