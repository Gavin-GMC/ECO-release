# 24 · Component Catalog

[English](24-catalog.md) | [简体中文](../手册/24-开发-组件清单.md) | [Manual index](README.md)

This catalog lists component tags, behavior, and parameters by family. Exact named setter signatures are in `config-setter.h`; machine-readable parameter schemas come from the registries. See [base classes](23-base-classes.md) and [configuration](11-optimizer-configuration.md).

In the maintainer source tree, the Chinese catalog is checked bidirectionally against `App::listComponents()` by `tests/app/smoke-component-catalog.cpp`: missing registered entries and listed unregistered entries fail that regression. Initializers, distribution models, and feature class/kind naming are special cases. Keep both language catalogs synchronized when adding components; the existing native test specifically targets the Chinese source catalog.

## 1. Learning topologies (32)

Registry: `LearningTopology`.

| Tag | Behavior | Parameters |
|---|---|---|
| `PGBest` | Personal best + global best, two objects | — |
| `RandomLearning` | n random learning objects per source | `number` |
| `Isolate` | No learning objects; self-perturbation | — |
| `Elite` | Repeat archive elites using an internal buffer, not population slots; use Rank merge | `elite_number`, `repeat_times` |
| `TopIndividual` | Repeat current top-e population slots with a random companion; supports kinship local search | `elite_number`, `repeat_times` |
| `BestAndWorst` | Direction-aware current best/worst as endpoints 0/1 | — |
| `RepeatWithBest` | k sources per member guided by current best, skipping best itself; graph size `(n−1)k` | `repeat_times` |
| `WaveRefractionActivation` | Activate waves with `h≤0`, guided by archive best; others pass through without evaluation | Reads wavestate height |
| `NewBestActivation` | Own a copy of the previous elite; only a new best activates `k~U[1,k_max]` same-origin sources, otherwise empty graph | `k_max` |
| `Competition` | Random pairs: winner passes through, loser learns from winner and per-dimension population mean; objects=2, coupled=2; empty members omitted from means | — |
| `LevelBasedLearning` | Level-based learning | `level_number` |
| `StochasticDominantLearning` | Stochastic dominant learning | — |
| `Continue` | Current offspring are sources and sole learning objects; requires offspringAvailable and provides offspringStart; supports inplace | — |
| `Inherit` | Previous graph sources with current offspring as sole objects; requires offspringAvailable and lastGraphAvailable; no inplace | — |
| `NoChange` | Copy last graph without reading offspring; inherits objects/coupled/graphScale, not source/behavior capabilities; no inplace | Requires lastGraphAvailable |
| `Roulette` | Fitness-weighted object selection | `object_number` |
| `Tournament` | Tournament-selected objects | `competition_scale`, `object_number` |
| `Uniform` | Uniform random objects | `object_number` |
| `TopRanked` | Current top k, e.g. three GWO leaders | `leader_count` |
| `LeaderAndRandom` | Global best and a random member | — |
| `ClonalExpansion` | More clones for better antibodies; objects=0 | `clone_factor` beta |
| `RandomSelect` | d random sources; objects=0 | `count` |
| `FireworkExplosion` | Fitness-dependent spark count; objects=0 | `M` base total, `a/b` lower/upper fractions, `0<a<b<1` |
| `Neighborhood` | Two parents from T nearest weight neighbors for MOEA/D, otherwise whole-population mating | `T`, `delta` neighborhood probability (default 0.9) |
| `RouletteSource` | ABC onlooker: fitness roulette with replacement chooses sources, each with one different random companion | — |
| `AgeActivation` | `age>limit` sources activate with a self endpoint; others pass through; requires ageMaintained | `limit` |
| `LeaderTwoRandom` | Global best plus two random members, objects=3; reusable for guided differential motion | — |
| `LeaderRandomCenter` | Global best, random member, per-dimension mean; means ignore empty values and stay empty when all are empty | — |
| `DungBeetleRefs` | Archive best-so-far, current best, current worst for DBO roles | — |
| `SalpChain` | Global food source and previous salp's parent position; first leader has no previous member; synchronous/parallel chain | — |
| `RankExtremesTriRandom` | Best/better/worst plus three randomly sampled members sorted by quality; current population sources, objects=6 | — |
| `ContinueRankExtremesTriRandom` | Same six objects but current offspring sources; requires offspringAvailable, provides offspringStart, supports inplace | — |

## 2. Learning strategies (68)

Registry: `LearningStrategy`.

### Position and velocity updates

