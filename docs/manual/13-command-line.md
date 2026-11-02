# 13 · Command-Line Executable

[English](13-command-line.md) | [简体中文](../手册/13-使用-可执行程序.md) | [Manual index](README.md)

Use EvoFlow as a standalone command-line tool: load a problem and an optimizer configuration from files, run optimization, and write results. No C++ program is required. Chapters 10–12 describe embedding the library in your own program; this chapter describes the compiled executable.

## 1. Building and installing

Windows x64 users can extract `EvoFlow-3.0.1.5-windows-x64.zip` and run the included `evoflow.exe`. The package also contains public source code, CMake projects, configurations, tutorials, documentation, and 13 first-party minimal examples. `release-manifest.json` records file sizes and SHA-256 hashes; the ZIP has a separate `.sha256` file.

Windows x64 with MSVC is the official release build. The Linux core and CLI have been built and tested on Ubuntu 24.04.4 WSL2 x86_64 with GCC 13.3, CMake 3.28.3, and Ninja 1.11.1. No prebuilt Linux release is provided.

```sh
cmake -S . -B obj-cmake-release
cmake --build obj-cmake-release --config Release
```

Alternatively, use `cmake -S build/cmake -B obj-cmake-release`, or open `build/vs/evoflow.sln` in Visual Studio 2022. See [build instructions](../../build/README.md) (Chinese).

The output `evoflow` or `evoflow.exe` is placed in the project root, alongside `src/`, `_pdata/`, and `config/`. Keep generated files in disposable `obj-*` directories; `build/` contains distributable project files. The Windows executable uses the static `/MT` runtime and does not require a separate Visual C++ Redistributable. It still uses Windows system APIs; this does not imply support for every Windows version or architecture.

## 2. Usage

Since v2.3.1.1, optimization requires the `run` subcommand. The former `evoflow <problem> <config>` syntax is invalid. Running without arguments, including double-clicking the executable, starts the GUI. To migrate an old script, insert `run` before the problem path; the other options retain their behavior.

```text
evoflow                                    Start the GUI
evoflow run <problem-file> <config> [options] Run optimization
evoflow validate <path> [options]           Validate configurations
evoflow report <log-directory> [options]    Analyze an experiment batch
evoflow data [data-directory]               Check minimal/full data packages
evoflow --version                          Display the product version
evoflow -h | --help                         Display help
```

Exit codes: `0` success, `1` runtime failure, `2` invalid usage or an unimplemented subcommand.

### 2.1 GUI

Run without arguments or double-click `evoflow.exe`. A local service binds to a dynamically assigned port on `127.0.0.1` and opens the default browser. Requests use a token generated for that launch; the service does not listen on a LAN address.

The GUI supports problem-file loading and definition metadata, form and block optimizer editors, opening/saving/validating v3.1/v3.2 `.cfg` files, experiment discovery, statistical tables, and SVG/PNG charts. It can also start optimization through the same App facade as the CLI. See the [GUI guide](14-gui.md).

The GUI service currently depends on Windows APIs. Linux validation of `run`, `validate`, and `report` does not imply Linux GUI or model-service support.

### 2.2 `run`

| Argument | Meaning |
|---|---|
| `<problem-file>` | EvoFlow instance file with a `TYPE:` header; the loader dispatches to the registered problem family. See `_pdata/<kind>/README.md` for formats. |
| `<config>` | Optimizer `.cfg` in v3.1 text or v3.2 JSON format, detected from the first line. See `tutorial/config-generate*.cpp`. |

