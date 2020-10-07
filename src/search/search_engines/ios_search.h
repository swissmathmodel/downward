#ifndef SEARCH_ENGINES_IOS_SEARCH_H
#define SEARCH_ENGINES_IOS_SEARCH_H

#include "../open_list.h"
#include "../search_engine.h"

#include "../evaluators/sub_evaluator.h"

#include <memory>
#include <vector>

class Evaluator;

namespace options {
class OptionParser;
class Options;
}

namespace ios_search {
class IOSSearch : public SearchEngine {
    const bool reopen_closed_nodes;
    const double weight;
    const std::shared_ptr<Evaluator> heuristic;
    std::unique_ptr<SearchSpace> focal_search_space;
    std::unique_ptr<StateOpenList> focal_list;
    std::unique_ptr<StateOpenList> open_list;

    std::unique_ptr<Plan> incumbent_plan;

    bool check_goal_and_set_incumbent_plan(const GlobalState &state);
    SearchStatus do_focal_list_step();
    SearchStatus do_open_list_step();

protected:
    virtual void initialize() override;

    virtual SearchStatus step() override;

public:
    explicit IOSSearch(const options::Options &opts);
    virtual ~IOSSearch() = default;

    virtual void print_statistics() const override;

    void dump_search_space() const;
};

extern void add_options_to_parser(options::OptionParser &parser);
}

#endif
