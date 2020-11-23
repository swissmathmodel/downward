#! /usr/bin/env python

import os
import os.path
from pathlib import Path
import platform

from downward.experiment import FastDownwardExperiment
from downward.reports.absolute import AbsoluteReport
from downward.reports.compare import ComparativeReport
from downward.reports.scatter import ScatterPlotReport
from lab.environments import BaselSlurmEnvironment, LocalEnvironment


ATTRIBUTES = ["coverage", "error", "expansions", "total_time", "run_dir", "cost"]

NODE = platform.node()
if NODE.endswith(".scicore.unibas.ch") or NODE.endswith(".cluster.bc2.ch"):
    # Create bigger suites with suites.py from the downward-benchmarks repo.
    SUITE = ["agricola-opt18-strips", "airport", "barman-opt11-strips", "barman-opt14-strips", "blocks", "childsnack-opt14-strips", "data-network-opt18-strips", "depot", "driverlog", "elevators-opt08-strips", "elevators-opt11-strips", "floortile-opt11-strips", "floortile-opt14-strips", "freecell", "ged-opt14-strips", "grid", "gripper", "hiking-opt14-strips", "logistics00", "logistics98", "miconic", "movie", "mprime", "mystery", "nomystery-opt11-strips", "openstacks-opt08-strips", "openstacks-opt11-strips", "openstacks-opt14-strips", "openstacks-strips", "organic-synthesis-opt18-strips", "organic-synthesis-split-opt18-strips", "parcprinter-08-strips", "parcprinter-opt11-strips", "parking-opt11-strips", "parking-opt14-strips", "pathways", "pegsol-08-strips", "pegsol-opt11-strips", "petri-net-alignment-opt18-strips", "pipesworld-notankage", "pipesworld-tankage", "psr-small", "rovers", "satellite", "scanalyzer-08-strips", "scanalyzer-opt11-strips", "snake-opt18-strips", "sokoban-opt08-strips", "sokoban-opt11-strips", "spider-opt18-strips", "storage", "termes-opt18-strips", "tetris-opt14-strips", "tidybot-opt11-strips", "tidybot-opt14-strips", "tpp", "transport-opt08-strips", "transport-opt11-strips", "transport-opt14-strips", "trucks-strips", "visitall-opt11-strips", "visitall-opt14-strips", "woodworking-opt08-strips", "woodworking-opt11-strips", "zenotravel"]
    # SUITE = ["agricola-opt18-strips", "airport", "barman-opt11-strips", "barman-opt14-strips", "blocks", "childsnack-opt14-strips", "data-network-opt18-strips", "depot", "driverlog", "elevators-opt08-strips", "elevators-opt11-strips", "floortile-opt11-strips", "floortile-opt14-strips", "freecell", "ged-opt14-strips", "grid", "gripper", "hiking-opt14-strips", "logistics00", "logistics98", "miconic", "movie", "mprime", "mystery", "nomystery-opt11-strips", "openstacks-opt08-strips", "openstacks-opt11-strips", "openstacks-opt14-strips", "openstacks-strips", "organic-synthesis-opt18-strips", "organic-synthesis-split-opt18-strips", "parking-opt11-strips", "parking-opt14-strips", "pathways", "pegsol-08-strips", "pegsol-opt11-strips", "petri-net-alignment-opt18-strips", "pipesworld-notankage", "pipesworld-tankage", "psr-small", "rovers", "satellite", "scanalyzer-08-strips", "scanalyzer-opt11-strips", "snake-opt18-strips", "sokoban-opt08-strips", "sokoban-opt11-strips", "spider-opt18-strips", "storage", "termes-opt18-strips", "tetris-opt14-strips", "tidybot-opt11-strips", "tidybot-opt14-strips", "tpp", "transport-opt08-strips", "transport-opt11-strips", "transport-opt14-strips", "trucks-strips", "visitall-opt11-strips", "visitall-opt14-strips", "woodworking-opt08-strips", "woodworking-opt11-strips", "zenotravel"]
    ENV = BaselSlurmEnvironment(email="caroline.steiblin@stud.unibas.ch")
