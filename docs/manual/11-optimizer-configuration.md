# 11 · Optimizer Configuration

[English](11-optimizer-configuration.md) | [简体中文](../手册/11-使用-优化器配置.md) | [Manual index](README.md)

Assemble optimizers with C++ `ConfigBuilder` or Python `evoflow.Config`. Changing an algorithm changes configuration entries rather than the application's structure. `evoflow.h` includes the fluent builder. Examples: `tutorial/1.optimizerconfig.cpp`, `python/examples/config_model.py`, `python/examples/run_optimizer.py`, and `tutorial/config-generate*.cpp`.

## Python configuration

`Config` owns a native `FullConfig`, sharing the native codecs, catalog, and validation rather than maintaining a separate dictionary implementation.

```python
from evoflow import Config

config = Config()
config.name = "python_pso"
config.max_fes = 100_000
workflow = config.add_workflow("wf")
workflow.set_initializer("Random")
workflow.add_topology("PGBest")
workflow.add_repair("Random")
workflow.add_strategy("VelocityDriven", [2, 2, 0.9, 0.4, 0])
workflow.add_generator("OrderedConstruct", [0])
workflow.add_evaluator("Basic")
workflow.add_selector("Index", [0])
subpopulation = config.add_subpopulation("1", size=100, workflow="wf")
subpopulation.max_fes = 100_000
report = config.validate(deep=True)
if report.ok:
    config.save("python-pso.cfg")
```

- `load/from_text` detect v3.1/v3.2; `to_text/save` retain the source format unless explicitly passed `"v3.1"`/`"v3.2"`. New configurations default to v3.1.
- Parameter `None` maps to `EMPTYVALUE`, encoded as text `nan` or JSON `null`. For termination settings it means unlimited.
- Validation reports highest `status` (`error/warning/info/pass`); individual issues use the first three. `ok` is false only for Error/loading failure. Deep validation constructs and destroys an optimizer without executing it.
- `component_categories/list_components/resolve_parameter_schema` expose 12 categories, five initializers, and progressive schemas. `component_requirements/component_provisions` query parameter-dependent contracts.
- `Workflow.components` is a read-only snapshot. Use workflow methods to add/replace/move/remove entries. Removing/clearing workflows or subpopulations invalidates previously obtained handles of that kind with `RuntimeError`; appending alone does not.
- `console_echo` is runtime-only and is not serialized. Configurations can be passed directly to Python `Optimizer`.

## Python execution

`Optimizer` makes deep native snapshots of `Problem`/`CompiledProblem` and `Config`; later changes/destruction of source objects do not affect it.

```python
from evoflow import Optimizer, Problem, VariableType

problem = Problem("sphere")
problem.add_variable("x", -5, 5, 0.001, 5, type=VariableType.CONTINUOUS)
problem.add_objective("f", "x", lambda x: sum(v * v for v in x))
optimizer = Optimizer(problem, config, optimizer_name="PSO_sweep_01", max_fes=20_000)
result = optimizer.run(seed=42, runs=3,
                       progress=lambda completed, total: print(completed, total))
print(result.tag, result.best_fitness)
for round_result in result.rounds:
    print(round_result.run, round_result.result_index, round_result.best_fitness)
```

Repeated `run()` calls reinitialize the population. Default disk numbering continues after the largest occupied index for optimizer/problem/tag, including across instances and processes. `round_result.run` is one-based within the call; `result_index` is the zero-based filename suffix; `result.run_start` is the actual first index.

Explicit `run_start=N` rejects collisions before execution unless paired with `replace_existing=True`. Transactional replacement restores old `.rslt/.prcs` on failure/cancellation; replacement cannot omit the start index.

`runs=N` seeds once and continues one random stream for N rounds, not `seed+i`. Independent `run()` calls with the same seed on the same optimizer are reproducible. Constructor budget overrides `max_fes/max_convergence/max_time=None` preserve configured values; supplied nonnegative integers override global and all subpopulation budgets, matching the CLI. `verbose=True` enables runtime history and console echo.

Progress callbacks run after complete rounds, not per FES. `cancelled()` is polled at native termination checkpoints and latches true; normal cancellation returns partial results with `cancelled=True, ok=False`. Assembly, execution, and callback failures raise exceptions; observer-boundary exceptions are restored as `RuntimeError` after safe native return rather than discarded.

