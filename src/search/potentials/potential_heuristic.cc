#include "potential_heuristic.h"

#include "potential_function.h"

#include "../option_parser.h"

using namespace std;


namespace potentials {
PotentialHeuristic::PotentialHeuristic(
    const Options &opts, std::unique_ptr<PotentialFunction> function)
    : Heuristic(opts),
      function(move(function)) {
}

int PotentialHeuristic::compute_heuristic(const GlobalState &global_state) {
    const State state = convert_global_state(global_state);
    return function->get_value(state);
}
}
