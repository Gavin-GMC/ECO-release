# 23 · Component Base-Class Reference

[English](23-base-classes.md) | [简体中文](../手册/23-开发-组件基类参考.md) | [Manual index](README.md)

This reference describes responsibilities, lifecycle hooks, ownership, and contracts for implementers. See [registration/extensions](21-components.md), [execution architecture](20-architecture.md), and the [concrete catalog](24-catalog.md). `=0` denotes a pure virtual method; other virtual hooks have defaults. Signatures with ellipses describe hook families rather than compilable declarations.

## 1. LearningTopology

Build a `LearningGraph` connecting source individuals to learning solutions. Registered as `Registry<LearningTopology>`, category `T_learntopology`.

```cpp
virtual void ini() = 0;
virtual LearningGraph* getTopology(
    IndividualArray** subswarm, BestArchive** best_holder,
    const int swarm_number, IndividualArray* offspring,
    LearningGraph* last_graph) = 0;
```

`ini` resets each execution round. `subswarm[0]` is the current population; `best_holder[0]` is the global archive. `last_graph` refers to a previous generation stage within the current workflow invocation.

The caller manages the returned graph's lifetime; its source/destination pointers are non-owning and must remain valid during use. `postAssert` declares `objects`; zero means self-perturbation. A topology guaranteeing that every source comes from current offspring must explicitly provide `(T_learntopology,"offspringStart",1)` to support in-place generation. Matcher reads the current topology's own capability, not inherited historical capabilities.

`LearningGraph` is not registered. An edge holds an `Individual*` source and `end_point_size` `Solution*` destinations. Methods include `addStart`, `addEnd`, `getStartPoint`, `getEndPoint` (returns `Solution**`), `getSize`, and `getEndSize`. A null learning endpoint denotes pass-through copying without learning.

## 2. LearningStrategy

Generate offspring from learning objects, either as whole solutions or dimension-by-dimension. Registered under `T_learnstrategy`.

```cpp
virtual void ini(ProblemHandle*);
virtual void setProblem(ProblemHandle*);
virtual void preparation_s(IndividualArray& population, Terminator*);
virtual void preparation_i(Individual*, Solution** learn_obj, Individual* child);
virtual void preparation_d(int d, Individual*, Solution** learn_obj,
                           ProblemHandle*, Individual* child);
virtual double nextDecision(int d, Individual*, Solution** learn_obj,
                            ProblemHandle*, Individual* child);
virtual void getNewIndividual(Individual* child, Individual*,
                              Solution** learn_obj, ProblemHandle*);
```

Lifecycle: reset per round, prepare per population/generation, prepare per individual, prepare/choose per dimension, then update through `update_d`, `update_i`, and `update_s` hooks at their respective stages. Default whole-solution generation loops over dimensions. Default `nextDecision` throws `std::logic_error`; constructive strategies override it and declare `constructive=1`.

`preparation_s` reads terminator progress for scheduled coefficients. Per-individual random state reused across dimensions belongs in features, not a shared strategy scalar, because ParallelConstruct prepares multiple individuals before generating dimensions.

## 3. OffspringGenerator

Drive topology, strategy, and repair. Registered under `T_offspringgenerator`.

```cpp
virtual void generate(
    Terminator*, SolutionRepaireman*&, LearningTopology*&, LearningStrategy*&,
    IndividualArray* offspring, IndividualArray** parent, BestArchive** archive,
    int subswarm_number,
    std::queue<LearningStrategy*>& after_fes,
    std::queue<LearningStrategy*>& after_upd,
    LearningGraph*& last_graph) = 0;
```

Queues schedule strategy updates after evaluation and selection. The graph reference carries the previous stage's graph in and the current graph out. Generation appends offspring without clearing them. A null learning endpoint copies a parent and sets the existing API flag `has_evaluted=true`, so evaluation skips it.

Implementations: Generation (whole solution plus repair), GenerationNoCheck, OrderedConstruct, and ParallelConstruct (independent handles and dimension propagation). All share the in-place contract described in [configuration](11-optimizer-configuration.md).

## 4. EnvirSelect and AcceptCriterion

Selection combines where to replace and whether to accept. Registered selector category is `T_selector`.

```cpp
virtual void update_subswarm(IndividualArray& parent, IndividualArray& offspring,
                             Terminator* terminator, BestArchive* archive) = 0;
```

Replacement structures include Index, Rank, Close, WorstReplace, DistanceSelect, RankCrowding, ScalarReplace, and Kinship.

```cpp
virtual void accept(Individual& offspring, Individual& parent,
                    Terminator* terminator, BestArchive* archive) = 0;
virtual void update();
virtual void reset();
virtual std::vector<FeatureDemand> featureDemands();
```

`setFeatureKey(role,key)` supplies inferred identities. Acceptance rules are Unconditional, Better, Anneal, and Aging. Aging maintains stagnation age; current scout regeneration is a separate AgeActivation/Reinitialize stage, not embedded in acceptance. Global selectors such as Rank/WorstReplace/DistanceSelect do not use per-pair acceptance logic.

`RunSelect` clears offspring after `update_subswarm`; accumulated offspring therefore survive only until that selection stage.

## 5. Evaluator

Evaluate actual offspring fitness and update archives/FES. Registered under `T_evaluator`.

```cpp
virtual void setProblem(ProblemHandle*);
virtual void evaluate(Terminator*, BestArchive*, IndividualArray& offspring) = 0;
```

Per individual: skip already evaluated entries; stop if terminated; otherwise evaluate, mark `has_evaluted`, update the archive, and update termination accounting. Basic counts every real evaluation; Hash fills fitness from its duplicate cache without spending FES on hits.

