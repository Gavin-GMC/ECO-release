# 27 · Source Layout

[English](27-source-layout.md) | [简体中文](../手册/27-开发-目录结构.md) | [Manual index](README.md)

Use this map with [architecture](20-architecture.md), [component catalog](24-catalog.md), [operator extensions](21-components.md), and [problem extensions](25-extending-problems.md). The common entry is `src/evoflow.h`, including the problem, optimizer builder, and fluent configuration interfaces.

## 1. Foundation under `src/`

| Directory | Main files | Responsibility |
|---|---|---|
| `basics/` | `evoflow-{basicfunc,constant,math,rand,sort,sys,time,eigen}.{h,cpp}` | Reproducible randomness, mathematics, sorting, system/time, constants, linear algebra |
| `calculator/` | `evoflow-calculator.h`, `evoflow-functor.hpp`, `func-calculator.cpp`, `tree-calculator.cpp` | Function/functor/formula evaluation |
| `exprTree/` | `ExprTree.{h,cpp}` | muParserX expression support |
| `solution/` | `solution.{h,cpp}`, `solution-decoder.{h,cpp}` | Decisions/fitness and multivariable decoding |
| `logger/` | `logger.hpp` | System/optimizer logging |
| `cp/` | `cp.h`, `solver.h` | Constraint-propagation entry points |
| `cp/compile/` | `parser.h`, `program.h` | Arithmetic/relation parsing, no mathematical functions |
| `cp/core/` | `interval.h`, `interval_set.h`, `config.h`, `scalar_traits.h`, `small_vector.h` | Interval primitives |
| `cp/domain/` | `variable_domain.h` | Variable domains |
| `cp/eval/` | `propagate.h`, `violation.h` | Propagation and violation evaluation |
| `distribution/` | `distribution-{model,factory,type}.h` | Gaussian/Cauchy/Uniform/Histogram sampling for EDA |
| `multiobjective/` | `mo-util.h` | Nondominated sorting, crowding, weights, neighborhoods, Tchebycheff; utilities shared by NSGA-II/MOEA/D, not registered components |

## 2. Problem module: `src/problem/`

| Directory | Files and responsibilities |
|---|---|
| `core/` | `problem.h` owns four Define objects; `problem-handle.h` owns Managers and orchestration |
| `define/` | `{variable,objective,constrain,heuristic}-define.h`, `calc-spec.h`: descriptions |
| `manager/` | `{variable,objective,constraint,heuristic}-manager.h`: compiled runtime engines |
| `variable/` | `variable.{h,cpp}`: domains |
| `objective/` | `objective.{h,cpp}`, `comparer.hpp`: priorities and Pareto dominance within priority levels |
| `constrain/` | `evoflow-domain.h`, `evoflow-cardinality.h`, `evoflow-accumulate.h`, `evoflow-graph-constrain.h`, `evoflow-constrain.h`, `discrete_view.h` |
| `heuristic/` | `evoflow-inspiration.h` |
| `template/` | `problem-template.h`, `pt-{tsp,qap,mkp,mds,mis,cvrp,cflp,cpmp,fjsp,wfs,stp,sr}.h`: twelve families covering thirteen exact file TYPEs |

Constraints are not registry-created: `ConstrainDefine::compile()` dispatches by `ConstrainKind`.

## 3. Optimizer module: `src/optimizer/`

| Directory | Main contents |
|---|---|
| `core/` | `registry.h`, `module-type.h`, `parameter-template.h`, `evoflow-assert.h` |
| `individual/` | Base `individual.h`, features including `feature.h`/`feature-setvelocity.h`, aggregate `individuals.h`; no individual-subclass registry |
| `solution-op/` | `solution-initializer.h`, `solution-repairman.h`; registered repairs, builder-dispatched initializers |
| `population/` | `individual-array.h`, `subpopulation.h`, `population.h` |
| `subpopulation-constructer/` | `subpopulation-constructer.h` |
| `subpopulation-topology/` | `subpopulation-topology.h`, `subpopulation-topology-basic.h`, `neighbor-set.h` |
| `subpopulation-manager/` | `subpopulation-manager.h`, `subpopulation-manager-basic.h` |
| `strategy/` | Base `learning-strategy.h`, aggregate `learning-strategies.h`, `lstrategy-*.h` |
| `topology/` | Base `learning-topology.h`, `learning-graph.h`, aggregate `learning-topologies.h`, `ltopology-*.h`; neighborhood topology supports MOEA/D |
| `generator/` | `generator.h` |
| `selector/` | `selector.h`, `select-basic.h`, `select-immune.h`, `select-firework.h`, `select-rank-crowding.h`, `select-scalar-replace.h` |
| `evaluator/` | `evaluator.h`, `evaluator-basic.h`, `evaluator-hash.h` |
| `terminator/` | `terminator.h`, a single termination class rather than a registered family |
| `archive/` | `best-archive.h` |
| `orchestrate/` | `optimize-component.h`, `optimize-workflow.h`, `component-wrappers.h`, `optimizer-builder.h`, `optimizer.h` |
| `config/` | `config-setter.h`, `configure-list.h`, `assert-matcher.h`, text/JSON codecs |

## 4. Where to add a component

All paths in this table are relative to `src/optimizer/` unless explicitly prefixed with `problem/`.

| Family / registry base | Base file | Implementation location | Include/assembly connection |
|---|---|---|---|
| `LearningStrategy` | `strategy/learning-strategy.h` | `strategy/lstrategy-*.h` | `learning-strategies.h` |
| `LearningTopology` | `topology/learning-topology.h` | `topology/ltopology-*.h` | `learning-topologies.h` |
| `OffspringGenerator` | `generator/generator.h` | Same file | Same file |
| `EnvirSelect` | `selector/selector.h` | `selector/select-*.h` | Builder and matcher includes |
| `Evaluator` | `evaluator/evaluator.h` | `evaluator/evaluator-*.h` | Relevant aggregate includes |
| `SolutionRepaireman` | `solution-op/solution-repairman.h` | Same file | Same file |
| Individual/features (not a registry) | `individual/individual.h` | `individual/` | `individuals.h` and feature-demand inference |
| `BestArchive` | `archive/best-archive.h` | Same file | Same file |
| `SubpopulationManager` | `subpopulation-manager/subpopulation-manager.h` | `subpopulation-manager-basic.h` | Same family |
| `SubpopulationConstructer` | `subpopulation-constructer/subpopulation-constructer.h` | Same file | Same file |
| `SubpopulationTopology` | `subpopulation-topology/subpopulation-topology.h` | `subpopulation-topology-basic.h` | Same family |
| Initializer (not registered) | `solution-op/solution-initializer.h` | Same file | `OptimizerBuilder::Ibuild` |
| Constraint (not registered) | `src/problem/constrain/*.h` | Same directory | `ConstrainDefine::compile`, switch by kind |