| Tag | Behavior | Parameters |
|---|---|---|
| `VelocityDriven` | PSO; ignore empty guides per dimension, empty velocity gives no inertia displacement; recover empty position from first valid guide, otherwise preserve empty | `object_number` (default 2), `c`, `w_ini`, `w_final` (empty: per-dimension random inertia) |
| `SetVelocityDriven` | SetPSO; sequence edges require nonempty endpoints, empty positions skip crisping | `object_number`, `c`, `w_ini`, `v_heuristic`, `f_heuristic`, `w_final` (empty: per-round random inertia) |
| `BinaryVelocityDriven` | Binary PSO with sigmoid switching | `c1`, `c2`, `w_ini`, `w_final` (empty: per-round random inertia) |
| `StickyBinary` | Sticky BPSO | `c` base strength `i_s=c/N`, `ratio_p:ratio_g` personal/global attraction ratio, `ustkS` sticky-generation scale |

VelocityDriven and SetVelocityDriven require at least `object_number` topology objects: too few is Error, extra is Info. Nonempty `w_final` is the final inertia, interpolated by subpopulation termination progress as described in [configuration](11-optimizer-configuration.md).

### Distributions and evolution strategies

| Tag | Behavior | Parameters |
|---|---|---|
| `DistributionEstimation` | EDA; filter empty samples per dimension, return empty if all samples are empty | `model` 1 Gaussian/2 Cauchy/3 Uniform/4 Histogram, `good_number` (otherwise use ratio), `good_rate` |
| `GaussianOneFifth` | Global step size with one-fifth success rule | Initial `sigma`, `adapt<1` (e.g. 0.85) |
| `GaussianSelfAdapt` | Self-adaptive per-individual sigma feature | — |
| `CMAES` | Persistent covariance distribution from complete solutions; no complete first-generation solution uses finite-domain midpoint; later fewer than μ complete parents freeze the update | Initial `sigma`; other values derived from n/λ |

### Ant colony optimization

| Tag | Behavior | Parameters |
|---|---|---|
| `AntSystem` | AS | `alpha` pheromone weight, `belta` heuristic weight, `rho` evaporation, `tao_ini` initial pheromone; see schema for complete order |
| `MaxMinAntSystem` | Bounded pheromones and best-only updates | `alpha`, `belta`, `rho`, `p_best` for pheromone bounds, `use_global`, `use_local` |
| `AntColonySystem` | Global/local updates and pseudo-random proportional choice | `alpha`, `belta`, `rho_g`, `rho_l`, `q0`, `use_global`, `use_local`, `tao_ini` |

AS/ACS detect an empty path predecessor before integer conversion and return an empty current decision; MMAS inherits the AS guard. Pheromone updates also guard empty current/predecessor values.

### GWO, WOA, immune, and fireworks strategies

| Tag | Behavior | Parameters |
|---|---|---|
| `GreyWolfEncircling` | GWO with TopRanked(3) | `a_max/a_min`, default 2/0 |
| `WhaleForaging` | WOA with inferred whalestate and LeaderAndRandom | `a_max/a_min`, spiral constant `b` (default 1) |
| `AffinityHypermutation` | Affinity-dependent mutation gate around Gaussian mutation | `sigma`, worst-antibody `base_rate`, affinity decay `rho` |
| `RandomGeneration` | Uniform resampling for immune replacement | — |
| `ExplosionSpark` | Fitness-dependent explosion amplitude | `amp_max`, per-dimension displacement probability `dim_rate` |
| `GaussianSpark` | Multiply selected dimensions by N(1,1), using fireworkstate | `dim_rate` |

### Additional continuous swarm strategies

