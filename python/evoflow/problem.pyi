import os
from typing import Optional, Union

from ._core import (
    AccumulateConstraint as AccumulateConstraint,
    CompiledProblem as CompiledProblem,
    ConstraintInfo as ConstraintInfo,
    ObjectiveInfo as ObjectiveInfo,
    Problem as Problem,
    ProblemDescription as ProblemDescription,
    VariableInfo as VariableInfo,
    VariableType as VariableType,
)

PathInput = Union[str, bytes, os.PathLike]

def problem_type(path: PathInput) -> str: ...
def describe_problem(path: PathInput, logical_name: Optional[str] = ...) -> ProblemDescription: ...
def load_problem(path: PathInput, logical_name: Optional[str] = ...) -> Problem: ...
