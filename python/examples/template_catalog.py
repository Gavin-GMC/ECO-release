"""List registered templates and load one release-data instance."""

from evoflow import (
    create_problem_template,
    default_problem_data_root,
    discover_problem_instances,
    problem_types,
)


types = problem_types()
print("registered types:", ", ".join(types))

root = default_problem_data_root()
instances = discover_problem_instances(root, types=("TSP",))
if not instances:
    raise RuntimeError("the release does not contain a discoverable TSP instance")

instance = instances[0]
template = create_problem_template(instance.type).load(instance.path)
problem = template.to_problem()
description = problem.describe()
print(
    "loaded:",
    instance.relative_path,
    "type=",
    description.type,
    "dimension=",
    description.dimension,
)