else:
    SUITE = ["transport-opt11-strips:p01.pddl"]
    ENV = LocalEnvironment(processes=2)

# Use path to your Fast Downward repository.
REPO = str(Path(__file__).resolve().parents[2])
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REV = "steiblinbscthesis"

exp = FastDownwardExperiment(environment=ENV)

# Add built-in parsers to the experiment.
exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.TRANSLATOR_PARSER)
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)

exp.add_suite(BENCHMARKS_DIR, SUITE)
exp.add_algorithm("ASTAR", REPO, REV, ["--search", "astar(cegar())"])
for function in ["WA", "XDP", "XUP", "PWXDP"]:
    exp.add_algorithm(function, REPO, REV, ["--evaluator", "h=cegar()", "--evaluator", f"fsub=sub(h,2,{function})", "--search", "eager(tiebreaking([fsub, h], unsafe_pruning=false), reopen_closed=true, f_eval=sum([g(), h]))"])

# Add step that writes experiment files to disk.
exp.add_step("build", exp.build)

# Add step that executes all runs.
exp.add_step("start", exp.start_runs)

# Add step that collects properties from run directories and
# writes them to *-eval/properties.
exp.add_fetcher(name="fetch")

for function in ["WA", "XDP", "XUP", "PWXDP"]:
    algorithm_pairs = [("ASTAR", function),("WA", function), ("XDP", function), ("XUP", function)]

# Add report step (ComparativeReport).
exp.add_report(AbsoluteReport(attributes=ATTRIBUTES), outfile="absolute.html")
exp.add_report(ComparativeReport(algorithm_pairs, attributes=ATTRIBUTES), outfile="compare.html")

#def omit_parcprinter(run):
    #return "parcprinter" not in run["domain"]

# Add scatter plot report step.
exp.add_report(ScatterPlotReport(relative=False, attributes=["cost"], filter_algorithm=["ASTAR", "WA"]), outfile="scatterplot_cost_astar_wa.png",)
exp.add_report(ScatterPlotReport(relative=False, attributes=["cost"], filter_algorithm=["ASTAR", "XDP"]), outfile="scatterplot_cost_astar_xdp.png",)
exp.add_report(ScatterPlotReport(relative=False, attributes=["cost"], filter_algorithm=["ASTAR", "XUP"]), outfile="scatterplot_cost_astar_xup.png",)
exp.add_report(ScatterPlotReport(relative=False, attributes=["cost"], filter_algorithm=["ASTAR", "PWXDP"]), outfile="scatterplot_cost_astar_pwxdp.png",)

exp.add_report(ScatterPlotReport(relative=False, attributes=["cost"], filter_algorithm=["WA", "XDP"]), outfile="scatterplot_cost_wa_xdp.png",)
exp.add_report(ScatterPlotReport(relative=False, attributes=["cost"], filter_algorithm=["WA", "XUP"]), outfile="scatterplot_cost_wa_xup.png",)
exp.add_report(ScatterPlotReport(relative=False, attributes=["cost"], filter_algorithm=["WA", "PWXDP"]), outfile="scatterplot_cost_wa_pwxdp.png",)

exp.add_report(ScatterPlotReport(relative=False, attributes=["cost"], filter_algorithm=["XDP", "XUP"]), outfile="scatterplot_cost_xdp_xup.png",)
exp.add_report(ScatterPlotReport(relative=False, attributes=["cost"], filter_algorithm=["XDP", "PWXDP"]), outfile="scatterplot_cost_xdp_pwxdp.png",)

exp.add_report(ScatterPlotReport(relative=False, attributes=["cost"], filter_algorithm=["XUP", "PWXDP"]), outfile="scatterplot_cost_xup_pwxdp.png",)