Do not concurrently run or reenter the same optimizer from its callbacks. Separate instances may be used independently; Python objectives in parallel builds must still be thread-safe.

`run(problem, config, ...)` constructs a temporary optimizer. File-based execution shares the App/CLI path:

```python
from evoflow import run_files
result = run_files("_pdata/tsp/sample.tsp", "config/ACS.cfg", seed=42)
```

File execution failures raise `RuntimeError`; cancellation returns partial results. Successful uncancelled single rounds can continue:

```python
first = optimizer.run(seed=42)
more = optimizer.continue_run(max_fes=5_000)
```

Continuation requires at least one positive additional limit and can be repeated. It is unavailable after multi-round execution, failure, or cancellation. It wraps native `continue_exe`, not pause/checkpoint/cross-process recovery, and offers no progress/cancellation/new-log hooks because native continuation has none.

## 1. C++ configuration skeleton

```cpp
#include "evoflow.h"
using namespace EvoFlow;
ConfigBuilder cb;
auto wf = cb.workflow("wf");
wf.initializer("Random");
wf.topology.pgBest();
wf.repair.boundary();
wf.strategy.velocityDriven(2, 2.0, 0.9, 0.0001);
wf.generator.generation();
wf.evaluator.basic();
wf.selector.index(false);
cb.name("run").maxFES(40000).manager("Single").gArchive("Basic");
cb.logging(false);
cb.consoleEcho(true);
cb.subpopulation("1").size(20).workflow("wf").maxFES(40000).archive("Basic");
Optimizer* opt = cb.buildOptimizer();
opt->setProblem(&problem); // Supply your Problem or ProblemHandle.
opt->exe(42);             // One run, seed 42; multiple runs: exe(30, 42).
Solution* best = nullptr;
int n = 0;
opt->getBest(best, n);
delete opt;
```

`logging` controls result/history/detailed-history/population snapshots; its fourth swarm argument was added in v2.2.2.3. `consoleEcho` is runtime-only, equivalent to CLI `-v`. Individual types are inferred through feature demands, not specified in configurations.

### Multistage workflows

Generation stages can accumulate offspring before evaluation/selection, or repeat evaluation/selection groups. GA commonly chains crossover and in-place mutation:

```cpp
wf.topology.tournament(2,1);
wf.strategy.sbxCrossover(20,1);
wf.generator.generation();
wf.topology.continueGraph();
wf.strategy.pmMutation(20,0.01);
wf.generator.generation(true);
wf.evaluator.basic();
wf.selector.rank();
```

`inplace=true` modifies existing offspring without creating new ones. It requires unevaluated offspring not yet merged by selection, and a topology whose sources are those offspring. Built-in valid topologies are `Continue` and `ContinueRankExtremesTriRandom`. Parent-based topologies, `Inherit`, `NoChange`, and in-place generators after evaluation/selection are statically rejected.

Continuation states are distinct: Continue variants read offspring; `Inherit` reads offspring and the last graph; `NoChange` copies only the last graph. Selection clears offspring, invalidating the former paths, while NoChange may reuse the last graph still alive within the current workflow invocation. Evaluation marks offspring evaluated without deleting them, preventing later in-place mutation. The last graph is released at the end of `workflow.run`.

Contracts use the current topology/strategy/repair at the generator position. A new strategy replaces the old one and cannot inherit its `constructive` capability. For `AdaptiveOperatorSelection`, every operator in the possible pool must support dimension-wise construction before Ordered/ParallelConstruct is allowed. A mixed pool containing `Gauss` and whole-solution `Overturn` is rejected for constructive generation.

## 2. Assembly validation

`buildOptimizer()`/`buildSubpopulation()` validate before construction. Errors throw; warnings go to `sys_logger.warning`; informational issues appear in explicit structured validation only.

| Order | Validator | Checks | Disable macro |
|---|---|---|---|
| 1 | `AssertMatcher` | Tags, component requirements/provisions, ordering | `EVOFLOW_DISABLE_ASSERT_VALIDATION` |
| 2 | `ParamMatcher` | Parameter arity, emptiness, range, integrality | `EVOFLOW_DISABLE_PARAM_VALIDATION` |

