#! /usr/bin/env python

from pathlib import Path
import sys

from lab.experiment import Experiment

from downward.reports.absolute import AbsoluteReport
from downward.reports.compare import ComparativeReport
from downward.reports.scatter import ScatterPlotReport
from lab.environments import BaselSlurmEnvironment, LocalEnvironment

import project

DIR = Path(__file__).resolve().parent

ATTRIBUTES = [
    "error",
    "expansions",
    "run_dir",
    "total_time",
    "coverage",
    "cost",
]

SUITE = ["depot:p01.pddl", "gripper:prob01.pddl", "mystery:prob07.pddl"]
ENV = BaselSlurmEnvironment(email="caroline.steiblin@stud.unibas.ch")

exp = Experiment()
exp.add_step(
    "remove-combined-properties",
    project.remove_file,
    Path(exp.eval_dir) / "properties")

"""
Fetch and rename a single algorithm.
"""
assert not expname.rstrip("/").endswith("-eval")
new_algo = new_algo or algo

def algo_filter(run):
    if run["algorithm"] == algo:
        run["algorithm"] = new_algo
        run["id"][0] = run["id"][0].replace(algo, new_algo)
        return run
    return False

exp.add_fetcher(
    os.path.join("data", expname + "-eval"),
    filter=algo_filter,
    name="fetch-{new_algo}-from-{expname}".format(**locals()),
    merge=True,
)

for expname, searchname in [("priority", "astar"), ("iospriority", "ios")]:
    for func in ["XUP", "XDP", "WA", "PWXDP"]:
        fetch_algorithm(exp, expname, algo=func, new_algo=f"{searchname}-{func}")

#algorithms = ["ASTAR", "WA", "XDP", "XUP", "PWXDP", "IOSWA", "IOSXDP", "IOSXUP", "IOSPWXDP"]
#for algo1, algo2 in itertools.combinations(algorithms, 2):

# Add report step (ComparativeReport).
exp.add_report(AbsoluteReport(attributes=ATTRIBUTES), outfile="absolute.html")
#exp.add_report(ComparativeReport(algorithm_pairs, attributes=ATTRIBUTES), outfile="compare.html")

# Add scatter plot report step.

#def omit_parcprinter(run):
    #return "parcprinter" not in run["domain"]

# exp.add_report(ScatterPlotReport(relative=True, attributes=["cost"], filter_algorithm=["WA", "IOSWA"], filter=omit_parcprinter), outfile="scatterplot_cost_wa_xdp.png",)
# exp.add_report(ScatterPlotReport(relative=True, attributes=["cost"], filter_algorithm=["XDP", "IOSXDP"], filter=omit_parcprinter), outfile="scatterplot_cost_wa_xup.png",)
# exp.add_report(ScatterPlotReport(relative=True, attributes=["cost"], filter_algorithm=["XUP", "IOSXUP"], filter=omit_parcprinter), outfile="scatterplot_cost_wa_pwxdp.png",)
# exp.add_report(ScatterPlotReport(relative=True, attributes=["cost"], filter_algorithm=["PWXDP", "IOSPWXDP"], filter=omit_parcprinter), outfile="scatterplot_cost_xdp_xup.png",)

# exp.add_report(ScatterPlotReport(relative=True, attributes=["total_time"], filter_algorithm=["WA", "IOSWA"]), outfile="scatterplot_time_wa_xdp.png",)
# exp.add_report(ScatterPlotReport(relative=True, attributes=["total_time"], filter_algorithm=["XDP", "IOSXDP"]), outfile="scatterplot_time_wa_xup.png",)
# exp.add_report(ScatterPlotReport(relative=True, attributes=["total_time"], filter_algorithm=["XUP", "IOSXUP"]), outfile="scatterplot_time_wa_pwxdp.png",)
# exp.add_report(ScatterPlotReport(relative=True, attributes=["total_time"], filter_algorithm=["PWXDP", "IOSPWXDP"]), outfile="scatterplot_time_xdp_xup.png",)

# exp.add_report(ScatterPlotReport(attributes=["expansions"], filter_algorithm=["WA", "IOSWA"]), outfile="scatterplot_expansions_wa_xdp.png",)
# exp.add_report(ScatterPlotReport(attributes=["expansions"], filter_algorithm=["XDP", "IOSXDP"]), outfile="scatterplot_expansions_wa_xup.png",)
# exp.add_report(ScatterPlotReport(attributes=["expansions"], filter_algorithm=["XUP", "IOSXUP"]), outfile="scatterplot_expansions_wa_pwxdp.png",)
# exp.add_report(ScatterPlotReport(attributes=["expansions"], filter_algorithm=["PWXDP", "IOSPWXDP"]), outfile="scatterplot_expansions_xdp_xup.png",)



exp.run_steps()