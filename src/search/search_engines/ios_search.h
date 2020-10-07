#ifndef SEARCH_ENGINES_IOS_SEARCH_H
#define SEARCH_ENGINES_IOS_SEARCH_H

#include "../open_list.h"
#include "../search_engine.h"

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
    std::shared_ptr<Evaluator> eval;
    std::unique_ptr<StateOpenList> focal_list;

protected:
    virtual void initialize() override;

    virtual SearchStatus step() override;

    bool check_goal_and_switch_to_open(const GlobalState &state);

public:
    explicit IOSSearch(const options::Options &opts);
    virtual ~IOSSearch() = default;

    virtual void print_statistics() const override;

    void dump_search_space() const;
};

extern void add_options_to_parser(options::OptionParser &parser);
}

#endif
