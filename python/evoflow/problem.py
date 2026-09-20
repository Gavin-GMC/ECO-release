"""Problem modeling and file-loading API backed by EvoFlow C++."""

import os

from ._core import (
    AccumulateConstraint,
    CompiledProblem,
    ConstraintInfo,
    ObjectiveInfo,
    Problem,
    ProblemDescription,
    VariableInfo,
    VariableType,
    _describe_problem_file,
    _load_problem_file,
    _problem_type_file,
)


def _path(value):
    try:
        return os.fsdecode(os.fspath(value))
    except TypeError:
        raise TypeError("path must be str, bytes, or os.PathLike")


def _logical_name(value):
    if value is None:
        return ""
    if not isinstance(value, str):
        raise TypeError("logical_name must be str or None")
    if not value:
        raise ValueError("logical_name cannot be empty")
    return value


def problem_type(path):
    """Return the native ``TYPE:`` value from a problem file."""
    return _problem_type_file(_path(path))


def describe_problem(path, logical_name=None):
    """Describe a file-backed problem through ``App::describeProblem``."""
    return _describe_problem_file(_path(path), _logical_name(logical_name))


def load_problem(path, logical_name=None):
    """Load a registered file-backed problem as an owned :class:`Problem`."""
    return _load_problem_file(_path(path), _logical_name(logical_name))

__all__ = [
    "AccumulateConstraint",
    "CompiledProblem",
    "ConstraintInfo",
    "ObjectiveInfo",
    "Problem",
    "ProblemDescription",
    "VariableInfo",
    "VariableType",
    "describe_problem",
    "load_problem",
    "problem_type",
]