Tag recognition comes first to avoid misleading parameter errors. Macros are documented in [optimizer-builder.h](../../src/optimizer/orchestrate/optimizer-builder.h); normally leave validation enabled.

### 2.1 Parameter rules

An entirely empty parameter array means all internal defaults; factories receive `nullptr`. Once nonempty, the array must be complete because factories read positional entries. Use `EMPTYVALUE` for permitted omissions instead of shortening the array. Empty entries require `allow_empty`. Nonempty values must satisfy hard bounds, and Int/Enum values must be integral.

These rules prevent silent memory errors such as selecting annealing with `Index` but omitting its temperature/decay, or leaving a `TabuDecorator` inner operator incomplete.

### 2.2 Machine-readable schemas

Registry `ParameterTemplate` is authoritative for validation and automated configuration. Fields are name, kind, hard bounds, `allow_empty`, and recommended sampling bounds. Some schemas disclose later levels according to earlier values:

| Component | First level | Next level |
|---|---|---|
| `Index` / `Close` / `Kinship` | `accept_type` | Selected criterion parameters; annealing exposes `T0, decay` |
| `TabuDecorator` | `inner, tenure` | Complete inner-operator schema, including its further levels |
| `AdaptiveOperatorSelection` | `rho, segment_len, op_count` | `op_count` operator enums |

Query the registry for the correct base family, for example `Registry<EnvirSelect>::instance().params("Index")`. `hasNext()` and `nextLevel(para,count)` walk a progressive schema. `nextLevel` takes the **current level's slice**, not the full outer array:

```cpp
std::vector<double> para;
ParameterTemplate cur = *reg.params(tag);
size_t base = 0;
while (true) {
    for (const ParamInfo& pi : cur.params) para.push_back(sample(pi));
    if (!cur.hasNext()) break;
    ParameterTemplate next = cur.nextLevel(para.data() + base, para.size() - base);
    if (next.params.empty() && !next.hasNext()) break;
    base += cur.params.size();
    cur = next;
}
```

`sample` is your schema-aware sampler. Validate the result with `ParamMatcher::validate`. Each template declares only its own parameters; nested component parameters are exposed through `next`, not flattened into unrelated templates.

<a id="component-contracts"></a>
### 2.3 Component contracts

| Severity | Meaning | Effect |
|---|---|---|
| Error | Statically unbuildable, invalid execution chain, or illegal parameters | Blocks assembly/run |
| Warning | Buildable, but normal execution/termination cannot be guaranteed | Allowed; review before running |
| Info | Executable, but semantics/results/efficiency may differ from intent or be redundant | Allowed; review intent |
| Pass | No static issue found | Aggregate only, not a proof for every runtime input |

`notLessButNotice` reports Error for insufficient provision and Info for excess. `anyButNotice` reports Info for missing/inexact provision. Use `anyButWarning` for execution/termination uncertainty. `notLess` and `equal` are hard requirements.

Feature demands infer individual state automatically. Learning-object counts remain hard safety contracts: VelocityDriven/SetVelocityDriven need at least their configured `object_number`; fewer can cause array bounds errors, while extra objects are ignored with Info.

WWO has additional checks. `WaveRefractionActivation + WaveRefraction` activates/resets height; without an actually used propagation stage that maintains/decreases height, all particles may pass through with no FES progress, producing Warning `wave_refraction_without_height_progress`. Refraction assumes unconditional same-index replacement; another selector (for example Rank) produces Info `wave_refraction_selector_semantics`.

Thus `WaveRefractionActivation → Greedy → WaveRefraction(16) → Generation(0) → Basic → Rank` aggregates to Warning. Generation/Greedy are not independently invalid just because of this combination. A normal WWO loop includes actual WavePropagation generation and `Index(0)` after refraction.

| Strategy | Inferred features | Topology requirement |
|---|---|---|
| VelocityDriven | velocity; PGBest additionally requests pbest | At least `object_number`, normally 2 |
| SetVelocityDriven | setvelocity; pbest when required by topology | At least `object_number`, normally 2 |
| GaussianSelfAdapt | sigma | Isolate, 0 objects |
| GreyWolfEncircling | None | TopRanked with at least 3 |
| WhaleForaging | whalestate | LeaderAndRandom, 2 |
| GaussianSpark | fireworkstate | RandomSelect sources |
| Crossover/mutation/EDA | Usually none | Strategy-specific |

