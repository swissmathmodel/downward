#! /usr/bin/env python

import os
import os.path
import platform

from downward.experiment import FastDownwardExperiment
from downward.reports.absolute import AbsoluteReport
from downward.reports.compare import ComparativeReport
from downward.reports.scatter import ScatterPlotReport
from lab import cached_revision
from lab.environments import BaselSlurmEnvironment, LocalEnvironment


ATTRIBUTES = ["coverage", "error", "expansions", "total_time"]

NODE = platform.node()
if NODE.endswith(".scicore.unibas.ch") or NODE.endswith(".cluster.bc2.ch"):
    # Create bigger suites with suites.py from the downward-benchmarks repo.
    SUITE = ["agricola-opt18-strips", "airport", "barman-opt11-strips", "barman-opt14-strips", "blocks", "childsnack-opt14-strips", "data-network-opt18-strips", "depot", "driverlog", "elevators-opt08-strips", "elevators-opt11-strips", "floortile-opt11-strips", "floortile-opt14-strips", "freecell", "ged-opt14-strips", "grid", "gripper", "hiking-opt14-strips", "logistics00", "logistics98", "miconic", "movie", "mprime", "mystery", "nomystery-opt11-strips", "openstacks-opt08-strips", "openstacks-opt11-strips", "openstacks-opt14-strips", "openstacks-strips", "organic-synthesis-opt18-strips", "organic-synthesis-split-opt18-strips", "parcprinter-08-strips", "parcprinter-opt11-strips", "parking-opt11-strips", "parking-opt14-strips", "pathways", "pegsol-08-strips", "pegsol-opt11-strips", "petri-net-alignment-opt18-strips", "pipesworld-notankage", "pipesworld-tankage", "psr-small", "rovers", "satellite", "scanalyzer-08-strips", "scanalyzer-opt11-strips", "snake-opt18-strips", "sokoban-opt08-strips", "sokoban-opt11-strips", "spider-opt18-strips", "storage", "termes-opt18-strips", "tetris-opt14-strips", "tidybot-opt11-strips", "tidybot-opt14-strips", "tpp", "transport-opt08-strips", "transport-opt11-strips", "transport-opt14-strips", "trucks-strips", "visitall-opt11-strips", "visitall-opt14-strips", "woodworking-opt08-strips", "woodworking-opt11-strips", "zenotravel"]
    ENV = BaselSlurmEnvironment(email="caroline.steiblin@stud.unibas.ch")
else:
    SUITE = ["depot:p01.pddl", "gripper:prob01.pddl", "mystery:prob07.pddl"]
    ENV = LocalEnvironment(processes=2)

# Use path to your Fast Downward repository.
REPO = os.environ["DOWNWARD_REPO"]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
VCS = cached_revision.get_version_control_system(REPO)
REV = "default" if VCS == cached_revision.MERCURIAL else "main"

exp = FastDownwardExperiment(environment=ENV)

# Add built-in parsers to the experiment.
exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.TRANSLATOR_PARSER)
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)

exp.add_suite(BENCHMARKS_DIR, SUITE)
exp.add_algorithm("WA", REPO, REV, ["--evaluator", "h=sub(cegar(),2,WA)","--search", "astar(h)"])
exp.add_algorithm("XDP", REPO, REV, ["--evaluator", "h=sub(cegar(),2,XDP)","--search", "astar(h)"])
exp.add_algorithm("XUP", REPO, REV, ["--evaluator", "h=sub(cegar(),2,XUP)","--search", "astar(h)"])
exp.add_algorithm("PWXDP", REPO, REV, ["--evaluator", "h=sub(cegar(),2,PWXDP)","--search", "astar(h)"])

# Add step that writes experiment files to disk.
exp.add_step("build", exp.build)

# Add step that executes all runs.
exp.add_step("start", exp.start_runs)

# Add step that collects properties from run directories and
# writes them to *-eval/properties.
exp.add_fetcher(name="fetch")

algorithm_pairs = [("WA", "XDP", "XUP", "PWXDP")]

# Add report step (ComparativeReport).
exp.add_report(ComparativeReport(algorithm_pairs, attributes=ATTRIBUTES), outfile="report.html")

# Add scatter plot report step.
exp.add_report(ScatterPlotReport(attributes=["expansions"], filter_algorithm=["WA", "XDP", "XUP", "PWXDP"]), outfile="scatterplot.png",)

# Parse the commandline and show or run experiment steps.
exp.run_steps()