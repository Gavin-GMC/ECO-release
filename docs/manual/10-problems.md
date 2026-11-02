# 10 · Problem Modeling

[English](10-problems.md) | [简体中文](../手册/10-使用-问题模块.md) | [Manual index](README.md)

Add variables, objectives, constraints, and heuristics to `Problem`, then call `compile()` to obtain the optimizer-facing `ProblemHandle`. See `tutorial/0.problem.cpp` for a complete program. The signatures below are API sketches; use the declared C++ types in the headers when implementing a caller.

```cpp
#include "evoflow.h"
using namespace EvoFlow;

Problem p("my_problem");
// Add variables, objectives, constraints, and heuristics.
ProblemHandle* h = p.compile();
```

## 1. Variables

```cpp
int addVariable(name, low_bound, up_bound, accuracy, length,
                height = 1, type = VariableType::discrete);
```

`name` is used by objectives/constraints/heuristics. Bounds define `[low, up]`. `length` is the number of elements, for example five components `x[0..4]`; `height` describes a multirow layout and is normally omitted. The return value is a variable ID. Total decision dimension is the sum of variable lengths.

### 1.1 Type and accuracy

The domain implementation uses sets of continuous intervals, interpreted according to `type`:

| Type | Domain interpretation | `accuracy` | Sampling and generated values |
|---|---|---|---|
| `discrete` (default), `allocation`, `sequence_direction`, `sequence_bidiagraph`, `sub_module` | Grid `{low + k·accuracy}` | Grid spacing | Uniform over grid points; values snap to the grid |
| `continuous` | Real interval `[low, up]` | Resolution for removing a neighborhood and for display | Uniform by interval length; values remain unquantized |

Continuous accuracy does not change decision values. It defines a neighborhood when removing a value (a singleton has zero measure in a continuous domain) and output precision.

The documented five-dimensional sphere example on `[-5,5]`, accuracy `0.3`, illustrates the difference: continuous variables achieved approximately `5.17e-93`, with `x[0]≈1.97e-55`; discrete variables stopped at `0.05`, with the nearest grid value `0.1 = -5 + 17×0.3`. The discrete lower bound is `5×0.1²`, not an optimizer defect or a promise about every run.

```cpp
p.addVariable("x", -100, 100, 1e-5, 30, 1, VariableType::continuous);
p.addVariable("s", 0, n-1, 1, n);
```

Declare continuous variables explicitly. A tiny discrete step is not equivalent: `[-100,100]` with `1e-5` creates about twenty million grid points, affecting enumeration and counting. Continuous candidate enumeration/counting instead samples at most `FEASIBLE_LIST_LIMIT` points by measure. `getChoiceNumber` and the returned feasible-list length need not be identical.

Continuous domains are structurally incompatible with ACO pheromone matrices and SetPSO velocity tables, both of which require enumerable candidates per dimension.

## 2. Objectives

```cpp
void addObjective(name, priority, min_is_better, input_elements, double(*f)(double**));
void addObjective(name, priority, min_is_better, input_elements, eccalcul_functor* f);
void addObjective(name, priority, min_is_better, input_elements, formula);
```

Use a function pointer, a possibly stateful functor, or a muParserX formula. Larger priorities take precedence lexicographically. Objectives excluded by the comparator's priority rules may serve as penalty hosts; the Chinese reference notes that negative priorities are excluded and that zero-priority behavior must follow the implementation. `min_is_better=true` minimizes, `false` maximizes. `input_elements` lists names such as `"x"` or `"x,y"`.

<a id="objective-input-layout"></a>
### Objective input layout

For `double f(double** a)`, **`a[k][j]` is element j of input variable k**, with variables ordered by `input_elements`:

```text
inputs "x,y", five elements each:
a[0] -> x[0], x[1], ..., x[4]
a[1] -> y[0], y[1], ..., y[4]
sum of squares of x: sum a[0][i] * a[0][i]
```

The first index selects an input variable, not an element.

## 3. Constraints

Constraints can supply penalty weight `w` and receiving objective names `objs`. Domain constraints limit values; cardinality constraints limit equality/counts; capacity/distribution constraints impose resource quotas; accumulation constraints model sequential/DAG resources; graph constraints impose graph relationships; expression constraints use cp propagation; custom constraints use callbacks.

`EMPTYVALUE` is NaN. In `addConstrainRange`, an empty left/right bound means that side is unbounded. Test this sentinel with `is_empty()`, never ordinary equality.

<a id="constraint-reference"></a>
### Constraint reference

