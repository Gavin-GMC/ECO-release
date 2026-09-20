"""Build, inspect, compile, and evaluate a small EvoFlow problem."""

from evoflow import Problem, VariableType


problem = Problem("assignment-demo")
problem.add_constant("cost", [4.0, 1.0, 3.0])
problem.add_variable("assignment", 0, 2, 1, 3, type=VariableType.ALLOCATION)
problem.add_objective(
    "total_cost",
    ["assignment", "cost"],
    lambda assignment, cost: sum(cost[int(index)] for index in assignment),
)
problem.add_unique_constraint("assignment")
problem.add_random_heuristic("assignment")

print(problem.describe().dimension)
compiled = problem.compile()
solution = compiled.greedy_solution()
print(solution, compiled.evaluate(solution), compiled.is_legal(solution))