| Tag | Behavior / topology | Parameters |
|---|---|---|
| `SineCosineOscillation` | SCA sine/cosine approach to TopRanked(1), linearly shrinking amplitude | Initial `a` |
| `ArithmeticOperatorUpdate` | AOA division/multiplication/addition/subtraction around TopRanked(1); increasing MOA chooses phase, decreasing MOP sets step | `alpha`, `moa_min/moa_max`, `mu` |
| `EquilibriumUpdate` | EO mass-balance update toward one of top four candidates or their mean | `a1` exploration, `a2` exploitation, `GP` generation probability, `V` volume |
| `SlimeMouldOscillation` | SMA weighted oscillation toward best/two random members or contraction; occasional resampling; LeaderTwoRandom | `z` resampling probability |
| `MarinePredatorMotion` | MPA three Brownian/Levy phases, then FADs boundary/differential disturbance; LeaderTwoRandom | `P`, `fads` probability |
| `ReptileEncircling` | RSA four exploration/hunting phases; LeaderRandomCenter; empty random/mean guides fall back to leader, empty position recovers from valid leader, empty leader preserves position | `alpha`, `beta` |
| `AquilaHunt` | AO four hunting patterns: first 2/3 exploration, last 1/3 exploitation, random branch within phase; same empty-guide handling as RSA | `alpha/delta`, spiral `r1`, `U/omega`, initial phase `theta1` |
| `GoldenJackalPincer` | GJO best/second-best pair, declining escape energy, exploration at absolute energy ≥1, exploitation below; Levy motion; TopRanked(2) | `c1` energy coefficient, Levy exponent `beta` |
| `VultureForaging` | AVOA two leaders, three satiation phases with two branches each including spiral/Levy | `L1`, branch probabilities `P1/P2/P3`, `omega`, spiral `zb/hb` |
| `BelugaWhalePhases` | BWO exploration through paired sin/cos dimensions, Levy-guided exploitation, whale-fall phase; LeaderAndRandom | `wf_max/wf_min`, `b0_max`, `c1_fac/c2_fac` |
| `DungBeetleRoles` | DBO rolling/breeding/foraging/stealing roles; DungBeetleRefs; rolling keeps previous position in a solution feature | `k`, `b`, `S`, `obstacle_p`, `p_roll/p_brood/p_forage` (remainder stealing) |
| `TunicateJetSwarm` | TSA jet propulsion toward TopRanked(1), then averaging with current position; random social-force collision avoidance, no progress decay | `pmin/pmax` |
| `GrasshopperSwarm` | GOA all-pairs social attraction/repulsion plus target attraction; shrinking comfort factor; skip empty neighbors, recover empty positions from valid target, preserve position if target empty; inherently O(n²) | `f`, `l`, `c_max/c_min` |
| `SalpChainUpdate` | SSA leader follows food source; followers average with preceding parent position; SalpChain | `c1_base`, `c1_rate` |
| `WeightedMeanUpdate` | INFO update/vector combination: MeanRule produces z1/z2 and combines into u; RankExtremesTriRandom | `a_base`, `a_rate` |
| `WeightedMeanLocalSearch` | Reusable INFO refinement around best/random points using weighted direction and Gaussian perturbation; ContinueRankExtremesTriRandom | Trigger `prob` |

### ABC

| Tag | Behavior | Parameters |
|---|---|---|
| `DifferencePerturbation` | Change one random dimension using `v=x_i+phi*(x_i-x_k)`, phi in [-1,1]; employed/onlooker share it | None; needs ≥1 object |
| `Reinitialize` | Resample a solution and reset features, including age through initialization; does not evaluate and has no age dependency | — |

### Jaya, iterated greedy, and water-wave optimization

| Tag | Behavior | Parameters |
|---|---|---|
| `BestWorstGuided` | Jaya `X'=X+r1*(best-abs(X))-r2*(worst-abs(X))`, independent uniform coefficients per dimension/generation; BestAndWorst + greedy Index | None |
| `DestructRebuild` | Destroy d dimensions and greedily reconstruct with the problem handle's `getPrioriChoice`; Isolate + OrderedConstruct; selector controls acceptance | `destruct_size` |
| `WavePropagation` | `x+U(-1,1)*lambda*L`; generation preparation decays wavelengths, post-update maintains height (reset on improvement, decrement otherwise); requests wavestate | `h_max`, `alpha`, `lambda_init` |
| `WaveRefraction` | Sample `N((x_best+x)/2, abs(x_best-x)/2)`, reset height, update `lambda'=lambda*f(x)/f(x')`; activation topology and unconditional Index | `h_max` |
| `WaveBreaking` | One random dimension perturbed by `N(0,1)*beta*L`; beta decreases with progress; NewBestActivation + Kinship greedy | `beta_max/beta_min` |

### Adaptive neighborhoods and tabu search

| Tag | Behavior | Parameters |
|---|---|---|
| `PathRelinking` | Keep equal dimensions; differing dimensions copy guide with per-child beta~U(0,1), stored in relinkdepth; RepeatWithBest gives sampled path points | — |
| `AdaptiveOperatorSelection` | Weighted roulette over a low-level pool each generation; score new best/improvement/acceptance with σ1/σ2/σ3; update `w←w(1-rho)+rho*(pi/theta)` per segment; constructive capability is the intersection of all possible pool members | `rho`, `segment_len`, pool `ops` |
| `TabuDecorator` | Wrap a neighborhood and reject tabu differences | `inner`: 0 overturn/1 insert/2 exchange/3 segment relocation/4 reorder; `tenure`; full inner parameters |
| `TabuBit` | Tabu search with built-in point mutation | `tenure` |

