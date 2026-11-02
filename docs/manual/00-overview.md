# 00 · Overview

[English](00-overview.md) | [简体中文](../手册/00-总览.md) | [Manual index](README.md)

EvoFlow is a C++17 evolutionary computation framework for combinatorial and continuous optimization. It separates problem modeling from optimizer assembly. A self-registering component catalog lets you combine PSO, ACO, GA, DE, EDA, ES, GWO, WOA, immune, fireworks, and other operators into configurable optimization workflows.

The documentation and source version covered by the project README is **3.0.1.6**. Version 3.0.1.6 updates the documentation and source version metadata. Existing prebuilt executables, Python native modules, and download archives remain at 3.0.1.5; this update does not rebuild or republish them. Windows x64 is the fully validated release platform. Linux core/CLI source builds have been validated on Ubuntu 24.04.4 WSL2 x86_64, GCC 13.3, CMake 3.28.3, and Ninja 1.11.1; no prebuilt Linux release is provided. The GUI and model integration currently require Windows. macOS is unvalidated. Prebuilt Windows Python modules support 64-bit CPython 3.8–3.12.

The source/program package contains 13 first-party minimal examples. A separate full-data overlay supplies 701 larger instances with documented provenance and redistribution status. TSPLIB, QAPLIB, and GK originals are not redistributed; see the [external-data tutorial](../../tutorial/external-data/README.md) (Chinese). `evoflow data [path]` checks structure and sizes; `python -m evoflow --check-data [path]` additionally verifies SHA-256 hashes.

The common include is `#include "evoflow.h"`, which brings together the problem and optimizer-builder interfaces. Some dependencies require compiled sources; use the supplied build projects for complete applications.

## 1. Three layers

```text
Optimizer layer
  ConfigBuilder -> FullConfig -> validation -> OptimizerBuilder -> Optimizer
  Optimizer -> Population -> Subpopulation -> O_Workflow
    -> O_Component [generate -> evaluate -> select] -> IndividualArray -> Individual
  Registered topologies, strategies, generators, selectors, evaluators, terminators, archives
          |
          | Public ProblemHandle operations: evaluate, greedy, feasible, ...
          v
Problem layer
  Problem [four Define objects] --compile()--> ProblemHandle [four Managers]
  Variable / Objective / Constrain / Heuristic; 12 file-template families
          |
          v
Foundation
  basics; calculator + exprTree; solution; logger; cp; distribution
```

### 1.1 Foundation

| Component | Responsibility |
|---|---|
| `basics/` | Reproducible shared RNG facilities, mathematics, sorting, system/time utilities, constants |
| `calculator/` + `exprTree/` | Formula evaluation through muParserX for objectives/heuristics |
| `solution/` | `Solution` with separate `result` and `fitness` vectors; `SolutionDecoder` for multivariable layouts |
| `logger/` | System and per-optimizer logging; see [result analysis](12-analysis.md) |
| `cp/` | Interval constraint propagation, separate from muParserX, for expression-constraint domain reduction |
| `distribution/` | Gaussian/Cauchy/Uniform/Histogram sampling models for EDA |

### 1.2 Problem layer

`Problem` owns four Define objects and delegates `add*` calls to them. `compile()` produces a `ProblemHandle` containing the four compiled Managers and cross-domain orchestration. Optimizers interact only through the handle's public interface.

The domains are Variable, Objective, Constrain, and Heuristic. Each has a `*-define.h` description and a `*-manager.h` runtime engine. Constraints include domain, cardinality, accumulation, graph, and cp-expression families.

`problem-template.h` and `pt-*.h` construct TSP, QAP, MKP, MDS, MIS, CVRP, CFLP, CPMP, FJSP, WFS, STP, and SR problems. Model continuous problems and external benchmarks through generic objective functors/callables.

### 1.3 Optimizer layer

`Registry<Base>` and `EVOFLOW_REGISTER` register components by string tag. Execution units are `Individual` (with inferred features), `IndividualArray`, `Subpopulation`, `Population`, and `Optimizer`.

| Family | Base class | Role |
|---|---|---|
| Learning topology | `LearningTopology` | Decide who learns from whom; produce a `LearningGraph` |
| Learning strategy | `LearningStrategy` | Decide how to learn or generate offspring |
| Offspring generator | `OffspringGenerator` | Apply topology/strategy in one of four generation frameworks |
| Environmental selection | `EnvirSelect` | Choose a replacement target and decide whether to replace it |
| Evaluation / termination / archive | `Evaluator` / `Terminator` / `BestArchive` | Fitness and FES accounting / stopping conditions / best solutions |
| Solution operations | `SolutionInitializer` / `SolutionRepaireman` | Initialization / boundary repair |