## 3. Operator directory

Use named methods in `config-setter.h` for exact parameter signatures and [component catalog](24-catalog.md) for details. Continuous means real domains; permutations/sequences are discrete orderings; binary means 0/1.

### 3.1 Topologies: `wf.topology.*`

| Method | Behavior | Objects |
|---|---|---|
| `pgBest()` | Personal and global best | 2 |
| `randomLearning(n)` | n random learning objects per individual | n |
| `isolate()` | Self-perturbation without learning | 0 |
| `elite(e,r)` | Repeat each of e archive elites r times | 1 |
| `competition()` | Competitive pairing | Strategy-dependent |
| `levelBasedLearning(l)` | Level-based learning | Strategy-dependent |
| `stochasticDominantLearning()` | Stochastic dominant learning | Strategy-dependent |
| `continueGraph()` | Continue previous offspring | Previous state |
| `inherit()` / `noChange()` | Reuse offspring/last graph as described above | Previous state |
| `roulette(n)` / `tournament(s,n)` / `uniform(n)` | Roulette/tournament/uniform object selection | n |
| `topRanked(k)` | Current top k | k |
| `leaderAndRandom()` | Global best plus random member | 2 |
| `clonalExpansion(beta)` | More clones for better antibodies | 0 |
| `randomSelect(d)` | d random sources | 0 |
| `fireworkExplosion(M,a,b)` | Fitness-dependent spark counts | 0 |
| `topIndividual(e,r)` | Current top-e sources, r repeats, random companion | 2 |
| `bestAndWorst()` | Current best and worst | 2 |
| `repeatWithBest(k)` | k repeats per member guided by current best, skipping the best itself | 1 |

### 3.2 Strategies: `wf.strategy.*`

| Method | Algorithm / parameters |
|---|---|
| `velocityDriven(obj,c,w_ini,w_final)` / `setVelocityDriven(obj,c,w_ini,...,w_final)` | PSO/SetPSO; object count, acceleration, initial/final inertia |
| `binaryVelocityDriven(c1,c2,w_ini,w_final)` / `stickyBinary(c,rp,rg,ustkS)` | BPSO / sticky BPSO; only the former uses the linear inertia schedule |
| `distributionEstimation(model,good_n,good_rate)` | EDA; model 1 Gaussian, 2 Cauchy, 3 Uniform, 4 Histogram |
| `gaussianOneFifth(sigma,adapt)` / `gaussianSelfAdapt()` / `cmaes(sigma)` | One-fifth ES, self-adaptive ES, CMA-ES |
| `antSystem(...)` / `maxMinAntSystem(alpha,beta,rho,p_best,ug,ul)` / `antColonySystem(...)` | AS/MMAS/ACS |
| `greyWolfEncircling(a_max,a_min)` | GWO coefficient bounds, defaults 2/0 |
| `whaleForaging(a_max,a_min,b)` | WOA bounds and spiral constant |
| `affinityHypermutation(sigma,base_rate,rho)` / `randomGeneration()` | Immune hypermutation / resampling |
| `explosionSpark(A,dim_rate)` / `gaussianSpark(dim_rate)` | FWA amplitude and dimension probability |
| `tabuDecorator(inner,tenure,...)` / `tabuBit(tenure)` | Tabu neighborhood wrapper |

PSO `w_final` is the actual inertia at the end of the current subpopulation budget segment: `w = w_ini + (w_final-w_ini)*progress`, with combined terminator progress clamped to `[0,1]`. A new budget segment restarts at `w_ini`; a larger final value produces increasing inertia. Empty final values retain each strategy's random-inertia mode.

Empty-decision handling needs no extra configuration. AS/ACS/MMAS preserve empty dimensions when path predecessors are empty; SetPSO ignores sequence edges with empty endpoints and skips position crisping for empty positions; EDA ignores empty samples per dimension. CMA-ES updates only from complete solutions and freezes updates if too few complete parents exist. Competition/LeaderRandomCenter ignore empty members per dimension; GOA skips empty neighbors. VelocityDriven/RSA/Aquila recover from valid guides, fall back when auxiliary guides are empty, and preserve emptiness if no valid source exists. All follow finite-worst-fitness semantics without exceptions/warnings/forced termination.