### Crossovers

The final `coupled` parameter controls paired generation and defaults true.

| Tag | Meaning and parameters | Encoding |
|---|---|---|
| `Point` | Multipoint crossover: `point_number`, `cross_rate` | General |
| `Uniform` | Uniform crossover: `cross_rate` | General |
| `SBX` | Simulated binary crossover: `eta` (larger stays nearer parents), `cross_rate` | Continuous |
| `Difference` | DE: `cross_rate`, scaling `factor` | Continuous |
| `PartialMapped` | PMX: `cross_rate` | Permutation/sequence |
| `Cycle` | Cycle crossover: `cross_rate` | Permutation/sequence |
| `Order` | Order crossover: `cross_rate` | Permutation/sequence |
| `SubtourExchange` | Subtour exchange: `cross_rate` | Permutation/sequence |
| `PositionBased` | Position-based crossover: `cross_rate`, selected-position `proportion` | Permutation/sequence |

### Mutations

| Tag | Meaning and parameters | Encoding |
|---|---|---|
| `Bit` | Point mutation; per-dimension `mutation_rate` | General/binary |
| `BitFlip` | Bit flip; per-dimension `mutation_rate` | Binary |
| `PM` | Polynomial mutation: `eta`, per-dimension `mutation_rate` (often 1/n) | Continuous |
| `Gauss` | Add Gaussian noise to selected dimensions: `sigma`, per-dimension `mutation_rate` | Continuous |
| `Overturn` | Segment reversal: `times`, whole-operation `mutation_rate` | Permutation/sequence |
| `Exchange` | Position exchange: `times`, whole-operation `mutation_rate` | Permutation/sequence |
| `Insert` | Insertion: `times`, whole-operation `mutation_rate` | Permutation/sequence |
| `Reorder` | Segment reordering: `times`, whole-operation `mutation_rate` | Permutation/sequence |
| `SegmentRelocate` | Segment relocation: `times`, whole-operation `mutation_rate` | Permutation/sequence |
| `CyclicExchange` | Rotate k distinct positions; k=2 exactly reduces to exchange, preserving value multiset | `cycle_length`, `mutation_rate` |
| `BlockSwap` | `prefix+A+M+B+suffix → prefix+B+M+A+suffix`; unequal/separated blocks allowed, M order preserved; empty M gives segment relocation, singleton A/B gives exchange | `times`, `mutation_rate` |
| `MultiPointSameValue` | Independently select dimensions and set all selected ones to one shared domain value | `point_rate`; general/allocation |
| `SetExchange` | Select subsets of two existing value classes and swap their values; rate 1 swaps complete classes | `subset_rate`; empty samples each side's rate independently |
| `ClassRelocate` | Move a subset of one class to a sampled domain value; unused targets split/create a class, used targets merge partly | `subset_rate`, may be empty |
| `ClassMerge` | Move an entire class into another existing class, reducing used-value count by exactly one | Whole-operation `mutation_rate` |

Rate semantics are encoded in the class hierarchy. Bit/BitFlip/PM/Gauss derive from `PerDimMutation`, whose `nextDecision` performs the probability gate before calling `mutatedValue`. New per-dimension mutations should use this base rather than duplicate gating. Whole-solution overrides use one apply/skip decision. `point_rate` and `subset_rate` are separately named concepts. GA/DE remain compositions of topologies, crossover, and mutation.

## 3. Generators (4)

| Tag | Behavior | Parameter |
|---|---|---|
| `Generation` | Whole-solution generation plus repair | `inplace` |
| `GenerationNoCheck` | Whole-solution generation without repair | `inplace` |
| `OrderedConstruct` | Sequential dimension construction, requires constructive strategy | `inplace` |
| `ParallelConstruct` | Dimension-synchronous construction, independent handle/propagation per individual | `inplace` |

Successful generation provides `offspringAvailable`, `unevaluatedOffspringAvailable`, and `lastGraphAvailable`. Evaluation clears only unevaluated state; selection clears offspring and unevaluated state. The last graph survives until the workflow invocation ends. Continuation topologies must request the actual states they read, not a generic historical-stage flag.

