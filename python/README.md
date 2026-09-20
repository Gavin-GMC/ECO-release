# EvoFlow Python interface 3.0.1.5

This directory is the prebuilt Python part of an EvoFlow release. It is shipped
beside `evoflow.exe`; the executable is not installed into this directory and
Python is not part of the CMake build.

Run `python -m evoflow --version` to print the package product version. Config
codec versions (`v3.1` and `v3.2`) and persisted result-format versions are
separate protocols and do not follow the product version.

## Supported binaries

The Windows x64 release contains five serial native modules in one package:

```text
_core.cp38-win_amd64.pyd
_core.cp39-win_amd64.pyd
_core.cp310-win_amd64.pyd
_core.cp311-win_amd64.pyd
_core.cp312-win_amd64.pyd
```

They are release-verified with 64-bit CPython 3.8.5, 3.9.23, 3.10.19, 3.11.9,
and 3.12.12 respectively. Python selects the matching tagged module. A native
module must match the interpreter implementation, major and minor version,
word size, and platform; one `.pyd` is not shared across CPython minor versions.

Linux does not ship a prebuilt native module. A source-built CPython 3.12
module was verified on Ubuntu 24.04.4 WSL2 x86_64 with GCC 13.3. This is a
source-build verification, not a binary-compatibility promise. Python remains
outside CMake. PNG output on non-Windows systems requires explicit readable
font paths.

The package has no NumPy dependency. Inputs and outputs use ordinary Python
numbers and sequences; no zero-copy NumPy contract is claimed.

## Use without installation

From the release root in PowerShell:

```powershell
$env:PYTHONPATH = "$PWD\python"
python -m evoflow
python python\examples\problem_model.py
```

## Install a release-path file

The installer adds one `.pth` file to the selected interpreter. It does not copy
or modify the package, so keep the release directory in place.

```powershell
.\install-python.ps1 -PythonExe C:\Path\To\python.exe
python -m evoflow --check-data
```

Pass an explicit data root when the minimal or full package is elsewhere:

```powershell
python -m evoflow --check-data D:\EvoFlowData\_pdata
```

The check reports `missing`, `minimal`, `full`, or `incomplete`, validates the
manifest file count and safe relative paths, and verifies every listed file's
size and SHA-256. The executable provides the faster structural counterpart as
`evoflow data [path]`.

To remove only that path file:

```powershell
.\install-python.ps1 -PythonExe C:\Path\To\python.exe -Remove
```

Use `-TargetDirectory` for a virtual environment or a test directory. Use
`-PackageRoot` only when validating a staged package such as the parallel build.

## Public modules

- `evoflow.problem`: programmatic models and registered problem files.
- `evoflow.templates`: classic template families and `_pdata` discovery.
- `evoflow.config`: optimizer construction, component catalog, and validation.
- `evoflow.runtime`: in-memory and file-based optimization.
- `evoflow.analysis`: persisted result and process-log analysis.
- `evoflow.diagnostics`: loaded binary and release-layout information.

`CompiledProblem.is_better` applies the native objective priority and
minimize/maximize rules to two fitness vectors. `CompiledProblem.format_solution`
uses the native solution decoder for compact or full text.

External benchmark packages or user functions can be used as objective
callables; see `examples/external_benchmark.py`. The specialized CEC template
is not distributed. A native external library should instead be linked by a
user-owned C++ host as shown in `../tutorial/7.external-benchmark.cpp`; the
stock executable and UI do not dynamically load arbitrary native libraries.

The source tree and program package contain one first-party sample per problem
family. Larger redistributable datasets are delivered as a separate full-data
overlay. TSPLIB, QAPLIB, and GK source instances are not redistributed; use the
documented download and conversion workflow in
`../tutorial/external-data/README.md` to add local copies.

The same public names are re-exported from `evoflow`. Complete `.pyi` files and
the `py.typed` marker are included for editors and static type checkers.

Configuration validation reports use four aggregate levels: `error`, `warning`,
`info`, and `pass`. Individual issues use the first three through
`ConfigIssue.severity`; `pass` means that static validation found no issue.
`ValidationReport.ok` becomes false only for an error or load failure, so warnings
and informational findings remain visible without blocking assembly or execution.

Public validation errors remain `TypeError` or `ValueError`; failed native runs
raise `RuntimeError`, report failures raise `AnalysisError`, and an ABI or loader
mismatch raises an explanatory `ImportError`.

## Rebuild from the packaged source

The packaged Windows tree includes `build/python/build-python.ps1` and the binding source.
Because the public package does not carry the maintainer test suite, rebuild one
matching interpreter with `-SkipTests`:

```powershell
.\build\python\build-python.ps1 -PythonExe C:\Path\To\python.exe -SkipTests
```

The maintainer source branch additionally contains `build-python-matrix.ps1`
and `tests/python`. The formal release gate uses all five explicit interpreter
paths, runs the complete suite for every ABI, and retains exactly the five
serial modules:

```powershell
.\build-python-matrix.ps1 `
  -Python38Exe C:\Path\To\Python38\python.exe `
  -Python39Exe C:\Path\To\Python39\python.exe `
  -Python310Exe C:\Path\To\Python310\python.exe `
  -Python311Exe C:\Path\To\Python311\python.exe `
  -Python312Exe C:\Path\To\Python312\python.exe
```

Parallel builds remain validation-only staging artifacts and are not shipped.
