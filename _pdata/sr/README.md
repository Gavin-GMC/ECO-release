# SR 数据格式说明（Symbolic Regression，符号回归）

由 [`src/problem/template/pt-sr.h`](../../src/problem/template/pt-sr.h) 的 `PT_SR` 解析。

## 1. 问题简介

给定若干 `(x, y)` 数据点（`x` 为 `d` 维输入、`y` 为标量目标），进化出一个数学表达式 `f(x)` 拟合 `y`，目标最小化 RMSE（可选加简约项）。模板用 GEP 定长基因（头长 `h` + 尾长 `t`）把变长表达式树装进定长维度，经 Karva 解码为合法树。

## 2. 文件位置与加载

- 路径：`_pdata/sr/<name>.sr`
- 加载：`PT_SR::load("<name>")`（例：`load("sr_sample")`）；也可传完整/相对路径。
- 本文件仅用于**回归模式**（`Regression`）。超启发式模式（`Heuristic`）经 `setTarget` 注入，不读本文件。

## 3. 文件格式

纯文本，空白分隔。头部键 + 一个数据段。

### 3.1 头部键（`KEY: value`）

| 键 | 含义 |
|---|---|
| `NAME:` | 实例名（仅记录） |
| `TYPE:` | 固定 `SR`（通用载入器按此分派） |
| `VARIABLES:` | 输入维度 `d` |
| `POINTS:` | 数据点数 `np` |
| `HEAD:` | （可选）GEP 头长 `h`；显式 `setHeadLength` 优先 |

### 3.2 数据段

- `DATA_SECTION` — 随后 `np` 行，每行 `d+1` 个数：前 `d` 个为输入 `x0 … x_{d-1}`，最后一个为目标 `y`。

## 4. 样例（`sample.sr`）

```
NAME: sr_sample
TYPE: SR
VARIABLES: 2
POINTS: 5
HEAD: 3
DATA_SECTION
1 2 3
3 4 7
0 5 5
2 2 4
-1 3 2
```

2 维输入、5 个点；此例 `y = x0 + x1`。
