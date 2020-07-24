#ifndef EVALUATORS_XDP_EVALUATOR_H
#define EVALUATORS_XDP_EVALUATOR_H

#include "../evaluator.h"

#include <memory>

namespace options {
class Options;
}

namespace xdp_evaluator {
class XDPEvaluator : public Evaluator {
    std::shared_ptr<Evaluator> evaluator;
    double w;

public:
    explicit XDPEvaluator(const options::Options &opts);
    XDPEvaluator(const std::shared_ptr<Evaluator> &eval, double weight);
    virtual ~XDPEvaluator() override;

    virtual bool dead_ends_are_reliable() const override;

    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;

    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &evals) override;
};
}

#endif