At every generator, matcher checks currently active topology/strategy/repair slots. Replaced SET components do not keep contributing capabilities. Cross-stage contracts use explicit runtime state or workflow-wide capabilities. The current workflow-wide capability is Aging selection's `ageMaintained`, consumed by AgeActivation. In-place generation modifies existing offspring only under the offspring-source/unevaluated contract.

## 4. Environmental selectors (8)

| Tag | Behavior | Parameters |
|---|---|---|
| `Index` | Same-position replacement | `accept_type`: 0 unconditional, 1 better, 2 anneal, 3 aging |
| `Rank` | Best merge of μ parents and λ offspring | — |
| `Close` | Replace the nearest member by Euclidean distance | `accept_type` |
| `WorstReplace` | Unconditionally replace the worst λ, inferred from offspring count | — |
| `DistanceSelect` | Preserve best and select by distance density | — |
| `RankCrowding` | Merge μ+λ, fill nondomination fronts, truncate last front by crowding | — |
| `ScalarReplace` | Tchebycheff replacement of up to nr neighbors per offspring; monotone historical ideal point | `nr`, `T` matching Neighborhood |
| `Kinship` | Match offspring parent_id to originating slot; multiple offspring may target one parent | `accept_type`; Singular kinship |

Only `accept_type` belongs to Index/Close/Kinship themselves. Progressive schemas append `T0,decay` only for annealing; allowed empty values default to 100.0/0.95. Types 0/1/3 append nothing.

Acceptance rules: Unconditional always replaces; Better only improves; Anneal always accepts improvement and accepts deterioration with `exp(-delta/T)`; Aging is greedy plus stagnation counting (improvement resets age, otherwise increments it). Aging requests Singular age. Scout regeneration is separate (`AgeActivation + Reinitialize`) to avoid onlooker many-to-one pullback. Terminator/archive arguments remain in the common signature, though split Aging no longer uses them. Aging's `acceptPostAssert` provides `ageMaintained`; missing maintenance for AgeActivation is a hard assembly error.

## 5. Evaluators (2)

| Tag | Behavior |
|---|---|
| `Basic` | Real evaluation per candidate, each spending FES |
| `Hash` | Duplicate cache; hits restore fitness without FES |

## 6. Initializers (5; builder dispatch)

| Tag | Behavior | Parameter |
|---|---|---|
| `Random` | Domain-uniform sampling | — |
| `Greedy` | Heuristic-priority construction | — |
| `R_G` | Random with probability prob, otherwise greedy | `prob` |
| `Distribution` | Distribution sampling | `model`: 1 Gaussian/2 Cauchy/3 Uniform/4 Histogram |
| `No` | No initialization | — |

These are selected by `OptimizerBuilder::Ibuild`, not a registry.

## 7. Repairs (5)

| Tag | Behavior |
|---|---|
| `Boundary` | Clamp to nearest boundary |
| `Modular` | Modular wrap into the domain, used by FWA |
| `Random` | Random domain value |
| `Greedy` | Heuristic-priority choice |
| `No` | Preserve the original value |

## 8. Archives (3)

| Tag | Behavior |
|---|---|
| `Basic` | One global best |
| `Multiobject` | Pareto set |
| `Multimodal` | Multiple decision-space solutions deduplicated by accuracy |

Accuracy is not yet exposed as an archive ParameterTemplate parameter. `Historic` is unregistered; its incomplete comparer initialization is tracked internally.

<a id="features"></a>
## 9. Individual features

There is only the base `Individual`, no Particle/SetParticle subclasses or `Registry<Individual>`. Components declare feature demands; assembly infers/attaches them, and runtime access uses identity keys.

| Kind / class | Storage | Uses |
|---|---|---|
| `vector` / `VectorFeature` | Decision-length real vector; Zero/Constant/RandomInDomain initialization | PSO/BPSO velocity, ES sigma |
| `scalar` / `ScalarFeature` | k scalars | whalestate k=4 (p/A/C/l), fireworkstate k=1 (g), Singular wavestate k=2 (height/wavelength) |
| `solution` / `SolutionFeature` | One Solution | Generic stored solution, e.g. previous position |
| `pbest` / `PbestFeature` | Historical best, inherited at birth and improved after evaluation | PGBest, Singular |
| `setvelocity` / `SetVelocityFeature` | One SetVelocity matrix per variable | SetPSO |
| `age` / `AgeFeature` | Integer stagnation age; get/reset/ageing | Aging writes, AgeActivation reads, Reinitialize resets; Singular |
| `kinship` / `KinshipFeature` | `id`, `parent_id`, direct `isChildOf` relation | Birth derives parent identity; ABC/local search; Singular |