`O_Component` is a pipeline operation. `O_Workflow` runs its ordered component list every generation. `OptimizerBuilder` assembles configurations. The fluent `ConfigBuilder` creates a `FullConfig`; `AssertMatcher` validates compatibility. Text/JSON codecs serialize configurations.

## 2. Three ways to use the platform

### 2.1 Define a problem

```cpp
EvoFlow::Problem p("my_problem");
p.addVariable("x", -5, 5, 0.001, 5);     // Name, bounds, precision, dimensions
p.addObjective("f", 1, true, "x", f_ptr); // Name, priority, minimize, inputs, function
p.addConstrainRange("x", -3, 3, 1, "");
EvoFlow::ProblemHandle* h = p.compile();
h->solutionEvaluate(sol);
```

This is an API sketch: supply the objective and solution objects in your application. Templates offer ready-made models. See `tutorial/0.problem.cpp` and [problem modeling](10-problems.md).

### 2.2 Configure and run an optimizer

```cpp
EvoFlow::ConfigBuilder cb;
auto wf = cb.workflow("wf");
wf.initializer("Random");
wf.topology.pgBest();
wf.repair.boundary();
wf.strategy.velocityDriven(2, 2.0, 0.9, 0.4);
wf.generator.generation();
wf.evaluator.basic();
wf.selector.index(false);
cb.name("run").maxFES(40000).manager("Single").gArchive("Basic");
cb.subpopulation("1").size(20).workflow("wf").maxFES(40000).archive("Basic");

EvoFlow::Optimizer* opt = cb.buildOptimizer();
opt->setProblem(&p);
opt->exe(30);
Solution* best = nullptr;
int n = 0;
opt->getBest(best, n);
```

See `tutorial/1.optimizerconfig.cpp` for the complete example and [optimizer configuration](11-optimizer-configuration.md) for overloads and execution options.

### 2.3 Use Python

`python/evoflow` shares the C++ problem, configuration, execution, and analysis implementation, with type hints, ABI diagnostics, and examples. Windows uses a separate tagged `.pyd` per CPython minor version. Linux CPython 3.12.3 source builds and portable tests have been validated, but no prebuilt Linux module is provided. Python builds are independent of CMake. See the [Python guide](15-python.md).

## 3. Execution within a generation

```text
Optimizer.exe(n)
  Each independent run: iterate Population
    Each generation: run Subpopulation.O_Workflow in configuration order
      RunGenerate: topology -> per-individual strategy -> offspring -> repair
      RunEvaluate: evaluate offspring; update archive and terminator (FES++)
      RunSelect: update parents from offspring; clear offspring
    Stop when an evaluation, convergence, or time limit is reached
```

A multistage workflow repeats generation/evaluation/selection as required, for example crossover plus mutation or two immune-selection stages.

## 4. Terminology

| Term | Meaning |
|---|---|
| Define / Manager | Problem-domain description / compiled runtime engine |
| `ProblemHandle` | Compiled problem and optimizer-facing interface |
| Registry / tag | Self-registration table / string component name used in configurations |
| `LearningGraph` | Each edge connects one starting individual to learning objects; an empty destination set means no learning |
| `O_Component` / `O_Workflow` | Pipeline operation / ordered orchestration |
| Segment | A generation/evaluation/selection stage; boundaries depend on whether later generation needs earlier evaluation results |
| `AssertMatcher` | Accumulate `postAssert` provisions and check each component's `preAssert` requirements |
| FES | Fitness evaluations, the main computation budget |
| Inferred features | `featureDemands()` requests velocity/pbest/sigma/etc.; assembly attaches them to each individual instead of selecting legacy individual subclasses |

## 5. Naming and versions

Product, configuration, and persisted-file versions are independent. Query the actual product through `evoflow --version` or `python -m evoflow --version`; do not rewrite configuration `v3.1/v3.2` because the product changes.

Pluggable operators use behavioral names: PSO topology is `PGBest`; GWO strategy is `GreyWolfEncircling`. Whole-algorithm strategies may keep standard names such as `AntSystem`.

Sources use UTF-8 (`cl /utf-8`), and test/example output is English to avoid console encoding problems.