| Option | Meaning | Default |
|---|---|---|
| `-s <seed>` | RNG seed | `1` |
| `-n <runs>` | Independent runs; each is recorded in `.rslt`, while the console reports the last run's best result | `1` |
| `--tag <tag>` | Experiment batch identifier; overrides the configuration tag and becomes part of result filenames | Configuration tag |
| `--optimizer-name <name>` | Override the optimizer name to distinguish parameter-sweep variants; 1–128 ASCII letters, digits, or `._-+` | Configuration name |
| `--run-start <N>` | Explicit first zero-based result index; the entire range is checked before optimization | Continue after the largest occupied index |
| `--replace-existing` | Transactionally replace existing `.rslt/.prcs` in the requested range; requires `--run-start`; failure/cancellation restores old files | Off |
| `-o <file>` | Write the final result to a file | stdout |
| `--max-fes <N>` | Override the total evaluation budget, globally and for all subpopulations | Configuration |
| `--max-time <seconds>` | Override the time limit | Configuration |
| `--max-conv <N>` | Override the maximum number of stagnant generations | Configuration |
| `-q, --quiet` | Print only the best fitness, suitable for scripts | Off |
| `-v, --verbose` | Echo progress every generation | Off |
| `-h, --help` | Display help | — |

If neither the command nor the configuration supplies a tag, the framework falls back to a timestamp. Separate invocations then belong to different batches. Supply a shared tag when results will be analyzed together.

Exit codes: `0` normal completion, `1` runtime error, `2` invalid arguments.

### 2.3 `validate`

