# 25 · Extending Problem Modules

[English](25-extending-problems.md) | [简体中文](../手册/25-开发-扩展问题模块.md) | [Manual index](README.md)

This chapter covers constraints, templates, and objective backends. It complements [optimizer component extensions](21-components.md). First understand the four-domain [Define → compile → Manager architecture](20-architecture.md).

## 1. Add a constraint

A constraint follows `ConstrainDefine` → `compile()` → `ConstraintManager`. Each implementation derives from `Constrain`.

### 1.1 Base interface

The interface in `problem/constrain/evoflow-constrain.h` includes:

```cpp
virtual void ini() = 0;
virtual bool meet(int dim, double value) = 0;
virtual void update(int dim, double value) = 0;
virtual double violation(double* variables, int size) = 0;
virtual void regionReduction(int dim, discrete_view& region) = 0;
virtual ConstrianLevel getConstrainLevel() = 0;
virtual Constrain* clone() = 0;
virtual void addCorresConstrain(Constrain*) {}
```

`ini` resets state before solving. `meet` tests a proposed assignment; `update` commits it and propagates state. Together they support incremental construction. `violation` scores a full solution for penalties. `regionReduction` narrows a dimension's feasible domain through `discrete_view`. `getConstrainLevel` selects the applicable runtime path. `clone` must make an independent copy for parallel execution/handle copying. Related-constraint registration is optional.

### 1.2 Implementation steps

1. Implement `ConstrainXxx` in `problem/constrain/`. Reference `evoflow-domain.h`, `evoflow-cardinality.h`, `evoflow-graph-constrain.h`, `evoflow-constrain.h`, or `evoflow-accumulate.h`.
2. Add a `ConstrainKind` value in `problem/define/constrain-define.h`.
3. Add description storage and `addConstrainXxx()`. A `Spec` records `kind`, `related_variables`, `related_objects` for penalty objectives, packed `params` (`vector<vector<double>>`), and any graph edges or function-pointer addresses.
4. Add a case to `ConstrainDefine::compile(varMgr, objMgr)` that builds the object and transfers it to `ConstraintManager`. Validate legality/parameters at compilation, not when storing the description.
5. Expose a delegating `Problem::addConstrainXxx(...)` in `problem/core/problem.h`.
6. Update the [constraint reference](10-problems.md#constraint-reference) and add a smoke test for `meet`, `violation`, and `regionReduction`.

Packed `Spec.params` uses separate numeric vectors, with conventions such as `buf[0]` holding a length. Function pointers are stored in `addresses` as `long long` and cast back during compilation. Follow the existing representation consistently.

## 2. Add a problem template

`PT_Xxx` is a convenience class that assembles a concrete problem through public problem APIs. Existing `problem/template/pt-*.h` files are examples.

```cpp
class PT_Xxx {
public:
    void setName(std::string);
    void setData(...);
    void setPenalty(double);
    Problem* getProblem() {
        Problem* p = new Problem(_name);
        p->addVariable(...);
        p->addObjective(...);
        // Add appropriate constraints and optional construction heuristics.
        return p; // Caller owns the returned object.
    }
};
```

This is an implementation outline, not a complete compilable class. Encoding determines suitable operators: permutations/sequences for TSP/QAP/scheduling suit ACO or sequence GA; continuous models suit PSO/ES; binary selections such as MKP/MDS/MIS suit GA or binary PSO.

Follow the [objective input layout](10-problems.md#objective-input-layout) for `double**`. Stateful objectives can use `eccalcul_functor`, for example a TSP distance matrix. A directly used convenience template does not need registry registration: include its header and construct it. See `tutorial/5.template.cpp`.

## 3. Add an objective or heuristic backend

Most objectives need no framework change. `addObjective` accepts a `double(*)(double**)` function pointer, a stateful `eccalcul_functor*`, or a muParserX formula. Heuristic entries include `addInspirationFunc/Matrix/Expr/Random/Boundary`; see [problem modeling](10-problems.md).

A completely new evaluation backend, such as another expression engine or `CalcKind`, changes `problem/define/calc-spec.h` and its calculator implementation. Discuss the design before making that deeper change. See [expression syntax](26-expressions.md) for the distinction between cp constraints and muParserX objectives/heuristics.

## 4. Extension comparison

| | Problem side | Optimizer side |
|---|---|---|
| New constraint/operator | `Constrain` subclass, Kind, Spec, compile case, Problem API | Component class, `EVOFLOW_REGISTER`, aggregate header, optional fluent setter |
| Assembly | Define → compile → Manager, dispatch by kind | Self-registering `Registry<Base>`, create by tag |
| Convenience layer | `PT_Xxx` assembles `add*` calls | Algorithm recipes combine components |
