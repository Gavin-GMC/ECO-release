"""Optimization execution API backed by the EvoFlow C++ runtime."""

from __future__ import annotations

import os
from typing import Any, Callable, Optional, Union

from . import _core
from .config import Config
from .problem import CompiledProblem, Problem


ProgressCallback = Callable[[int, int], Any]
CancellationCallback = Callable[[], bool]
PathInput = Union[str, os.PathLike]

RunSolution = _core.RunSolution
RunRoundResult = _core.RunRoundResult
RunResult = _core.RunResult


def _compiled(problem: Union[Problem, CompiledProblem]) -> CompiledProblem:
    if isinstance(problem, Problem):
        return problem.compile()
    if isinstance(problem, CompiledProblem):
        return problem
    raise TypeError("problem must be a Problem or CompiledProblem")


def _path(value: PathInput, argument: str) -> str:
    try:
        return os.fsdecode(os.fspath(value))
    except TypeError as error:
        raise TypeError(f"{argument} must be a path-like object") from error


def _file_budget(value: Optional[int], argument: str) -> int:
    if value is None:
        return -1
    if isinstance(value, bool) or not isinstance(value, int):
        raise TypeError(f"{argument} must be an integer or None")
    if value < 0:
        raise ValueError(f"{argument} must be non-negative or None")
    return value


def _run_start(value: Optional[int]) -> Optional[int]:
    if value is None:
        return None
    if isinstance(value, bool) or not isinstance(value, int):
        raise TypeError("run_start must be an integer or None")
    if value < 0:
        raise ValueError("run_start must be non-negative or None")
    return value


class Optimizer:
    """A reusable optimizer assembled from snapshots of a problem and config."""

    def __init__(
        self,
        problem: Union[Problem, CompiledProblem],
        config: Config,
        *,
        tag: Optional[str] = None,
        optimizer_name: Optional[str] = None,
        max_fes: Optional[int] = None,
        max_convergence: Optional[int] = None,
        max_time: Optional[int] = None,
        verbose: bool = False,
    ) -> None:
        if not isinstance(config, Config):
            raise TypeError("config must be a Config")
        self._native = _core._Optimizer(
            _compiled(problem),
            config,
            tag=tag,
            optimizer_name=optimizer_name,
            max_fes=max_fes,
            max_convergence=max_convergence,
            max_time=max_time,
            verbose=verbose,
        )

    @property
    def tag(self) -> str:
        """The actual run tag, including a generated tag when config.tag is empty."""

        return self._native.tag

    @property
    def optimizer_name(self) -> str:
        """The stable optimizer identity used in result and process paths."""

        return self._native.optimizer_name

    def run(
        self,
        *,
        seed: int = 1,
        runs: int = 1,
        run_start: Optional[int] = None,
        replace_existing: bool = False,
        progress: Optional[ProgressCallback] = None,
        cancelled: Optional[CancellationCallback] = None,
    ) -> RunResult:
        """Run one or more independent initializations on one native RNG stream."""

        if not isinstance(replace_existing, bool):
            raise TypeError("replace_existing must be a bool")
        run_start = _run_start(run_start)
        if replace_existing and run_start is None:
            raise ValueError("replace_existing=True requires an explicit run_start")
        return self._native.run(
            seed=seed, runs=runs, run_start=run_start,
            replace_existing=replace_existing, progress=progress,
            cancelled=cancelled
        )

    def continue_run(
        self,
        *,
        max_fes: Optional[int] = None,
        max_convergence: Optional[int] = None,
        max_time: Optional[int] = None,
    ) -> RunResult:
        """Continue the state left by a successful, non-cancelled single run."""

        return self._native.continue_run(
            max_fes=max_fes,
            max_convergence=max_convergence,
            max_time=max_time,
        )


def run(
    problem: Union[Problem, CompiledProblem],
    config: Config,
    *,
    seed: int = 1,
    runs: int = 1,
    tag: Optional[str] = None,
    optimizer_name: Optional[str] = None,
    run_start: Optional[int] = None,
    replace_existing: bool = False,
    max_fes: Optional[int] = None,
    max_convergence: Optional[int] = None,
    max_time: Optional[int] = None,
    verbose: bool = False,
    progress: Optional[ProgressCallback] = None,
    cancelled: Optional[CancellationCallback] = None,
) -> RunResult:
    """Build an :class:`Optimizer` and execute it once."""

    return Optimizer(
        problem,
        config,
        tag=tag,
        optimizer_name=optimizer_name,
        max_fes=max_fes,
        max_convergence=max_convergence,
        max_time=max_time,
        verbose=verbose,
    ).run(seed=seed, runs=runs, run_start=run_start,
          replace_existing=replace_existing, progress=progress,
          cancelled=cancelled)


def run_files(
    problem_path: PathInput,
    config_path: PathInput,
    *,
    problem_name: Optional[str] = None,
    optimizer_name: Optional[str] = None,
    seed: int = 1,
    runs: int = 1,
    tag: Optional[str] = None,
    run_start: Optional[int] = None,
    replace_existing: bool = False,
    max_fes: Optional[int] = None,
    max_convergence: Optional[int] = None,
    max_time: Optional[int] = None,
    verbose: bool = False,
    progress: Optional[ProgressCallback] = None,
    cancelled: Optional[CancellationCallback] = None,
) -> RunResult:
    """Run existing problem/config files through the shared C++ App facade."""

    options = _core.RunOptions()
    options.problem_path = _path(problem_path, "problem_path")
    options.config_path = _path(config_path, "config_path")
    options.problem_name = "" if problem_name is None else problem_name
    options.optimizer_name = "" if optimizer_name is None else optimizer_name
    options.tag = "" if tag is None else tag
    options.seed = seed
    options.runs = runs
    checked_run_start = _run_start(run_start)
    if not isinstance(replace_existing, bool):
        raise TypeError("replace_existing must be a bool")
    if replace_existing and checked_run_start is None:
        raise ValueError("replace_existing=True requires an explicit run_start")
    options.run_start = -1 if checked_run_start is None else checked_run_start
    options.replace_existing = replace_existing
    options.max_fes = _file_budget(max_fes, "max_fes")
    options.max_convergence = _file_budget(max_convergence, "max_convergence")
    options.max_time = _file_budget(max_time, "max_time")
    options.verbose = verbose

    result = _core._run_optimization(
        options, progress=progress, cancelled=cancelled
    )
    if result.cancelled:
        return result
    if not result.ok:
        raise RuntimeError(result.error or "optimization failed")
    return result


__all__ = [
    "CancellationCallback",
    "Optimizer",
    "ProgressCallback",
    "RunResult",
    "RunRoundResult",
    "RunSolution",
    "run",
    "run_files",
]
