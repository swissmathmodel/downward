#include "ios_search.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../evaluators/sub_evaluator.h"

using namespace std;

namespace plugin_ios {
static shared_ptr<SearchEngine> _parse(OptionParser &parser) {
    parser.document_synopsis("Improved Optimistic Search",
                             "This search function uses specified priority functions "
                             "in a focal open list to conduct suboptimal search.");
    parser.add_option<shared_ptr<Evaluator>>("heuristic", "heuristic");
    parser.add_option<double>("weight", "weight", "2.0");
    sub_evaluator::add_priority_function_option(parser);
    parser.add_option<bool>("reopen_closed", "reopen closed nodes", "true");

    ios_search::add_options_to_parser(parser);
    Options opts = parser.parse();

    shared_ptr<ios_search::IOSSearch> engine;
    if (!parser.dry_run()) {
        engine = make_shared<ios_search::IOSSearch>(opts);
    }

    return engine;
}

static Plugin<SearchEngine> _plugin("ios", _parse);
}
