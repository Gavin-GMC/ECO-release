# EvoFlow User Manual

[English](README.md) | [简体中文](../手册/README.md) | [Project README](../../README.md)

This English manual covers using and extending EvoFlow: problem modeling, optimizer configuration, execution, analysis, and component development. It covers documentation and source version **3.0.1.6**. Product, configuration, and result-file format versions are independent. Version 3.0.1.6 updates the documentation and source version metadata. Existing prebuilt executables, Python native modules, and download archives remain at 3.0.1.5; this update does not rebuild or republish them.

The chapters correspond to the Chinese manual under `docs/手册/`. API names, tags, and filenames retain their original spelling. Code fragments with placeholders are interface sketches; complete runnable examples live in `tutorial/` and `python/examples/`. Data and build guides linked outside this English manual remain in Chinese unless otherwise indicated.

## Choose a reading path

| Goal | Start here |
|---|---|
| Run a first optimization | [Overview](00-overview.md), then [CLI](13-command-line.md) or [GUI](14-gui.md) |
| Model a custom problem or external benchmark | [Problem modeling](10-problems.md), `tutorial/0.problem.cpp`, `tutorial/7.external-benchmark.cpp` |
| Define and compile a problem in Python | [Python modeling](10-problems.md#python-modeling), `python/examples/problem_model.py` |
| Configure/run/analyze from Python | [Python interface](15-python.md), then the Python examples |
| Configure or tune an optimizer | [Optimizer configuration](11-optimizer-configuration.md) and [algorithm recipes](11-optimizer-configuration.md#recipes) |
| Compare algorithms experimentally | [Result analysis](12-analysis.md), `tutorial/4.log-analysis.cpp` |
| Check packages or import external data | [CLI data checks](13-command-line.md#25-data), [Python diagnostics](15-python.md), [external-data tutorial](../../tutorial/external-data/README.md) (Chinese) |
| Add operators or extend the framework | [Architecture](20-architecture.md), [internal components](21-components.md) |

## Contents

### Overview

- [00 · Overview](00-overview.md): layers, workflows, execution chain, terminology, and versions.

### User guides

- [10 · Problem Modeling](10-problems.md): C++ variables, objectives, constraints, heuristics, handles, templates, external benchmarks, Python modeling and discovery.
- [11 · Optimizer Configuration](11-optimizer-configuration.md): fluent C++/Python configuration, validation, parameter schemas, operators, budgets, parallel execution, serialization, and algorithm recipes.
- [12 · Result Analysis](12-analysis.md): reductions, statistics, comparisons, quality metrics, TXT/CSV/LaTeX reports, and SVG/PNG charts.
- [13 · Command-Line Executable](13-command-line.md): builds, run/validate/report/data commands, paths, results, and supported file families.
- [14 · Graphical User Interface](14-gui.md): problem loading/generation, forms and block workflows, validation, execution, reporting, model settings, and troubleshooting.
- [15 · Python Interface](15-python.md): installation, ABI/platform support, diagnostics, five public interfaces, ownership, exceptions, examples, and builds.

### Developer references

- [20 · Architecture and Workflow](20-architecture.md): Define/Manager separation, execution units, stages, and assembly.
- [21 · Internal Components](21-components.md): registration, assertions, extension steps, feature state, and selection layers.
- [23 · Base-Class Reference](23-base-classes.md): fourteen core interfaces, lifecycle, ownership, and contracts.
- [24 · Component Catalog](24-catalog.md): topology/strategy tags and parameters, generators, selectors, evaluators, initialization, repair, archives, features, cooperation, and extension checklist.
- [25 · Extending Problem Modules](25-extending-problems.md): new constraints, templates, and evaluation backends.
- [26 · Expression Syntax](26-expressions.md): cp constraint expressions versus muParserX objective/heuristic formulas.
- [27 · Source Layout](27-source-layout.md): source directories and where each component family belongs.

## Runnable examples

Paths below are relative to the project root.

| File | Purpose |
|---|---|
| `tutorial/0.problem.cpp` | Define variables/objectives/constraints/heuristics, compile, evaluate, compare, propagate, and rank choices |
| `tutorial/1.optimizerconfig.cpp` | Configure and run a single-subpopulation optimizer |
| `tutorial/2.subpopulation.cpp` | Multistage crossover/mutation workflow and subpopulation access |
| `tutorial/3.population-manager.cpp` | Heterogeneous PSO/GA subpopulation cooperation |
| `tutorial/4.log-analysis.cpp` | Logging and ECAnalyzer report generation |
| `tutorial/5.template.cpp` | Template-based end-to-end optimization |
| `tutorial/6.elite-local-search.cpp` | Population/archive elite local search |
| `tutorial/7.external-benchmark.cpp` | External native benchmark integration |
| `tutorial/config-generate*.cpp` | Generate configurations for algorithm families |
| `python/examples/problem_model.py` | Python modeling and compiled evaluation |
| `python/examples/config_model.py` | Build, validate, and save a configuration |
| `python/examples/run_optimizer.py` | In-memory runs, progress, and continuation |
| `python/examples/template_catalog.py` | Registered types, instance discovery, and templates |
| `python/examples/file_workflow.py` | Run problem/configuration files |
| `python/examples/analyze_results.py` | Discover and analyze persisted experiment batches |
| `python/examples/external_benchmark.py` | External objective callables |

The CLI, GUI, and Python entry points share native behavior. Their presentation and supported controls can differ; each guide states its own restrictions rather than assuming all interfaces expose every native feature.
