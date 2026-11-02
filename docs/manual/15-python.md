# 15 · Python Interface

[English](15-python.md) | [简体中文](../手册/15-使用-Python接口.md) | [Manual index](README.md)

EvoFlow provides Python interfaces for problem modeling, configuration, execution control, template/instance discovery, and result analysis. Python is a safe facade over the C++ platform: algorithms, configuration codecs, validation, problem formats, logging, and analysis retain native semantics.

## 1. Release layout and support

```text
EvoFlow-release/
├─ evoflow.exe
├─ install-python.ps1
├─ release-manifest.json
├─ python/
│  ├─ README.md
│  ├─ examples/
│  └─ evoflow/
│     ├─ _core.cp38-win_amd64.pyd
│     ├─ _core.cp39-win_amd64.pyd
│     ├─ _core.cp310-win_amd64.pyd
│     ├─ _core.cp311-win_amd64.pyd
│     ├─ _core.cp312-win_amd64.pyd
│     ├─ *.py / *.pyi
│     └─ py.typed
├─ config/
└─ _pdata/
```

- Validated Windows interpreters: 64-bit CPython 3.8.5, 3.9.23, 3.10.19, 3.11.9, and 3.12.12, corresponding to the five `cp38`–`cp312` modules.
- A native module must match the Python implementation, major/minor version, architecture, and platform. A single `.pyd` does not work across CPython minor versions.
- All five files coexist in one package; Python selects its own extension suffix. Other combinations are not claimed as validated.
- Linux source builds and 16 portable tests were validated on Ubuntu 24.04.4 WSL2 x86_64, GCC 13.3, and CPython 3.12.3. No prebuilt Linux module or compatibility guarantee for other Python minor versions is provided.
- No support for 32-bit Python, PyPy, or non-Windows prebuilt packages.
- NumPy is not required. Ordinary Python numbers and sequences are converted according to the binding contracts; NumPy zero-copy operation is not promised.
- Python is outside CMake. The executable ships alongside the package, not inside `python/`.
- `evoflow.__version__`, CLI, and GUI share the product version. Configuration `v3.1/v3.2` and persisted-file formats have independent versions.
- Release manifests record the native modules' sizes and SHA-256 hashes. The program includes 13 first-party examples; the 701-file full-data overlay is installed separately.

## 2. Installation

For temporary use, run from the release root:

```powershell
$env:PYTHONPATH = "$PWD\python"
python -m evoflow
```

For persistent use, install a `.pth` path file for a specific interpreter:

```powershell
.\install-python.ps1 -PythonExe C:\Path\To\python.exe
python -m evoflow --check-data
```

The installer checks for a matching `_core` using that interpreter's `importlib.machinery.EXTENSION_SUFFIXES`, then verifies an actual import. It does not copy the package. Reinstall the path if you move the release directory. Removal deletes only the path file and works even if the original release directory no longer exists:

```powershell
.\install-python.ps1 -PythonExe C:\Path\To\python.exe -Remove
```

Use `-TargetDirectory <venv>\Lib\site-packages` for a virtual environment. `-PackageRoot` can select a staged build for validation.

## 3. Installation diagnostics

```powershell
python -m evoflow
python -m evoflow --version
python -m evoflow --check-data
python -m evoflow --check-data D:\datasets\EvoFlow\_pdata
```

Diagnostics report package version, Python/ABI, platform, loaded native module, serial/parallel build flag, data path, and registered TYPE values. `--check-data` recomputes file sizes and SHA-256 hashes and reports `missing/minimal/full/incomplete`.

```python
from evoflow import installation_info
info = installation_info()
print(info.native_module, info.python_tag, info.registered_problem_types)
```

Missing/incompatible modules raise `ImportError` with interpreter details, packaged module filenames, build instructions, and the original loader error.

## 4. Five public interfaces

### 4.1 Problem modeling and files

`evoflow.problem` exposes `Problem`, `CompiledProblem`, variable/objective/constraint descriptions, and file-type discovery, description, and loading. `CompiledProblem.is_better(first, second)` uses native objective priorities and minimize/maximize semantics. `format_solution(values, objectives, full=False)` uses the native solution decoder. Both validate sequence lengths and expose no raw comparer/decoder pointers. See [problem modeling](10-problems.md) and `python/examples/problem_model.py`.

### 4.2 Configuration

`evoflow.config` exposes `Config`, workflows, subpopulations, component catalogs, parameter schemas, requirements/provisions, and static/deep validation. See `python/examples/config_model.py`.

`Config.validate()` returns a `ValidationReport` whose `status` is the highest severity: `error / warning / info / pass`. `error_files / warning_files / info_files / pass_files` classify each file by its highest severity. `ConfigFileReport` provides `status` and three issue counts; individual `ConfigIssue.severity` is `error / warning / info`.

`report.ok` is false only for Error or loading failure. Display all issues before deciding whether to block execution. `Workflow.check_component()` returns `errors / warnings / infos`; Info maps to `informational`, Warning to `notfullysatisfied`, and both have `allowed=True`.

### 4.3 Execution

`evoflow.runtime.Optimizer` reuses problem/configuration snapshots; `run` is the one-shot in-memory entry point; `run_files` shares the CLI/App file facade. All accept `optimizer_name=` and safely continue disk result numbering for the same optimizer/problem/tag.

