"""Connect a benchmark supplied by an external Python package or module."""

import math

from evoflow import Config, Optimizer, Problem, VariableType


class ExternalBenchmark:
    """Stand-in for an object imported from a separately licensed package."""

    def __init__(self, shift):
        self.shift = float(shift)

    def evaluate(self, values):
        return sum((value - self.shift) ** 2 for value in values)


benchmark = ExternalBenchmark(1.25)
problem = Problem("python-external-benchmark")
problem.add_variable("x", -5.0, 5.0, 0.001, 5, type=VariableType.CONTINUOUS)
problem.add_objective("fitness", "x", benchmark.evaluate, minimize=True)

config = Config()
config.name = "python_external"
config.max_fes = 1000
workflow = config.add_workflow("pso")
workflow.set_initializer("Random")
workflow.add_topology("PGBest")
workflow.add_repair("Boundary")
workflow.add_strategy("VelocityDriven", [2, 2.0, 0.7, 0.00025, 0])
workflow.add_generator("OrderedConstruct", [0])
workflow.add_evaluator("Basic")
workflow.add_selector("Index", [0])
subpopulation = config.add_subpopulation("1", size=20, workflow="pso")
subpopulation.max_fes = 1000

result = Optimizer(problem, config).run(seed=42)
print(result.best_fitness)
if not math.isfinite(result.best_fitness) or result.best_fitness >= 1.0:
    raise RuntimeError("external benchmark example did not converge")
