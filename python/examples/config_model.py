"""Build, validate, and save an optimizer configuration."""

from pathlib import Path

from evoflow import Config


config = Config()
config.name = "python_pso"
config.max_fes = 100_000

workflow = config.add_workflow("wf")
workflow.add_topology("PGBest")
workflow.add_repair("Random")
workflow.add_strategy("VelocityDriven", [2, 2, 0.9, 0.4, 0])
workflow.add_generator("OrderedConstruct", [0])
workflow.add_evaluator("Basic")
workflow.add_selector("Index", [0])

subpopulation = config.add_subpopulation("1", size=100, workflow="wf")
subpopulation.max_fes = 100_000

report = config.validate(deep=True)
for issue in report.issues:
    print(f"[{issue.severity}] [{issue.reason}] {issue.message}")
if not report.ok:
    raise SystemExit(1)

output = Path("python-pso.cfg")
config.save(output)
print(f"Saved {output} as {config.source_version}")
