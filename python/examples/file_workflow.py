"""Run a release problem/config pair through the file-oriented facade."""

from evoflow import default_problem_data_root, run_files


release_root = default_problem_data_root().parent
result = run_files(
    release_root / "_pdata" / "tsp" / "sample.tsp",
    release_root / "config" / "ACS.cfg",
    seed=42,
    max_fes=1_000,
)
print(
    "tag=",
    result.tag,
    "runs=",
    len(result.rounds),
    "best_fitness=",
    result.best_fitness,
)
