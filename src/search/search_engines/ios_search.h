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
    bool found_plan;
    SearchSpace focal_search_space;
    std::unique_ptr<StateOpenList> focal_list;
    std::unique_ptr<StateOpenList> open_list;

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