Validation reports all detected issues rather than stopping at the first error. Severity levels are Error, Warning, Info, and Pass. Warning/Info do not block assembly; see [component contracts](11-optimizer-configuration.md#component-contracts).

```sh
evoflow validate config
evoflow validate config/ACS.cfg
evoflow validate config -r --build
evoflow validate config -f json -o rep.json
```

| Option | Meaning |
|---|---|
| `<path>` | A `.cfg` file or directory containing configuration files |
| `-r, --recursive` | Recurse into subdirectories; otherwise scan one level |
| `--build` | Also assemble objects, without running optimization; catches errors such as a subpopulation referring to a nonexistent workflow |
| `-f, --format txt\|json` | Output format; default `txt` |
| `-o <file>` | Output file; default stdout |
| `-q, --quiet` | Print only the summary |

Exit codes: `0` no Error (Warning/Info allowed), `1` Error or path/loading failure, `2` invalid arguments.

```text
scanned: 1   pass: 0   info: 0   warning: 0   error: 1   status: error   depth: static

bad.cfg
  [error] workflow 'wf' [scope 0] . component[2] strategy 'NoSuchStrategyXYZ'
         unknown_tag: unknown strategy tag: 'NoSuchStrategyXYZ'
  [error] workflow 'wf' [scope 0] . component[3] generator 'ParallelConstruct'
         no_strategy_before_generator: generator 'ParallelConstruct': no strategy set before it
```

An unknown tag can cause multiple dependent issues; validation reports them together, whereas `run` stops at the first Error. Text output uses `[error]`, `[warn ]`, and `[info ]`. JSON exposes report/file `status`, file counts for the four levels, and issue `severity`. `pass` is an aggregate state only.

Static validation checks parameter arity, ranges and integrality, assertions, tags, and workflow completeness. Deep validation additionally checks assembly. `reason` values such as `unknown_tag`, `missing_provision`, and `param_out_of_range` are stable machine-readable keys; tools should use them rather than parse messages. See `src/optimizer/config/assert-matcher.h`.

### 2.4 `report`

Summarize results under `_log/` by experiment batch, optionally producing charts. The positional log directory defaults to `_log`.

| Option | Meaning | Default |
|---|---|---|
| `--list` | List discovered batches without a report | — |
| `--tag <tag>` | Select the batch to analyze; required for a report | — |
| `--charts <directory>` | Generate a three-line table (`table`), multiple box plots (`dist`), and average-rank bar chart (`rank`) | No charts |
| `--png` | Produce PNG rather than SVG; needs an available system font | SVG |
| `-f, --format <format>` | `txt`, `csv`, `latex`, or `json` | `txt` |
| `-o <file>` | Output file | stdout |

Logs are not automatically deleted. Historical batches may use different problems, budgets, or code versions; combining their means and ranks is not meaningful. Without `--tag`, the program lists candidates instead of choosing one.

```text
$ evoflow report _log --list
experiment batches under the log directory (tag = one batch):

  --tag exp1
      optimizers: 2   problems: 1   runs each: 3   files: 6

pick one with --tag; results from different batches are NOT comparable
```

`ragged` indicates that optimizer/problem combinations have unequal run counts.

```sh
evoflow run _pdata/tsp/sample.tsp config/ACS.cfg --tag exp1 -n 30
evoflow run _pdata/tsp/sample.tsp config/AS.cfg --tag exp1 -n 30
evoflow report _log --tag exp1 --charts _analysis/exp1
```

The report includes mean, standard deviation, median, best-value highlighting, and average ranks. It uses the same table buffer as the analysis API's `report(Format)`.

Result identity consists of optimizer name, problem name, tag, and result index. Repeated invocations with the same identity continue numbering safely: three runs produce `0–2`; two more produce `3–4`. Give each parameter-sweep variant its own `--optimizer-name`.

An explicit start that collides with an existing result fails before optimization. To intentionally repeat indices `10–14`, use `--run-start 10 --replace-existing -n 5`. Old result/history files are backed up and replaced only if the batch succeeds. Execution/error `.log/.err` files remain append-only audit records.

### 2.5 `data`

```sh
evoflow data
evoflow data D:\datasets\EvoFlow\_pdata
```

`minimal` means all 13 first-party examples are present; `full` additionally includes the 701-instance overlay. `missing` and `incomplete` return exit code `1`. The native command checks manifest membership, sizes, and instance counts. Use `python -m evoflow --check-data [path]` to recompute SHA-256 hashes. See [data packages](../数据包与外部数据.md) (Chinese).

## 3. Examples

```sh
# First-party TSP example with ACS and seed 42
./evoflow run _pdata/tsp/sample.tsp config/ACS.cfg -s 42
# Override the budget and print only fitness
./evoflow run _pdata/qap/sample.qap config/ACS.cfg --max-fes 50000 -q
# Write the final result
./evoflow run _pdata/tsp/sample.tsp config/ACS.cfg -o result.txt
# Distinct identity for a parameter-sweep variant
./evoflow run _pdata/tsp/sample.tsp config/ACS.cfg --tag sweep --optimizer-name ACS_p1
# Intentionally replace result index 10
./evoflow run _pdata/tsp/sample.tsp config/ACS.cfg --tag sweep \
  --optimizer-name ACS_p1 --run-start 10 --replace-existing
```

Typical non-quiet output:

```text
problem: _pdata/tsp/sample.tsp
seed: 42
runs: 1
best_fitness: 12
solutions: 1
  [1] length:  12.000000   x: ...
```

The first-party `sample.tsp` is a 3–4–5 triangle. Every valid three-city tour has length 12, making it a useful installation smoke test without an external reference solution.

## 4. Paths

The executable resolves `_pdata/` and `config/` relative to the current working directory, not the executable's location. Run from the project root for relative paths, or provide absolute problem/configuration paths.

When calling family-specific templates directly, a problem path containing `/`, `\`, or `.` is treated as a path; otherwise a short name expands to `_pdata/<kind>/<name>.<ext>`. The generic `loadProblem` loader requires a real path so it can read `TYPE:` and dispatch.

## 5. Output channels

| Channel | Destination | Contents |
|---|---|---|
| Executable stdout / `-o` | Console or chosen file | Final best solution |
| Framework Logger | `_log/<name>/...` | Execution `.log`, errors `.err`, run results `.rslt`, generation history `.prcs` |

`-q` and `-o` affect only stdout. `-v` also echoes generation progress to the console. Framework logging is independent.

## 6. Problem families

File loading supports 13 exact `TYPE` values: ATSP, CFLP, CPMP, CVRP, FJSP, MDS, MIS, MKP, QAP, SR, STP, TSP, and WFS. TSP/ATSP share the TSP data family, giving 12 concrete file templates. See each `_pdata/<kind>/README.md` for its format.

The executable loads registered templates only; it does not dynamically load arbitrary external DLL/SO libraries. Use a C++ host or Python callable for continuous objectives and external benchmarks.

The program package contains 13 first-party examples. The 701 licensed external instances are distributed separately. Users obtain and convert TSPLIB, QAPLIB, and GK data themselves; see the [external-data tutorial](../../tutorial/external-data/README.md) (Chinese).
