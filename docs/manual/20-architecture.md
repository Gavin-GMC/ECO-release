# 20 · Development Architecture and Workflow

[English](20-architecture.md) | [简体中文](../手册/20-开发-架构与流程.md) | [Manual index](README.md)

This chapter explains Define → compile → Manager, the optimizer execution chain, and configuration assembly. For the steps to add an operator, see [internal components](21-components.md).

## 1. Four problem domains, two states

```text
Problem: descriptions                       ProblemHandle: runtime engines
  VariableDefine  ----compile()---->          VariableManager
  ObjectiveDefine ----compile()---->          ObjectiveManager
  ConstrainDefine ----compile()---->          ConstrainManager
  HeuristicDefine ----compile()---->          HeuristicManager
                                              + cross-domain orchestration
```

- `XXDefine` in `problem/define/*.h` stores a description populated by `add*`, without runtime state. `Problem` owns four Define objects and delegates to them.
- `XXManager` in `problem/manager/*.h` is the compiled engine with a fixed layout and domain operations.
- `Problem::compile()` compiles each domain and assembles a `ProblemHandle`.
- `problem/core/problem-handle.h` orchestrates domains: `solutionEvaluate` combines objectives, variable access, and constraint penalties; `getPrioriChoice` combines constraints and heuristics. Optimizers use the handle, not Define/Manager internals.

This separates user descriptions from execution, gives each domain one responsibility, and makes `compile()` the description-to-engine boundary. Constraint implementations live in `problem/constrain/`; heuristics in `problem/heuristic/`.

## 2. Execution units

```text
Optimizer -> Population -> SubpopulationManager -> Subpopulation(s)
                                                     |       |
                                                     |       +-> O_Workflow
                                                     +-> IndividualArray -> Individual
```

`Individual` owns a `Solution` and a feature bag. Components request velocity/pbest/sigma/etc. through `featureDemands()`; assembly attaches them. It no longer relies on individual subclasses. `IndividualArray` separates capacity from size.

`Subpopulation` is the basic generation/selection unit and owns a workflow. `SubpopulationManager` controls interactions, builders, and topology. Tags include `Single`, `NoInteraction`, `Immigrant`, and `RebuildTopology`. Since v1.4.8, `runEpoch` fixes the lifecycle and `interact()` is the extension hook; the framework handles two-level termination budgets.

`Population` is a thin top-level aggregate delegating to the manager and holding the global archive. `Optimizer` owns Population, Terminator, Logger, and a copied problem handle; `exe(seed)` and `exe(n, seed)` drive execution.

## 3. One generation

The workflow owns one ordered list of `O_Component` objects:

```text
RunGenerate: topology -> learning graph -> strategy per source -> repair
             Append offspring; do not clear the buffer.
RunGenerate: Optional additional stage; offspring accumulate.
RunEvaluate: Evaluate offspring, update archives, increment FES.
RunSelect:   Select the new parents, then clear offspring.
```

`orchestrate/optimize-component.h` defines `run(context)`. Wrappers in `component-wrappers.h` adapt generation, evaluation, and selection to this interface. `RunGenerate` uses topology, strategy, and repair; `RunEvaluate` wraps an evaluator; `RunSelect` wraps `EnvirSelect` and calls `offspring->clear()` at the end.

Offspring therefore accumulate across generation stages until selection merges and clears them. A stage boundary is needed when later generation depends on earlier evaluation/selection results. GA can use crossover `generation()` followed by `continueGraph()` and `generation(true)` for in-place mutation. Immune workflows use two generation/evaluation/selection groups, for clone selection and forced replacement. See `tutorial/2.subpopulation.cpp` and the immune configuration generators.

Individuals are conserved through selection by swapping rather than repeated allocation/deletion. The offspring buffer reuses allocated capacity when cleared.

## 4. Configuration assembly

```text
ConfigBuilder fluent calls
    -> FullConfig { workflows[], OptimizerConfig }
    -> registerWorkflow for each workflow
    -> OptimizerBuilder.buildOptimizer
       1. AssertMatcher compatibility checks
       2. Registry<Base>::create(tag, para) in configuration order
       3. O_Workflow -> Subpopulation -> Manager -> Population -> Optimizer
    -> setProblem and exe
```

`configure-list.h` defines ordered workflow components and optimizer settings: name, termination, interaction, archives, and subpopulations. Each operator is a `ComponentConfig{type, tag, para}`. Order defines execution order.

`AssertMatcher` accumulates `postAssert` provisions and checks `preAssert` requirements along the workflow. It rejects incompatibilities such as too few learning objects. Individual features are inferred from `featureDemands`; validation no longer takes an individual-type argument. See the [component contracts](11-optimizer-configuration.md#component-contracts).

Each component family has a `Registry<Base>`; `create(tag, para)` constructs by tag. A component implementation and registration do not require a new central factory case. See [registration](21-components.md). `config-codec.h` and `config-codec-json.h` serialize `FullConfig` to text and JSON.

## 5. End-to-end flow

```text
Problem --compile()--> ProblemHandle ----+
                                        +--> Optimizer.setProblem()
ConfigBuilder --buildOptimizer()---------+
                                             |
                                             v
Optimizer.exe: Population -> Subpopulation -> Workflow
  generate -> evaluate -> select, until a termination limit
  archive retains best solutions
                                             |
                                             v
Optimizer.getBest(best, size)
```

Continue with [internal components](21-components.md).
