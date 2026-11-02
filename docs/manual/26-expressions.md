# 26 · Expression Syntax

[English](26-expressions.md) | [简体中文](../手册/26-开发-表达式公式语法.md) | [Manual index](README.md)

String formulas use two different engines. Their syntax and capabilities are not interchangeable.

| Use | API | Engine | Main distinction |
|---|---|---|---|
| Constraints | `addConstrainExpr(var, elements, formula, …)` | cp interval propagation | Arithmetic/relations, no mathematical functions; supports domain reduction |
| Objectives / heuristics | `addObjective(…, formula)` / `addInspirationExpr(…, formula)` | muParserX | Arithmetic and mathematical functions; direct evaluation without propagation |

Both reference elements as `variable[index]`, for example `x[0]`. The variable name comes from `input_elements`; the subscript selects an element of that variable.

## 1. Constraint expressions: cp

```cpp
p.addConstrainExpr("x", "x", "x[0]+x[1]-10", 1, "g");
// x[0] + x[1] <= 10
```

The formula `F` is an arithmetic violation expression. The constraint is **`F <= 0`**, and the violation is **`max(0, F)`**.

Supported by `cp/compile/parser.h`:

- Arithmetic: `+`, `-`, `*`, `/`, unary minus.
- Relations: `<`, `<=`, `>`, `>=`, `==`, `!=`.
- Operands: `variable[constant-index]`, numbers including scientific notation such as `1e-3`, and parentheses.
- No mathematical functions: `sin`, `cos`, `sqrt`, `abs`, `pow`, etc. are unavailable. Use a custom constraint or arithmetic reformulation.

Relational subexpressions evaluate to `0` or `1`, so indicator arithmetic can encode logic:

| Logic | Violation expression |
|---|---|
| Implication a → b | `(a)-(b)` |
| At most k conditions hold | `(c1)+(c2)+…+(cn)-k` |
| AND/OR combinations | Combine indicator sums/differences; see the table in `evoflow-constrain.h` |

Domain reduction for relations is conservative: it may retain an extra boundary value rather than over-prune; penalties still detect violations. cp propagates expressions backwards to narrow domains through `regionReduction`, supporting `getFeasibleList` and `getPrioriChoice` during dimension-by-dimension construction.

`input_elements` may include multiple variables, for example `"x,y"`; follow their expanded element order. Invalid cp formulas raise `std::invalid_argument` during construction/compilation. Definitions store specifications; compilation validates them.

## 2. Objective and heuristic expressions: muParserX

```cpp
p.addObjective("f", 1, true, "x", std::string("x[0]*x[0] + x[1]*x[1]"));
p.addInspirationExpr("x", "", "x[1]");
```

`calculator/`, `exprTree/`, and `TreeCalculator` use muParserX. Supported expressions include arithmetic `+ - * / ^`, parentheses, indexed elements, constants, and standard functions such as `sin/cos/tan/exp/log/log10/sqrt/abs/pow/min/max`, subject to the configured muParserX function set.

These directly evaluate a supplied solution, without domain propagation. Objective `min_is_better` determines direction; `priority` determines comparison order. Invalid formulas raise `std::invalid_argument`; muParserX may also print `Compile Error…` to stderr.

Function pointers and stateful `eccalcul_functor` objects are alternatives for arbitrary C++ logic; formulas are a convenience, not a requirement.

## 3. Quick comparison

| Feature | Constraint cp | Objective/heuristic muParserX |
|---|---|---|
| `x[i]` element references | Yes | Yes |
| Arithmetic `+ - * /` | Yes | Yes, also `^` |
| Relations | Yes, as 0/1 indicators | Comparisons exist, though not normally needed for objectives |
| Mathematical functions | No | Yes |
| Meaning | `F <= 0`, violation `max(0,F)` | Direct value |
| Domain propagation | Yes | No |
| Non-formula alternative | Custom `addConstrain(ini,chk,chg)` | Function pointer / functor |

## 4. Common mistakes

- `sqrt(x[0])` in a constraint fails because cp has no function support. Use a custom constraint, an arithmetic reformulation, or muParserX where an objective is intended.
- Subscripts must be constant integers: `x[0]` is valid; a variable subscript `x[i]` is not.
- To express `a <= b`, pass `a-b`, not the whole relation `a<=b`. Although cp can parse a relation at the top level, the EvoFlow API expects an arithmetic violation formula `F`.