All `addConstrainXxx` signatures end with `double w=1` and `const std::string& objs=""`; the table omits these. An empty objective list assigns no specific objective penalty. `input`/`input_var` names the input variable.

| Method | Specific parameters | Meaning |
|---|---|---|
| `addConstrainRange` | `left`, `right`; `EMPTYVALUE` means unbounded | Each element lies in the interval |
| `addConstrainEligible` | `allowed`: `vector<vector<double>>` | Per-dimension allowed value sets |
| `addConstrainCompatibility` | `value`: `double*`, `length` | Compatibility with the supplied vector |
| `addConstrainUnique` | None | All elements are distinct |
| `addConstrainDistinctCap` | `p`: `int` | At most p distinct values |
| `addConstrainMinDistance` | Scalar `gap` or per-dimension `double*` | Pairwise value separation at least gap |
| `addConstrainCapacity` | Container `caps`, count `nC`; item `vols`, count `nI` | Assigned item volumes respect capacities |
| `addConstrainDistributed` | `vals`, `size`, per-value `nums` quotas | Limit occurrences of each value |
| `addConstrainExpr` | `input_elements`, cp `formula` | Cross-variable expression with domain propagation; see [syntax](26-expressions.md) |
| `addConstrainSequenceAccumulate` | Per-step `delta`, `lower`, `upper`, `init` (0) | Sequential cumulative value remains within bounds, e.g. route load |
| `addConstrainScheduleAccumulate` | `node_count`, predecessors `pred`, durations `exec`, communication `comm`, `deadline` | Max-plus DAG scheduling and deadlines |
| `addConstrainAccumulate` | Returns `ConstrainAccumulate*` | Add multiple capacity/time/communication tracks through the returned object |
| `addConstrainGraphIndependent` | `edges`: `vector<int>` | Adjacent vertices cannot both be selected |
| `addConstrainGraphDistinct` | `edges` | Adjacent vertices take different values |
| `addConstrainGraphClique` | `edges` | Clique constraint |
| `addConstrainGraphConflict` | `edges`, `bool(*conflict)(double,double)` | Adjacent values violate when the callback reports conflict |
| `addConstrainGraphDominating` | `edges` | Dominating set |
| `addConstrainGraphVertexCover` | `edges` | Vertex cover |
| `addConstrainNodeSum` | `n_vertices`, `edges`, `directed`, bounds `lo/hi`, edge capacity `cap` | Neighborhood weight sums within bounds, with edge capacities |
| `addConstrainGraphConnectivity` | `n_vertices`, `edges`, optional `terminals={}` | Connectivity; specified terminals give Steiner connectivity |
| `addConstrain` | `ini`, `chk(dim,val)`, `chg(dim,val)` | Custom initialization, checking, and propagation |

## 4. Heuristics

Heuristics guide greedy construction and candidate ranking:

| Method | Meaning |
|---|---|
| `addInspirationRandom(v)` | Random heuristic, also the default |
| `addInspirationBoundary(v)` | Boundary heuristic |
| `addInspirationFunc(v,input,f,stable=false)` | Function pointer or functor; follow the input layout above |
| `addInspirationMatrix(v,matrix,nDec,nDim)` | Lookup-table heuristic |
| `addInspirationExpr(v,input,formula,stable=false)` | Formula heuristic |

## 5. Compiled handles

After `ProblemHandle* h = p.compile()`, interact through the handle. Metadata includes `getProblemSize()`, `getObjectNumber()`, `getVariableNumber()`, `getVariableLowbound/Upbound(dim)`, `getSolutionDecoder()`, and `getSolutionComparer()`.

```cpp
Solution s;
s.setSize(h->getProblemSize(), h->getObjectNumber());
s.setDecoder(h->getSolutionDecoder());
// Fill decision values before evaluating.
h->solutionEvaluate(s);
Comparer* c = h->getSolutionComparer();
bool better = c->isBetter(s1.fitness, s2.fitness);
double violation = h->constraintViolation(s);
bool legal = h->solutionLegality(s);
```

**Empty decisions:** `EMPTYVALUE` marks an unfinished dimension and is a valid intermediate state. Since v3.0.0.3, if any decision is empty, evaluation does not invoke user objectives or objective constraint penalties. Participating objectives receive finite worst values: large positive for minimization, large negative for maximization. Nonparticipating objectives in partial evaluation remain empty. This path neither throws nor warns nor stops optimization; valid solutions can replace it through the normal comparator. Discretization preserves empty values, and `constrainChange` does not propagate them.