Scopes: Private is per component instance but can be shared through channels; Exclusive forbids sharing; Singular is unique per individual and shared by all readers. A Singular identity key is the role constant. Add features/demands for new individual state rather than new individual subclasses.

## 10. Subpopulation managers (4)

| Tag | Behavior |
|---|---|
| `Single` | One group, no cooperation |
| `NoInteraction` | Independent groups with global archive collection; normally Fixed constructer |
| `Immigrant` | Send each group's best to topology neighbors, replacing worst members; see setters for migration settings |
| `RebuildTopology` | Regroup during execution using Distance/Fitness constructers |

## 11. Subpopulation constructers (3)

| Tag | Behavior |
|---|---|
| `Fixed` | Fixed partition |
| `Distance` | Regroup by solution distance |
| `Fitness` | Regroup by fitness |

## 12. Subpopulation topologies (5)

| Tag | Behavior |
|---|---|
| `Connected` | All groups adjacent |
| `Star` | Hub-and-spoke |
| `Toroidal` | Toroidal grid |
| `Preswarm` | Predecessor chain |
| `NoConnect` | No edges |

## 13. Distribution models (4)

`DistributionFactory` supplies EDA and Distribution initialization:

| Model | Meaning |
|---|---|
| `Gaussian` (1) | Welford mean/variance and Box–Muller sampling |
| `Cauchy` (2) | Heavy-tailed Cauchy |
| `Uniform` (3) | Interval-uniform |
| `Histogram` (4) | Marginal histogram, supporting multimodal data |

Terminator and Subpopulation are single classes, not registry families. Configure limits through `maxFES/maxConvergence/maxTime`, and groups through `subpopulation(id).size(n).workflow(w)`.

## Appendix · Adding a component

### A. Implementation

- Derive from the correct base and implement required lifecycle hooks.
- Use behavioral names for pluggable operators.
- Register `{name, category, params, size, create, pre, post}` with `EVOFLOW_REGISTER`.
- Put per-individual state in feature demands and implement `setFeatureKey`; choose scope deliberately.
- Declare requirements/provisions in the component's own implementation. The matcher implements matching rules, not component-specific declarations.

### B. Parameter schema

`ParameterTemplate` is authoritative for named setters, automated sampling, and validation. Fill all fields `{name, kind, low, high, allow_empty, rec_low, rec_high}`. Hard bounds differ from recommended sampling bounds; Int/Enum require integrality.

`allow_empty=true` is a constructor-level contract: handle `is_empty(v)` in the constructor so every creation path honors the default/adaptive meaning. Otherwise NaN can silently disable behavior, as once happened with annealing. If zero already means disabled and no absent-value semantics exist, do not mark the parameter empty-capable.

Declare only the component's own parameters. Use progressive `next` schemas for selected acceptance criteria, nested operators, or repeated entries. For example, Index declares accept_type and delegates annealing parameters; TabuDecorator returns its selected inner schema intact; ALNS count determines the number of operator entries.

`next` consumes its current-level slice, using relative indices. It returns a description, not a boolean verdict, so generators can ask what parameters come next. Nested templates keep their own next hooks; consumers expand levels with a depth limit. Factory positional reads and schema arity must match exactly to prevent out-of-bounds access.

### C. Configuration and documentation

Add a named setter with schema-matching order and clear semantics; add generated configurations. Update both catalogs, configuration recipes/operator descriptions, and any new base-class reference. Update counts and the development changelog.

### D. Validation

Add an English-output smoke test returning failure when checks fail. Verify observable behavior, not merely absence of crashes or constant-true assertions. Prefer structural proofs where possible: distinct input values can expose exact permutations/block boundaries. Compare against the nearest existing operator to demonstrate the intended difference, such as ClassRelocate activating an unused value while SetExchange cannot. Run focused behavior and assembly checks for the change, and broader regression tests appropriate to its impact.

### E. Silent failure traps

| Trap | Effect |
|---|---|
| `== EMPTYVALUE` | NaN equality is always false; guard never fires |
| NaN in ordinary comparisons | Probability/range tests silently fail |
| Unimplemented allow_empty promise | NaN propagates into behavior |
| Factory reads beyond parameter array | Undefined reads that may appear to work |
| Individual state in shared component members | State becomes detached from individuals during swaps/preparation |
| Constant-true tests | Green results without coverage |
