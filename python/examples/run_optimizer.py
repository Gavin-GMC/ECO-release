"""Build and run a small in-memory optimization problem."""

from evoflow import Config, Optimizer, Problem, VariableType


problem = Problem("python_sphere")
problem.add_variable("x", -5.0, 5.0, 0.001, 5, type=VariableType.CONTINUOUS)
problem.add_objective(
    "sphere", "x", lambda values: sum(value * value for value in values)
)

config = Config()
config.name = "python_sphere"
config.tag = "python_example"
config.max_fes = 1000
workflow = config.add_workflow("pso")
workflow.set_initializer("Random")
workflow.add_topology("PGBest")
workflow.add_repair("Random")
workflow.add_strategy("VelocityDriven", [2, 1.5, 0.4, 0, 0])
workflow.add_generator("OrderedConstruct", [0])
workflow.add_evaluator("Basic")
workflow.add_selector("Index", [0])
subpopulation = config.add_subpopulation("1", size=40, workflow="pso")
subpopulation.max_fes = 1000

optimizer = Optimizer(problem, config)
result = optimizer.run(
    seed=42,
    progress=lambda completed, total: print(f"completed {completed}/{total}"),
)
print(f"tag={result.tag} best={result.best_fitness}")

# Continue from the current population without reinitializing it.
continued = optimizer.continue_run(max_fes=500)
print(f"continued best={continued.best_fitness}")
