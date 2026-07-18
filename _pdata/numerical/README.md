# NUMERICAL 数据格式说明（连续域数值优化基准）

由 [`src/problem/template/pt-numerical.h`](../../src/problem/template/pt-numerical.h) 的 `PT_Numerical` 解析。

## 1. 问题简介

连续域数值优化，包装 CEC2013 两套竞赛基准：

- **CEC2013_N**（Niching Competition）：`func_id` 1–20；每个函数的维度/定义域由基准固定。
- **CEC2013_L**（Large-Scale Competition）：`func_id` 1–15；均为 1000 维。

基准的**系数/数据由 CEC 代码内置**（本目录的 `CEC2013_N/`、`CEC2013_L/` 子目录），**不需外部实例数据**。因此一个数值"用例"仅由 `(基准族, func_id)` 唯一确定。

## 2. 描述文件（`.num`）

为让数值问题也能像其它家族一样"喂文件"（经通用载入器 `loadProblem` / `main.exe`），提供一个**仅含头部说明段**的描述文件：

- 路径：`_pdata/numerical/<name>.num`
- 加载：`PT_Numerical::load("<name>")`（例：`load("cec2013n-f3")`）；也可传完整/相对路径。
- 存盘：`PT_Numerical::save(bool overwrite)` 回写本描述文件。
- 代码内直接构造仍可用两参重载：`PT_Numerical::load(Benmarks, func_id)`。

### 头部键（`KEY: value`）

| 键 | 含义 |
|---|---|
| `NAME:` | 问题名（构造出的 `Problem` 用此命名） |
| `TYPE:` | 固定 `NUMERICAL`（通用载入器按此分派） |
| `BENCHMARK:` | `CEC2013_N` 或 `CEC2013_L`（选基准族） |
| `FUNC_ID:` | 函数编号（N：1–20；L：1–15） |

维度与定义域由 `(BENCHMARK, FUNC_ID)` 在模板内部固定，**描述文件不需指定**。

## 3. 样例（`cec2013n-f3.num`）

```
NAME: cec2013n-f3
TYPE: NUMERICAL
BENCHMARK: CEC2013_N
FUNC_ID: 3
```

CEC2013_N 的 f3 为 1 维；跑法：`main.exe _pdata/numerical/cec2013n-f3.num config/cmaes.cfg`。

## 4. 注

- 数值基准需连续域优化器（CMA-ES / ES 等），不适用 ACO 等排列/构造式算子。
- CEC 竞赛源码定义了 `INF`/`E`/`PI` 等全局宏；`pt-numerical.h` 在 include 边界已 `#undef` 抑制其外泄。