**Two constraint paths:** `constraintViolation` sums raw violations of all registered constraints without weights; it is a user API, not the optimizer's evaluation path. Objective penalties multiply violations by weights and include only constraints attached to that objective. Consequently their numeric totals need not match. `solutionLegality` checks the same constraint set as `constraintViolation` but short-circuits on the first violation; legal means total raw violation is zero.

Compilation normally adds an automatic range constraint for each declared variable domain. It participates in feasibility/violation, but has weight zero and no target objective, so it contributes no fitness penalty. An explicit `addConstrainRange` takes over that variable's domain: the automatic one is omitted to prevent double counting. If the explicit range is wider, the original variable bounds are no longer enforced by that automatic constraint. Intersect ranges yourself when both should apply. `EVOFLOW_DISABLE_AUTO_DOMAIN_CONSTRAIN` in [problem.h](../../src/problem/core/problem.h) disables this mechanism globally.

For incremental construction:

```cpp
h->constrainReset();
h->setResult(s);
for (int d = 0; d < PS; ++d) {
    double* feasible = h->getFeasibleList(d);
    int count = h->getChoiceNumber(d);
    double best = h->getPrioriChoice(d);
    double score = h->getChoiceHeuristic(d, best);
    if (h->constrainCheck(d, best)) {
        s[d] = best;
        h->constrainChange(d, s[d]);
    }
    delete[] feasible;
}
```

The raw candidate array must be freed with `delete[]`. The overload `getFeasibleList(int dim, double*& buf, int& size)` supports buffer reuse and reports its actual size; do not assume the separate count equals the array length. Other helpers include `getBoundaryChoice`, `getRandomChoice`, `getRandomChoiceInspace`, `getCloseChoice`, and `choiceDiscretized`.

## 6. Problem templates

Templates expose setters and `getProblem()`, returning a caller-owned `Problem*` for compilation or optimizer use. See `tutorial/5.template.cpp`.

| Class | Header | Problem |
|---|---|---|
| `PT_TSP` | `pt-tsp.h` | Traveling salesperson |
| `PT_QAP` | `pt-qap.h` | Quadratic assignment |
| `PT_MKP` | `pt-mkp.h` | Multidimensional knapsack |
| `PT_MDS` / `PT_MIS` | `pt-mds.h` / `pt-mis.h` | Minimum dominating / maximum independent set |
| `PT_CVRP` | `pt-cvrp.h` | Capacitated vehicle routing |
| `PT_CFLP` / `PT_CPMP` | `pt-cflp.h` / `pt-cpmp.h` | Capacitated facility location / p-median |
| `PT_FJSP` | `pt-fjsp.h` | Flexible job-shop scheduling |
| `PT_WFS` | `pt-wfs.h` | Workflow scheduling |
| `PT_STP` | `pt-stp.h` | Steiner tree |
| `PT_SR` | `pt-sr.h` | Symbolic regression with GEP encoding |

### 6.1 Template construction APIs

All support optional `setName`; many graph/penalty models expose `setPenalty`. Exact `setData` overloads are in the headers.

| Template | Key setters |
|---|---|
| `PT_TSP` | `setCitys(pos,n,dim)` or `setMap(dist,n,symmetric)`; `setEdgeWeightType(EvoFlow::DistanceType::{Euclidean/Manhattan/Chebyshev/Geo}, EvoFlow::RoundType::{None/Round/Ceil/Floor})`; shared enums moved to `evoflow-math.h` in v2.1.2, Geo is great-circle distance |
| `PT_QAP` | `setFacilities(n,flow_matrix)`; `setLocations(n,distance_matrix)`; flat or nested arrays |
| `PT_MKP` | `setResourceNumber(r)`; `addBackpacks(n,capacities)`; `addItems(n,profits,occupy)`; `setItemValueSort(bool)` |
| `PT_MDS` | `setGraph(n,edges)`; `setPenalty(p)` |
| `PT_MIS` | `setGraph(n,edges)`; `setPenalty`; `setDegreeSort(bool)` |
| `PT_CVRP` | `setData(N,xs,ys,dem,cap...)`; `setVehicles(K)`; `setSplitMode({Greedy/Optimal/Linear})`; `setMaxRouteDistance(d)` where 0 is unlimited |
| `PT_CFLP` | `setData(F,M,cap,fcost,...)`; `setPenalty` |
| `PT_CPMP` | `setData(N,p,cap,dem,d)` for distances, or `setCoord(N,p,cap,dem,coord)` plus `setEdgeWeightType(dtype,rtype)` for lazy coordinate distances; `setHeuristic({GreedySeed/Composite})` |
| `PT_FJSP` | `setData(ops,M,proc,prev)`; `setSeqMode({Const/Variable})`; `setSeqHeur({MWKR/Random})` |
| `PT_WFS` | `setData(...)`; `setSeqMode({Const/Heft/HeftInsert/Variable})`; `setSeqHeur({UpwardRank/Random})`; `setMapHeur({Fastest/EFT})`; `setObjMode({MaxMakespan/AvgMakespan})` |
| `PT_STP` | `setGraph(nv,edges,w,terminals)`; `setHeuristic({Spanning/SteinerPrune})` |
| `PT_SR` | `addFunctions({Add,Sub,Mul,...})`; `setConstantPool(count,lo,hi)` or `setConstants(...)`; `setHeadLength(h)`; `setData(...)`; `ObjMode::{Regression/Heuristic}` |