Crossover methods include `pointCrossover`, `uniformCrossover`, `sbxCrossover`, `differenceCrossover`, `partialMappedCrossover`, `cycleCrossover`, `orderCrossover`, `subtourExchangeCrossover`, and `positionBasedCrossover`; GA coupling defaults true.

Mutations include `bitMutation`, `bitFlipMutation`, `pmMutation`, `gaussMutation`, `overturn`, `exchange`, `insert`, `reorder`, `segmentRelocate`, `cyclicExchange`, `blockSwap`, `multiPointSameValue`, `setExchange`, `classRelocate`, and `classMerge`.

Mutation rates differ by family: bit/bitFlip/PM/Gauss select each dimension independently. Overturn/exchange/insert/reorder/segmentRelocate/cyclicExchange/blockSwap/classMerge use one probability for applying the whole operation. `multiPointSameValue(point_rate)` selects dimensions; `setExchange/classRelocate(subset_rate)` select class members, with empty rate meaning a fresh random probability. GA/DE are component compositions, not standalone strategy classes.

### 3.3 Generators

| Method | Behavior |
|---|---|
| `generation(inplace=false)` | Whole-solution generation with constraint repair |
| `generationNoCheck(inplace)` | Whole-solution generation without repair |
| `orderedConstruct(inplace)` | Sequential dimension construction; requires constructive strategy |
| `parallelConstruct(inplace)` | Dimension-synchronous construction with independent handles and propagation per individual |

All four in-place modes obey the same offspring-source contract and must precede evaluation/selection.

### 3.4 Selectors

| Method | Behavior |
|---|---|
| `index(false)` / `index(true)` | Unconditional same-slot replacement / greedy improvement |
| `indexAnneal(T0,decay)` | Same-slot Metropolis; permitted empty values fall back to 100.0/0.95 |
| `indexAging()` | Same-slot greedy acceptance with stagnation age; scout is a separate activation stage |
| `kinshipAging()` | ABC onlooker selection by `parent_id`, with age tracking |
| `kinshipGreedy()` | Replace the source elite slot only when its descendant improves |
| `rank()` | Best merge of parents and offspring, (μ+λ) |
| `close(better)` / `closeAnneal(T0,decay)` | Replace the most similar individual |
| `worstReplace()` | Unconditionally replace the worst λ members |
| `distanceSelect()` | Preserve the best and use distance density for diversity |

Age and kinship features are inferred with Singular scope.

### 3.5 Other families

Evaluators: `basic()` evaluates each candidate; `hash()` caches duplicates and cache hits consume no FES. Repairs: boundary clamp, modular wrap, random, heuristic greedy, and no repair. Initializers: `Random`, `Greedy`, `R_G`, `Distribution`, `No`. Individual state is inferred, with only the base `Individual`. Archives are `Basic`, `Multiobject`, and `Multimodal`. Managers include `Single` and multi-subpopulation cooperation variants.

## 4. Termination and repeated runs

Optimizer and subpopulation levels each have `maxFES`, `maxConvergence`, and `maxTime`. `exe(seed)` seeds one run; `exe(n,seed)` seeds once for n runs and records each; `continue_exe(new_terminator)` continues with a new terminator. Retrieve results with `getBest(best,size)`.

## 5. Parallel subpopulations

Default execution is serial. Define `EVOFLOW_PARALLEL_SUBPOP` at compilation (MSVC `/D` or CMake definitions) for subpopulation-level parallel epochs. Multiple island groups can benefit; one group stays serial without spawning workers. Cheap evaluations may be dominated by thread overhead; the documented expensive-evaluation experiment achieved about 6× with eight groups on sixteen cores, not a general speed guarantee.

Independent deterministic RNG streams, problem handles, preallocated budgets, and ordered result gathering make supported serial/parallel runs bitwise consistent for the same seed, independent of scheduling. User objectives/constraints must be thread-safe: avoid shared mutable global/static caches and use framework per-subpopulation RNG helpers (`get_int`, `rand01`, `get_normal`, etc.), not a shared global generator. The framework emits a runtime reminder but cannot statically enforce this contract.