exp.add_report(ScatterPlotReport(relative=False, attributes=["total_time"], filter_algorithm=["ASTAR", "WA"]), outfile="scatterplot_time_astar_wa.png",)
exp.add_report(ScatterPlotReport(relative=False, attributes=["total_time"], filter_algorithm=["ASTAR", "XDP"]), outfile="scatterplot_time_astar_xdp.png",)
exp.add_report(ScatterPlotReport(relative=False, attributes=["total_time"], filter_algorithm=["ASTAR", "XUP"]), outfile="scatterplot_time_astar_xup.png",)
exp.add_report(ScatterPlotReport(relative=False, attributes=["total_time"], filter_algorithm=["ASTAR", "PWXDP"]), outfile="scatterplot_time_astar_pwxdp.png",)

exp.add_report(ScatterPlotReport(relative=False, attributes=["total_time"], filter_algorithm=["WA", "XDP"]), outfile="scatterplot_time_wa_xdp.png",)
exp.add_report(ScatterPlotReport(relative=False, attributes=["total_time"], filter_algorithm=["WA", "XUP"]), outfile="scatterplot_time_wa_xup.png",)
exp.add_report(ScatterPlotReport(relative=False, attributes=["total_time"], filter_algorithm=["WA", "PWXDP"]), outfile="scatterplot_time_wa_pwxdp.png",)

exp.add_report(ScatterPlotReport(relative=False, attributes=["total_time"], filter_algorithm=["XDP", "XUP"]), outfile="scatterplot_time_xdp_xup.png",)
exp.add_report(ScatterPlotReport(relative=False, attributes=["total_time"], filter_algorithm=["XDP", "PWXDP"]), outfile="scatterplot_time_xdp_pwxdp.png",)

exp.add_report(ScatterPlotReport(relative=False, attributes=["total_time"], filter_algorithm=["XUP", "PWXDP"]), outfile="scatterplot_time_xup_pwxdp.png",)

exp.add_report(ScatterPlotReport(attributes=["expansions"], filter_algorithm=["ASTAR", "WA"]), outfile="scatterplot_expansions_astar_wa.png",)
exp.add_report(ScatterPlotReport(attributes=["expansions"], filter_algorithm=["ASTAR", "XDP"]), outfile="scatterplot_expansions_astar_xdp.png",)
exp.add_report(ScatterPlotReport(attributes=["expansions"], filter_algorithm=["ASTAR", "XUP"]), outfile="scatterplot_expansions_astar_xup.png",)
exp.add_report(ScatterPlotReport(attributes=["expansions"], filter_algorithm=["ASTAR", "PWXDP"]), outfile="scatterplot_expansions_astar_pwxdp.png",)

exp.add_report(ScatterPlotReport(attributes=["expansions"], filter_algorithm=["WA", "XDP"]), outfile="scatterplot_expansions_wa_xdp.png",)
exp.add_report(ScatterPlotReport(attributes=["expansions"], filter_algorithm=["WA", "XUP"]), outfile="scatterplot_expansions_wa_xup.png",)
exp.add_report(ScatterPlotReport(attributes=["expansions"], filter_algorithm=["WA", "PWXDP"]), outfile="scatterplot_expansions_wa_pwxdp.png",)

exp.add_report(ScatterPlotReport(attributes=["expansions"], filter_algorithm=["XDP", "XUP"]), outfile="scatterplot_expansions_xdp_xup.png",)
exp.add_report(ScatterPlotReport(attributes=["expansions"], filter_algorithm=["XDP", "PWXDP"]), outfile="scatterplot_expansions_xdp_pwxdp.png",)

exp.add_report(ScatterPlotReport(attributes=["expansions"], filter_algorithm=["XUP", "PWXDP"]), outfile="scatterplot_expansions_xup_pwxdp.png",)

# Parse the commandline and show or run experiment steps.
exp.run_steps()
