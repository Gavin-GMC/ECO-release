"""Installation and release-layout diagnostics for EvoFlow Python."""

from __future__ import annotations

import struct
import sys
import sysconfig
from dataclasses import dataclass
from pathlib import Path
from typing import Tuple

from . import _core
from .templates import default_problem_data_root, problem_types


@dataclass(frozen=True)
class InstallationInfo:
    """A value snapshot describing the loaded Python/native release pair."""

    version: str
    implementation: str
    python_version: str
    python_tag: str
    platform_tag: str
    pointer_bits: int
    native_module: Path
    parallel_subpop: bool
    data_root: Path
    data_available: bool
    registered_problem_types: Tuple[str, ...]


def installation_info() -> InstallationInfo:
    """Return deterministic diagnostics without scanning the full data tree."""

    data_root = default_problem_data_root()
    return InstallationInfo(
        version=_core.__version__,
        implementation=sys.implementation.name,
        python_version="{}.{}.{}".format(*sys.version_info[:3]),
        python_tag=sys.implementation.cache_tag or "unknown",
        platform_tag=sysconfig.get_platform(),
        pointer_bits=8 * struct.calcsize("P"),
        native_module=Path(_core.__file__).resolve(),
        parallel_subpop=bool(_core.parallel_subpop_build),
        data_root=data_root,
        data_available=data_root.is_dir(),
        registered_problem_types=problem_types(),
    )


__all__ = ["InstallationInfo", "installation_info"]