WFS sequence mode controls both decisions and simulation:

- `Const` (default): mapping only, submit tasks in internal index order, append at machine tails.
- `Heft`: mapping only, standard HEFT upward-rank order, append at tails.
- `HeftInsert`: mapping only, the same HEFT order with earliest-gap insertion.
- `Variable`: mapping `x` plus ordering `s`, submit by `s`, append at tails.

Standard HEFT rank uses average processing time on available machines and average inter-machine bandwidth. It is distinct from the mapping-dependent `SeqHeur::UpwardRank`, which constructs `s` in Variable mode. `MapHeur` controls construction, not evaluation mode.

Optional WFS fields `SEQ_MODE`, `SEQ_HEUR`, `MAP_HEUR`, `OBJ_MODE`, and `PENALTY` default to `CONST`, `UPWARD_RANK`, `FASTEST`, `MAX_MAKESPAN`, and automatic penalty (effective `1e6`). Setters called after `load()` override file settings; see [WFS format](../../_pdata/wfs/README.md).

Machine mappings must be in `[0,machines-1]`; Variable priorities in `[0,tasks-1]`. Nonfinite/out-of-range values yield a finite large penalty instead of indexing machine/speed/bandwidth/capacity arrays. With storage constraints, an invalid machine also counts as a capacity violation. This safety guard does not add repair to `GenerationNoCheck`; normal WFS configurations should prefer `Generation` with discretization/repair.

### 6.2 External benchmarks

Continuous optimization does not require a built-in benchmark. Wrap external C/C++ contexts in `eccalcul_functor` and implement `copy()` with independent context ownership for each problem/parallel evaluation state. See [external benchmark example](../../tutorial/7.external-benchmark.cpp), [supporting C interface](../../tutorial/external-benchmark/), and [Python callable example](../../python/examples/external_benchmark.py).

The executable/GUI load registered file templates, not arbitrary DLL/SO libraries. Link native libraries into your own C++ host, or call installed packages from Python.

<a id="python-modeling"></a>
## 7. Python modeling

Python builds are separate from CMake. In the Windows development tree, `build-python.ps1 -PythonExe <python.exe>` places a tagged `_core.*.pyd` in `python/evoflow`. See the [Python guide](15-python.md).

### 7.1 Define, compile, and evaluate

```python
from evoflow import Problem, VariableType

p = Problem("demo")
p.add_constant("weight", [2.0, 3.0, 5.0])
p.add_variable("x", 0, 2, 1, 3, type=VariableType.ALLOCATION)
p.add_objective("cost", ["x", "weight"],
                lambda x, weight: sum(weight[int(i)] for i in x))
p.add_unique_constraint("x", objectives="cost")
p.add_random_heuristic("x")
print(p.describe().dimension)  # Constants do not occupy decision dimensions.
h = p.compile()               # Independent snapshot
solution = h.greedy_solution()
fitness = h.evaluate(solution)
print(solution, fitness, h.is_legal(solution))
print(h.is_better(fitness, [100.0]))
print(h.format_solution(solution, fitness, full=True))
```

`is_better` strictly compares two objective-length fitness sequences and returns false on ties. `format_solution` validates both decision and objective lengths; `full=True` includes names/decisions. No raw comparer, decoder, or Solution pointers are exposed.

`Problem.clear()` clears definitions only; previously compiled snapshots remain usable. Incomplete definitions, length mismatches, duplicate names, and callback failures raise exceptions. Formula objectives share native overloads; since 2.6.2.1 indexed multielement variables use their declared lengths and genuine out-of-range references fail compilation.