## 6. Serialization

```cpp
ConfigBuilder::saveConfig(fc, "myrun", "v3.1", "config");
FullConfig loaded = ConfigBuilder::loadConfig("myrun", "config");
```

Both formats use `.cfg`. First-line version markers select `<ver= v3.1>` text or `{"version":"v3.2"...}` JSON. Missing files/unknown versions throw. Codecs are `ConfigCodecV31`, `ConfigCodecV32`, and `detectVersion(text)`.

Allowed empty parameters mean internal defaults/adaptation, including `tao_ini`, `factor`, `w_final`, and `subset_rate`. Serialize as `nan` in v3.1 and `null` in JSON; zero is a real value, not emptiness. Empty PSO final inertia preserves random-inertia mode. Empty subset rate chooses a random probability; zero chooses an empty subset. The decoder tolerates old invalid v3.2 bare `nan` output from before v1.4.6.3, but new JSON always writes `null`.

<a id="recipes"></a>
## 7. Algorithm recipes

These component combinations come from `tutorial/config-generate*.cpp`. `gen` means Generation, `oc` OrderedConstruct, `pc` ParallelConstruct; `index(T/F)` means greedy/unconditional. Features are inferred automatically. Consult the [catalog](24-catalog.md) for full parameters.

| Algorithm | Features | Topology | Strategy | Generator | Selector | Repair |
|---|---|---|---|---|---|---|
| PSO | velocity | PGBest | VelocityDriven | oc | index(F) | random |
| CSO | velocity | Competition | VelocityDriven | oc | index(F) | boundary |
| LLSO | velocity | LevelBasedLearning(L) | VelocityDriven | oc | index(T) | random |
| SDLSO | velocity | StochasticDominantLearning | VelocityDriven | oc | index(T) | random |
| SetPSO/SLLSO/SSDLSO | setvelocity | Corresponding topology above | SetVelocityDriven | oc | index | random |
| Sticky BPSO | velocity | PGBest | StickyBinary | gen | index(F) | no |
| GA | None | Tournament + Continue | Crossover + mutation | gen + gen(true) | index(F)/rank | random/boundary/greedy |
| DE | None | RandomLearning(2) | DifferenceCrossover(cr,F) | gen | index(F) | random |
| AS | None | Isolate | AntSystem | oc | index(F) | random |
| ACS | None | Isolate | AntColonySystem | pc | index(F) | random |
| MMAS | None | Isolate | MaxMinAntSystem | oc | index(F) | random |
| EDA | None | Isolate | DistributionEstimation | gen | index(F) | random |
| One-fifth ES | None | Isolate | GaussianOneFifth | gen | rank | boundary |
| Self-adaptive ES | sigma | Isolate | GaussianSelfAdapt | gen | rank | boundary |
| CMA-ES | None | Isolate | CMAES | gen | rank | boundary |
| Hill climbing | None | Isolate | Neighborhood mutation | gen | index(T) | no/boundary |
| Simulated annealing | None | Isolate | Same neighborhoods | gen | indexAnneal | no/boundary |
| Tabu search | None | Isolate | TabuDecorator / TabuBit | gen | index | random |
| GWO | None | TopRanked(3) | GreyWolfEncircling(2,0) | gen | rank | boundary |
| WOA | whalestate | LeaderAndRandom | WhaleForaging(2,0,1) | oc | index(F) | boundary |
| CLONALG | None | ClonalExpansion then RandomSelect | AffinityHypermutation then RandomGeneration | gen + gen | rank then worstReplace | boundary |
| FWA | fireworkstate | FireworkExplosion then RandomSelect | ExplosionSpark then GaussianSpark | gen + gen | distanceSelect | modular |

### ABC: three stages per generation

| Stage | Topology | Strategy | Generator | Selector |
|---|---|---|---|---|
| Employed | `randomLearning(1)` | `differencePerturbation()` | gen | `indexAging()` |
| Onlooker | `rouletteSource()` | `differencePerturbation()` | gen | `kinshipAging()` |
| Scout | `ageActivation(limit)` | `reinitialize()` | gen | `index(false)` |

