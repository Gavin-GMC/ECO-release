"""Command-line diagnostics for ``python -m evoflow``."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Optional, Sequence

from .diagnostics import installation_info
from .templates import discover_problem_instances


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _verify_manifest(root: Path, filename: str):
    path = root / filename
    if not path.is_file():
        return None, 0, ["missing {}".format(filename)]
    try:
        manifest = json.loads(path.read_text(encoding="utf-8-sig"))
    except (OSError, ValueError) as error:
        return None, 0, ["cannot read {}: {}".format(filename, error)]
    if manifest.get("schema") != "evoflow-data-manifest-v1":
        return None, 0, ["unsupported schema in {}".format(filename)]
    files = manifest.get("files")
    expected = manifest.get("instance_count")
    if not isinstance(files, list) or expected != len(files):
        return None, 0, ["invalid file count in {}".format(filename)]
    errors = []
    verified = 0
    for item in files:
        relative = item.get("path") if isinstance(item, dict) else None
        if not isinstance(relative, str):
            errors.append("invalid path entry in {}".format(filename))
            continue
        relative_path = Path(relative)
        if relative_path.is_absolute() or ".." in relative_path.parts:
            errors.append("unsafe manifest path: {}".format(relative))
            continue
        candidate = root / relative_path
        if not candidate.is_file():
            errors.append("missing {}".format(relative))
            continue
        size = candidate.stat().st_size
        if size != item.get("bytes"):
            errors.append("size mismatch: {}".format(relative))
            continue
        digest = _sha256(candidate)
        if digest != item.get("sha256"):
            errors.append("SHA-256 mismatch: {}".format(relative))
            continue
        verified += 1
    return manifest, verified, errors


def _check_data(root: Path) -> int:
    if not root.is_dir():
        print("Data package: missing")
        print("Data error: data root is not a directory")
        return 1

    minimal, minimal_verified, minimal_errors = _verify_manifest(
        root, "manifest-minimal.json"
    )
    full_path = root / "manifest-full.json"
    full = None
    full_verified = 0
    full_errors = []
    if full_path.exists():
        full, full_verified, full_errors = _verify_manifest(root, "manifest-full.json")

    errors = minimal_errors + full_errors
    if minimal is None or errors:
        status = "incomplete"
    elif full is not None:
        status = "full"
    else:
        status = "minimal"
    print("Data package: {}".format(status))
    expected = 0
    if minimal is not None:
        expected += minimal["instance_count"]
    if full is not None:
        expected += full["instance_count"]
    print("Manifest files verified: {}/{}".format(
        minimal_verified + full_verified, expected
    ))
    try:
        instances = discover_problem_instances(root)
        print("Loadable problem instances: {}".format(len(instances)))
    except RuntimeError as error:
        errors.append("instance discovery failed: {}".format(error))
    if errors:
        for error in errors[:20]:
            print("Data error: {}".format(error))
        if len(errors) > 20:
            print("Data error: {} additional issue(s)".format(len(errors) - 20))
    return 0 if status in {"minimal", "full"} and not errors else 1


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(description="Inspect an EvoFlow Python installation")
    parser.add_argument("--version", action="store_true", help="print only the EvoFlow version")
    parser.add_argument(
        "--check-data",
        nargs="?",
        const="",
        metavar="PATH",
        help="verify the default or supplied _pdata tree against its manifests",
    )
    options = parser.parse_args(argv)
    info = installation_info()
    if options.version:
        print(info.version)
        return 0

    print("EvoFlow {}".format(info.version))
    print("Python: {} {} ({}, {}-bit)".format(
        info.implementation,
        info.python_version,
        info.python_tag,
        info.pointer_bits,
    ))
    print("Platform: {}".format(info.platform_tag))
    print("Native module: {}".format(info.native_module))
    print("Parallel subpop build: {}".format(info.parallel_subpop))
    print("Data root: {} ({})".format(
        info.data_root, "available" if info.data_available else "missing"
    ))
    print("Problem TYPE values: {}".format(", ".join(info.registered_problem_types)))
    if options.check_data is not None:
        root = info.data_root if options.check_data == "" else Path(options.check_data)
        return _check_data(root.resolve())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
