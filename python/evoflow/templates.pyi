import os
from pathlib import Path
from typing import Iterable, Optional, Tuple, Union

from ._core import (
    CFLPTemplate as CFLPTemplate,
    CPMPHeuristic as CPMPHeuristic,
    CPMPTemplate as CPMPTemplate,
    CVRPSplitMode as CVRPSplitMode,
    CVRPTemplate as CVRPTemplate,
    DistanceType as DistanceType,
    FJSPSequenceHeuristic as FJSPSequenceHeuristic,
    FJSPSequenceMode as FJSPSequenceMode,
    FJSPTemplate as FJSPTemplate,
    MDSTemplate as MDSTemplate,
    MISTemplate as MISTemplate,
    MKPTemplate as MKPTemplate,
    ProblemInstance as ProblemInstance,
    ProblemTemplate as ProblemTemplate,
    QAPTemplate as QAPTemplate,
    RoundType as RoundType,
    SRFunction as SRFunction,
    SRObjectiveMode as SRObjectiveMode,
    SRTemplate as SRTemplate,
    STPHeuristic as STPHeuristic,
    STPTemplate as STPTemplate,
    TSPTemplate as TSPTemplate,
    WFSMappingHeuristic as WFSMappingHeuristic,
    WFSObjectiveMode as WFSObjectiveMode,
    WFSSequenceHeuristic as WFSSequenceHeuristic,
    WFSSequenceMode as WFSSequenceMode,
    WFSTemplate as WFSTemplate,
)

PathInput = Union[str, bytes, os.PathLike]

def default_problem_data_root() -> Path: ...
def problem_types() -> Tuple[str, ...]: ...
def create_problem_template(type: str) -> ProblemTemplate: ...
def discover_problem_instances(root: Optional[PathInput] = ..., types: Optional[Iterable[str]] = ...) -> Tuple[ProblemInstance, ...]: ...
