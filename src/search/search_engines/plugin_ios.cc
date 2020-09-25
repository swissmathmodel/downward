#include "eager_search.h"
#include "search_common.h"

#include "../option_parser.h"
#include "../plugin.h"

using namespace std;

namespace plugin_ios{
    static shared_ptr<SearchEngine> _parse(OptionParser &parser) {
        parser.document_synopsis("Improved Optimistic Search",
                "This search function uses specified priority functions "
                "in a focal open list to conduct suboptimal search.");
        parser.add_option<shared_ptr<OpenListFactory>>("focal", "focal list");
        parser.add_option<bool>("reopen_closed",
                                "reopen closed nodes", "false");
        parser.add_option<shared_ptr<Evaluator>>(
                "f_eval", "evaluator for focal list");

        ios_search::add_options_to_parser(parser);
        Options opts = parser.parse();

        shared_ptr<ios_search::IOSSearch> engine;
        if (!parser.dry_run()) {
            engine = make_shared<eager_search::EagerSearch>(opts);
        }

        return engine;
    }

    static Plugin<SearchEngine> _plugin("ios", _parse);
}