`RunRoundResult.run` is the one-based round within this invocation; `result_index` is the actual file suffix. `RunResult.run_start` is the first actual result index in the batch.

```python
from evoflow import run_files

result = run_files(
    "_pdata/tsp/sample.tsp", "config/ACS.cfg",
    problem_name="sample", optimizer_name="ACS_sweep_01",
    tag="experiment_a", runs=3,
)
print(result.run_start)
print([round.result_index for round in result.rounds])

# Supply both options only when intentionally replacing old results.
replacement = run_files(
    "_pdata/tsp/sample.tsp", "config/ACS.cfg",
    problem_name="sample", optimizer_name="ACS_sweep_01",
    tag="experiment_a", runs=3,
    run_start=0, replace_existing=True,
)
```

With `run_start` alone, any occupied result in the range rejects the whole batch before optimization. `replace_existing=True` requires an explicit start. Replacement stages new files and backs up old `.rslt/.prcs`; errors/cancellation restore them. Progress callbacks report completed rounds, cancellation returns a partial `RunResult`, and `continue_run` is available only after a successful, uncancelled single round. It is not checkpointing or cross-process recovery.

### 4.4 Templates and instance catalogs

`evoflow.templates` exposes 13 registered TYPE values, 12 concrete template families, the release data root, and safe instance discovery. Data remains beside the executable and Python package, not inside the package. The program includes 13 first-party examples; the optional full overlay adds 701 licensed instances. See `template_catalog.py` and `external_benchmark.py` for custom objective callables.

### 4.5 Analysis

`evoflow.analysis` shares native batch discovery, reports, statistics/metrics/comparisons, TXT/CSV/LaTeX export, SVG/PNG charts, and `.prcs` reading. Non-Windows PNG generation requires an explicit readable font file.

`ExperimentBatch.run_numbers` contains actual result indices shared by all selected optimizer/problem combinations, including noncontiguous sets such as `(3, 5)`. `run_diagnostics` lists found/missing indices per cell. `analyze_results.py` demonstrates a minimal persisted batch. Private raw `.rslt` parsing and unimplemented native Excel/Word exports are not exposed as empty stubs.

Common names are also re-exported from the top-level `evoflow` package.

## 5. Types, exceptions, and ownership

- Complete `.pyi` files and `py.typed` cover native classes, methods, properties, enums, module functions, and public facades.
- Validation raises `TypeError`/`ValueError`; execution failures raise `RuntimeError`; analysis-plan/batch errors use `AnalysisError`; ABI/loading failures use `ImportError`.
- `Problem.compile()`, optimizer construction, and configuration copying create owned native snapshots. Later edits or destruction of source objects do not change them.
- Long native operations release the GIL; Python objective, progress, cancellation, and scan callbacks reacquire it. Callbacks preserve correctness but do not promise parallel speedup around the GIL.
- A single `Optimizer` is neither concurrent nor reentrant. Independent instances can be used separately.

## 6. Examples

| File under `python/examples/` | Purpose |
|---|---|
| `problem_model.py` | Define, describe, compile, and evaluate problems |
| `config_model.py` | Build, validate, and save configurations |
| `run_optimizer.py` | In-memory execution, progress, and continuation |
| `template_catalog.py` | TYPE values, instance discovery, and loading |
| `file_workflow.py` | Run problem/configuration files |
| `analyze_results.py` | Discover persisted batches and analyze them |
| `external_benchmark.py` | Integrate external benchmarks as objective callables |

After installation or setting `PYTHONPATH`:

```powershell
python python\examples\file_workflow.py
```

## 7. Source builds

Python builds do not use CMake:

```powershell
.\build-python.ps1 -PythonExe C:\Path\To\python.exe
.\build-python.ps1 -PythonExe C:\Path\To\python.exe -Parallel
```

The serial release module goes into `python/evoflow`. `-Parallel` stages validation output under `obj-python-parallel/python/evoflow` without overwriting release binaries. Requirements: Windows x64, MSVC 2022, 64-bit CPython 3, and the repository's pybind11 source.

Maintainers build and validate the five-interpreter matrix from the development tree:

```powershell
.\build-python-matrix.ps1 `
  -Python38Exe C:\Path\To\Python38\python.exe `
  -Python39Exe C:\Path\To\Python39\python.exe `
  -Python310Exe C:\Path\To\Python310\python.exe `
  -Python311Exe C:\Path\To\Python311\python.exe `
  -Python312Exe C:\Path\To\Python312\python.exe
```

Public packages omit maintainer tests and matrix orchestration, but retain binding source and a single-version build script. Use `-SkipTests` when rebuilding one ABI from such a package. Official binaries require the maintainer matrix: exact CPython 3.8–3.12 Windows x64 paths, full tests per interpreter, exactly five serial modules, and actual imports of each matching tagged file. Parallel binaries are excluded from releases.

Historical validation: the 2.6.8 baseline passed 16 test scripts, 341 checks, six examples, and seven matrix-specific checks per interpreter. In 2.7.4 all five release modules were rebuilt from the final 3.0.0 source, each passing 17 current scripts and 7/7 matrix checks, followed by exact-ABI imports, data verification, file execution, and native analysis in the assembled release tree. These historical counts do not replace the final artifact hashes in `release-manifest.json`.