### 7.2 Callbacks

Each named objective input becomes an immutable tuple, in input order:

```python
p.add_objective("f", ["x", "y"], lambda x, y: sum(x) + sum(y))
```

Callable heuristics additionally receive `dimension, choice` before input tuples. The binding reacquires the GIL; callback errors cross C++ as `RuntimeError` with the objective/heuristic name. Parallel native workers may enter callbacks, but Python code remains subject to the GIL.

### 7.3 Constraints and heuristics

The 19 safe built-in constraint categories are exposed through:

- `add_range_constraint`, `add_compatibility_constraint`, `add_eligible_constraint`, `add_unique_constraint`, `add_distinct_cap_constraint`, `add_min_distance_constraint`, `add_capacity_constraint`, `add_distributed_constraint`, `add_expression_constraint`.
- `add_sequence_accumulate_constraint`, `add_schedule_accumulate_constraint`, and `add_accumulate_constraint` (a builder for capacity/time tracks).
- `add_graph_constraint` kinds `independent/distinct/clique/dominating/vertex_cover`, plus `add_node_sum_constraint` and `add_connectivity_constraint`.

Raw-C-function custom constraints and graph-conflict callbacks are not bound: global trampolines would compromise instance, concurrency, and lifetime isolation.

Heuristics support random, boundary, callable, matrix, and formula forms. Formula heuristics retain the native special array: `x[0]` is the local dimension, `x[1]` the candidate value, and `x[2...]` the first elements of additional inputs.

The 2.6.2.1 `BOUNDARY-SINGLETON-DOMAIN` fix skips interval fragments containing no valid discrete grid points after unique/other propagation. Boundary/greedy queries do not return NaN solely because such fragments exist when a valid candidate remains.

### 7.4 Construction methods

```python
h.reset_construction()
for dimension in range(h.dimension):
    choices = h.feasible_choices(dimension)
    choice = h.priori_choice(dimension)
    if h.check_choice(dimension, choice):
        h.commit_choice(dimension, choice)
```

Other methods: `choice_count`, `close_choice`, `boundary_choice`, `random_choice` (original domain), `random_choice_inspace` (propagated domain), `snap_choice`, `choice_heuristic`, and `priori_choices`. Python returns ordinary lists/tuples; the binding frees native candidate arrays.

## 8. Python templates and instance discovery

The bindings share `ProblemTemplateRegistry`, `loadProblem`, `peekProblemType`, and `App::describeProblem`; there is no separate Python format parser.

### 8.1 Query and load files

```python
from evoflow import describe_problem, load_problem, problem_type, problem_types
path = "_pdata/tsp/sample.tsp"
print(problem_types())
print(problem_type(path))
print(describe_problem(path).dimension)
problem = load_problem(path)
compiled = problem.compile()
print(compiled.dimension)
```

`load_problem(path, logical_name=None)` returns an owning `Problem` independent of a temporary template. It can be modified, described, and compiled. Missing files, unknown TYPEs, and native loading failures raise exceptions.

### 8.2 Concrete templates

Classes are `TSPTemplate`, `QAPTemplate`, `MKPTemplate`, `MDSTemplate`, `MISTemplate`, `CVRPTemplate`, `CFLPTemplate`, `CPMPTemplate`, `FJSPTemplate`, `WFSTemplate`, `STPTemplate`, and `SRTemplate`. Each has `load(path)`, `clear()`, `set_name(name)`, `save(overwrite=False)` into working-directory `_pdata/<family>`, and `to_problem()` returning an independently owned problem.

```python
from evoflow import TSPTemplate
template = TSPTemplate()
template.set_map([[0.0, 2.0, 3.0], [2.0, 0.0, 1.0], [3.0, 1.0, 0.0]])
problem = template.to_problem()
```

Corresponding setters/enums are exposed. `SRTemplate` uses safe regression-data entries instead of raw-pointer `setTarget/setFeatureMap` interfaces.

### 8.3 Discover release instances

```python
from evoflow import default_problem_data_root, discover_problem_instances
print(default_problem_data_root())
for item in discover_problem_instances(types=["TSP", "CVRP"]):
    print(item.type, item.family, item.name, item.path, item.size)
```

`ProblemInstance` contains type, family, name, absolute path, stable relative path, and size. Scans reject symlink roots, skip symlink entries and binary/helper files, and limit directories, files, and candidate read sizes. Long scans release the GIL. The default root is the nearest release `_pdata` above the package. Minimal/full data remains outside the Python package.
