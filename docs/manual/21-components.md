# 21 · Internal Components and Extensions

[English](21-components.md) | [简体中文](../手册/21-开发-内部组件.md) | [Manual index](README.md)

Optimizer extensions rely on self-registration and assembly-time compatibility assertions. This chapter shows how to add a learning strategy; topology, selection, and repair extensions follow the same pattern with their own base classes.

## 1. `Registry<Base>`

Each component base has a Meyers-singleton registry in `core/registry.h`. `EVOFLOW_REGISTER` uses C++17 inline variables to self-register by string tag at the end of a component header. Configuration/assembly creates by tag without knowing concrete classes.

```cpp
inline Registry<LearningStrategy>::Entry xxxEntry() {
    return {
        "MyStrategy",
        ModuleType::T_learnstrategy,
        ParameterTemplate{{
            {"alpha", ParamKind::Real, 0.0, 10.0, false, 1.0, 3.0},
        }},
        sizeof(MyStrategy),
        [](const double* p) -> LearningStrategy* {
            return p ? new MyStrategy(p[0]) : new MyStrategy();
        },
        [](AssertList& L, const double* p) {
            MyStrategy::preAssert(L, const_cast<double*>(p));
        },
        [](AssertList& L, const double* p) {
            MyStrategy::postAssert(L, const_cast<double*>(p));
        }
    };
}
EVOFLOW_REGISTER(lstrat_mystrategy, LearningStrategy, xxxEntry());
```

This sketch assumes `MyStrategy` has been implemented. Factory `const double*` may be null for defaults. Parameter fields are name, kind, lower/upper hard limits, permission to be empty, and recommended lower/upper values. Kinds include Real/Int/Enum/Bool. `createArray(tag,n,para)` supports batch construction where applicable.

Compile UTF-8 source with `/utf-8` or an appropriate BOM when comments contain non-ASCII text; misreading it as GBK can produce MSVC C2059 errors.

## 2. Assertions

At assembly, `AssertMatcher` accumulates `postAssert` provisions and checks `preAssert` requirements along the workflow:

```cpp
list.add(new Assert(ModuleType::T_individual, "velocity", 1, MatchType::equal));
list.add(new Assert(ModuleType::T_learntopology, "objects", 2, MatchType::notLessButNotice));
list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert));
```

The first line illustrates the assertion vocabulary; current feature provisioning is inferred through feature demands rather than legacy individual subclasses.

| MatchType | Result | Use |
|---|---|---|
| `notLess` | Too few: Error | Hard minimum |
| `equal` | Unequal: Error | Exact match |
| `notLessButNotice` | Too few: Error; excess: Info | Hard minimum with harmless extra provision |
| `anyButNotice` | Missing/inexact: Info | Executable but potentially different semantics/efficiency |
| `anyButWarning` | Missing/inexact: Warning | Buildable but execution/termination uncertain |
| `postAssert` | No precondition comparison | Declare a provision |

Notice means Info; do not use it for runtime reliability risks. Common contracts include inferred velocity/pbest/sigma/whalestate/fireworkstate/setvelocity, minimum topology `objects`, and strategy `constructive` for ordered/parallel construction. Keep vocabulary exact (`objects`, not `object`). See [validation details](11-optimizer-configuration.md#component-contracts).

## 3. Module categories

`core/module-type.h` includes `T_learnstrategy`, `T_learntopology`, `T_offspringgenerator`, `T_selector`, `T_evaluator`, `T_Repair`, `T_individual`, `T_bestarchive`, `T_subswarmmanager`, `T_subswarbuilder`, `T_subswarmtopology`, and other declared categories. Preserve exact API spellings.

## 4. Add a learning strategy

1. Create `src/optimizer/strategy/lstrategy-mystrategy.h` using UTF-8 and the surrounding source file conventions. Derive from `LearningStrategy` and implement lifecycle hooks: `preparation_s` per generation, `preparation_i` per individual, `nextDecision` for constructive generation or `getNewIndividual` for whole-solution generation. Declare pre/post assertions and registration.
2. Include it from `strategy/learning-strategies.h`. Topologies use `learning-topologies.h`; selectors also need the relevant include points in `optimizer-builder.h` and `assert-matcher.h`.
3. Optionally add a named fluent method to the appropriate setter in `config/config-setter.h`:

```cpp
void myStrategy(double alpha = 1.0) { set("MyStrategy", {alpha}); }
```

4. Choose the correct state lifetime:

| State | Implementation | Examples |
|---|---|---|
| Deterministic function of current population/fitness | Build a read-only per-individual map in `preparation_s`; look it up during generation | AffinityHypermutation, ExplosionSpark; constructive-safe |
| Per-individual state | Request a feature (role/kind/scope/init), cache its identity key, access `individual->feature<T>(key)` | Velocity, sigma, whale/firework state, set velocity |
| Across-generation personal best | `PbestFeature`, inherited at birth and updated after evaluation | PGBest topology |
| Stagnation age | `AgeFeature` get/reset/ageing, zeroed on initialization | Aging acceptance and scout activation |
| Parent identity | Singular `KinshipFeature`; birth sets `parent_id=parent.id` | ABC and elite local search |
| Existing operator reuse | Own an inner registry-created component, forward initialization/problem assignment, wrap behavior | TabuDecorator, immune Gaussian mutation |

Do not keep per-individual temporary values in one shared strategy scalar: ParallelConstruct calls preparation for all individuals before dimension generation, so the last preparation would overwrite everyone else's state. Use features. Private/Exclusive/Singular scopes and `shareFeature/feature_channels` control sharing; see [feature catalog](24-catalog.md#features).

5. Add a generated configuration and focused smoke test for behavior/convergence and assembly compatibility. Test output is English; build through `build.ps1`.
6. Use behavioral names for pluggable operators, for example `PGBest` and `GreyWolfEncircling`; whole-algorithm strategies may retain established names.

## 5. Selection has two layers

`EnvirSelect::update_subswarm(parent,offspring)` selects a replacement target: Index by position, Rank by merged ordering, Close by distance, WorstReplace by worst members, DistanceSelect by diversity, Kinship by parent identity.

`AcceptCriterion` decides whether replacement occurs: Unconditional, Better, Anneal, or Aging (greedy acceptance plus age maintenance). Scout activation is a separate stage in the current ABC recipe. Rank/WorstReplace/DistanceSelect implement global selection and use a placeholder unconditional criterion. Acceptance receives Terminator/BestArchive context where needed.

Add a criterion for a new acceptance rule; derive `EnvirSelect` for a new replacement structure.

## 6. Examples to inspect

- Stateful guide strategy: `lstrategy-woa.h` and the whale feature implementation.
- Read-only computed state: `lstrategy-immune.h` (`AffinityHypermutation`).
- Operator decorators: `lstrategy-tabu.h`, `lstrategy-immune.h`.
- Variable source-count topologies: `ltopology-clonal.h`, `ltopology-firework.h`.
- Selectors: `select-immune.h`, `select-firework.h`.
- Repair: `ModularRepair` in `solution-repairman.h`.