## 6. Terminator

A concrete class, not a registry family. It tracks FES, stagnation/convergence, and elapsed seconds; any enabled limit can terminate execution.

```cpp
void update(bool best_update);
bool termination();
int getFESTimes();
void reset();
double getProgress();
```

`update` increments FES, resets stagnation on improvement or increments it otherwise, and updates elapsed time. `getProgress` returns the maximum enabled state/condition ratio. `merge` accumulates local progress; `limited` restricts local resources. `EVOFLOW_MAX` disables a condition. Construction initializes states to zero even before reset.

## 7. BestArchive

Keep the best or relevant solution set. Registered under `T_bestarchive`.

```cpp
virtual Solution* getBest();
virtual Solution* getElite() = 0;
virtual bool updateBest(const Solution&) = 0;
virtual int getBestSize();
virtual void clear();
virtual void setProblem(ProblemHandle*) = 0;
```

Basic keeps one best, Multiobject a Pareto set, Multimodal multiple distinct decision-space optima. Historic is not registered. Comparisons use the problem's direction-aware comparer. An archive may be empty before generation: check `getBestSize()`/null before dereferencing.

## 8. SolutionInitializer

Generate initial candidates. Selected by `workflow.initializer(tag)` but dispatched by `OptimizerBuilder::Ibuild`, not a registry.

```cpp
virtual void ini_solution(double* result, int size) = 0;
void ini_solution(Solution&);
virtual void setProblem(ProblemHandle*);
```

The Solution overload fills and optionally evaluates; otherwise an empty fitness marks it unevaluated. Implementations: Random, Greedy, R_G, Distribution, No. A new initializer requires updating `Ibuild`.

## 9. SolutionRepaireman

The API retains this spelling. Registered category `T_Repair` repairs one invalid dimension:

```cpp
virtual double repair(const Solution&, int dim, ProblemHandle*) = 0;
```

Boundary clamps, Modular wraps, Random samples, Greedy uses heuristic priority, and No preserves the value. Generators invoke repair when `constrainCheck(dim,value)` fails.

## 10. Individual and features

There is one base `Individual`, with a Solution, evaluation state, and feature bag. The old Particle/SetParticle/etc. subclasses and `Registry<Individual>` are no longer the extension model.

Common operations include solution/objective sizes, `setProblem`, `ini`, copy/shallow-copy/clear/swap, indexed decision access, and direction-aware comparison. Components declare role/kind/scope/init through `featureDemands`; assembly attaches features and supplies identity keys for `individual->feature<T>(key)`.

Feature kinds are vector, scalar, solution, pbest, setvelocity, age, and kinship. At birth, features with `inheritAtBirth=true` use `birthFrom(parent)`. Default behavior copies memory; pbest/setvelocity carry history, while kinship derives `parent_id=parent.id`. See the [feature table](24-catalog.md#features).

## 11. Subpopulation

The basic generation/selection unit owns `O_Workflow`, parent/offspring arrays, and a local archive. `setProblem`, `ini`, `run`, `getBest`, and `getSize` manage setup and execution. `run()` executes workflow stages each generation.

## 12. SubpopulationManager

Define group interaction and drive the constructer/topology. Registered category `T_subswarmmanager`.

```cpp
void ini();      // Fixed framework lifecycle
void runEpoch(); // Interaction, topology maintenance, quota/run/accounting
protected:
virtual void interact() {}
virtual void iniHook() {}
```

`globalOptimumCollection(BestArchive*)` and `setProblem(ProblemHandle*)` remain virtual. Single handles one group; NoInteraction evolves groups independently and gathers global results; Immigrant sends best members to neighbors; RebuildTopology invokes regrouping through the constructer.

Local terminators hold per-epoch budgets; the global terminator holds the total. Each epoch assigns `min(local budget, global remainder)` through `limited`, then immediately `merge`s consumed resources. This preserves both global limits and interaction cadence. New managers implement `interact()` rather than resetting/merging terminators themselves. Initialization is accounted for as a one-time cost under remaining global resources.

## 13. SubpopulationConstructer

Partition or repartition individuals. Registered category `T_subswarbuilder`; preserve the API's spelling.

```cpp
virtual void build(Subpopulation** subpopulations, int& swarm_number) = 0;
virtual void ini(Subpopulation** subpopulations, int& swarm_number);
```

Fixed preserves partitions; Distance and Fitness regroup by their respective criteria. `build` swaps individuals into target groups in place, conserving individuals. Swap-order concerns are tracked in the internal roadmap.

## 14. SubpopulationTopology

Store one `NeighborSet` per subpopulation and define group-level neighbors for migration/cooperation, distinct from individual-level LearningTopology. Registered category `T_subswarmtopology`.

```cpp
virtual void build(Subpopulation** subpopulations, int& swarm_number) = 0;
virtual void ini(Subpopulation** subpopulations, int& swarm_number) = 0;
```

`ini` allocates sizes and builds; `build` refreshes existing neighbor sets. Connected, Star, Toroidal, Preswarm, and NoConnect are provided. The manager calls `build` each epoch, so random/dynamic topologies resample there while static ones reproduce the same neighbors. No separate dynamic flag or obsolete `update()` hook is needed.

## Execution map

```text
Workflow:
  topology -> graph -> generator -> strategy hooks + repair
                    -> evaluator -> archive + terminator
                    -> selector + acceptance -> clear offspring
Outer execution:
  Optimizer -> Population -> manager.runEpoch
    interact -> topology maintenance -> each group [quota -> run -> accounting]
    -> global optimum collection
```
