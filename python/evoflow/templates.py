"""Classic problem templates and release-data discovery."""

import os
from pathlib import Path

from ._core import (
    CFLPTemplate,
    CPMPHeuristic,
    CPMPTemplate,
    CVRPSplitMode,
    CVRPTemplate,
    DistanceType,
    FJSPSequenceHeuristic,
    FJSPSequenceMode,
    FJSPTemplate,
    MDSTemplate,
    MISTemplate,
    MKPTemplate,
    ProblemInstance,
    ProblemTemplate,
    QAPTemplate,
    RoundType,
    SRFunction,
    SRObjectiveMode,
    SRTemplate,
    STPHeuristic,
    STPTemplate,
    TSPTemplate,
    WFSMappingHeuristic,
    WFSObjectiveMode,
    WFSSequenceHeuristic,
    WFSSequenceMode,
    WFSTemplate,
    _create_problem_template,
    _discover_problem_instances,
    _problem_template_types,
)


def _path(value):
    try:
        return os.fsdecode(os.fspath(value))
    except TypeError:
        raise TypeError("root must be str, bytes, or os.PathLike")


def default_problem_data_root():
    """Return the nearest release-tree ``_pdata`` directory above the package."""
    package_file = Path(__file__).resolve()
    release_parent = package_file.parents[2]
    for parent in list(package_file.parents)[2:6]:
        candidate = parent / "_pdata"
        if candidate.is_dir():
            return candidate
    return release_parent / "_pdata"


def problem_types():
    """Return the exact TYPE strings registered by C++."""
    return tuple(_problem_template_types())


def create_problem_template(type):
    """Create a generic native template through ``ProblemTemplateRegistry``."""
    if not isinstance(type, str):
        raise TypeError("type must be str")
    if not type:
        raise ValueError("type cannot be empty")
    return _create_problem_template(type)


def discover_problem_instances(root=None, types=None):
    """Discover registered, loadable problem files under a bounded data root."""
    if root is None:
        root = default_problem_data_root()
    if types is not None:
        if isinstance(types, str):
            raise TypeError("types must be an iterable of exact TYPE strings, not str")
        types = tuple(types)
    return tuple(_discover_problem_instances(_path(root), types))


__all__ = [
    "CFLPTemplate",
    "CPMPHeuristic",
    "CPMPTemplate",
    "CVRPSplitMode",
    "CVRPTemplate",
    "DistanceType",
    "FJSPSequenceHeuristic",
    "FJSPSequenceMode",
    "FJSPTemplate",
    "MDSTemplate",
    "MISTemplate",
    "MKPTemplate",
    "ProblemInstance",
    "ProblemTemplate",
    "QAPTemplate",
    "RoundType",
    "SRFunction",
    "SRObjectiveMode",
    "SRTemplate",
    "STPHeuristic",
    "STPTemplate",
    "TSPTemplate",
    "WFSMappingHeuristic",
    "WFSObjectiveMode",
    "WFSSequenceHeuristic",
    "WFSSequenceMode",
    "WFSTemplate",
    "create_problem_template",
    "default_problem_data_root",
    "discover_problem_instances",
    "problem_types",
]