One preceding boundary repair applies to all stages. Setter order is execution order. Employed/onlooker acceptance maintains age; scout activates when `age>limit`. `RunGenerate` assigns population-slot kinship IDs and birth derives `parent_id`. `ageActivation` requires age-maintaining acceptance in the same workflow or validation reports missing `ageMaintained`. See `smoke-abc.cpp`.

### Jaya, iterated greedy, and WWO

Jaya uses `bestAndWorst → bestWorstGuided → oc → index(true)` and is parameter-free. Iterated greedy uses `isolate → destructRebuild(d) → oc → index(true)` or annealing, with greedy repair. Ordered construction lets `getPrioriChoice` see the actual partial solution and retained decisions.

| WWO stage | Topology | Strategy | Generator | Selector |
|---|---|---|---|---|
| Propagation | Isolate | WavePropagation(h_max,alpha,lambda_init) | oc | index(true) |
| Refraction | WaveRefractionActivation (h≤0, others pass through) | WaveRefraction(h_max) | oc | index(false) |
| Breaking | NewBestActivation(k_max), only on a new best | WaveBreaking(beta_max,beta_min) | gen | kinshipGreedy |

The Singular `wavestate` stores height/wavelength. See `jaya`, `wwo`, `ig_greedy`, `ig_anneal` configurations and matching smoke tests.

### Advanced neighborhoods

Path relinking uses `repeatWithBest(k) → pathRelinking() → gen → index(true)`. A per-child `beta~U(0,1)` is held in `relinkdepth`, giving k sampled points along a guided path. ALNS uses `isolate → adaptiveOperatorSelection(rho,segment_len,ops) → gen → index(true)` or annealing. Each generation selects a pool operator by roulette, scores outcomes with σ1/σ2/σ3, and updates weights each segment.

| Operator | Meaning | Encoding |
|---|---|---|
| `cyclicExchange(k,rate)` | Rotate k distinct positions; k=2 is exchange; preserves multiset | Permutation/sequence |
| `blockSwap(times,rate)` | Swap arbitrary separated blocks, possibly unequal lengths; generalizes adjacent relocation and single-position exchange | Permutation/sequence |
| `multiPointSameValue(point_rate)` | Independently select dimensions and assign one shared random value | General/allocation |
| `setExchange(subset_rate)` | Swap values of subsets from two value classes; empty rate samples each side independently | Allocation/grouping |
| `classRelocate(subset_rate)` | Move a subset of one class to another value, including unused values | Allocation/grouping |
| `classMerge(rate)` | Merge an entire class into another existing class, reducing used-value count by one | Allocation/grouping |

These cover swapping, splitting/migration, merging, and coordinate collapse. See the corresponding `smoke-pathrelink/alns/multipoint/cyclicexchange/setexchange/classrelocate/blockswap/classmerge.cpp` tests.

### Elite local search

Append an intensification stage after the main algorithm:

| Source | Topology | Strategy | Selector |
|---|---|---|---|
| Population top-e | `topIndividual(e,r)` | DifferencePerturbation/Gauss/etc. | `kinshipGreedy()` replaces the originating elite slot on improvement |
| Archive elites | `elite(e,r)` | Gaussian etc.; self endpoint cannot support a nonzero differential direction | `rank()` global merge |

Kinship is inferred by its consumer and assigned by generic generation. See `elite_pop_pso`, `elite_archive_pso`, `tutorial/6.elite-local-search.cpp`, and `smoke-elite-ls.cpp`.

### Multiobjective recipes

Use `Multiobject` archives and equal objective priorities, with all objectives minimized for these recipes.

| Algorithm | Topology | Strategy | Generator | Selector | Repair |
|---|---|---|---|---|---|
| NSGA-II | Tournament(2,1) + Continue | SBX + PM | gen + gen(true) | RankCrowding | boundary |
| MOEA/D | Neighborhood(T,delta) + Continue | SBX + PM | gen + gen(true) | ScalarReplace(nr,T), matching topology T | boundary |

See `tutorial/config-generate-multiobjective.cpp` and `tests/optimizer/multiobjective/smoke-nsga2.cpp` / `smoke-moead.cpp`. Complete runnable configurations remain the best reference for assembling each recipe.
