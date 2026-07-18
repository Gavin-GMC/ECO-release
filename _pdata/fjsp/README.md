# FJSP 数据格式说明（Flexible Job-Shop Scheduling，柔性作业车间调度）

由 [`src/problem/template/pt-fjsp.h`](../../src/problem/template/pt-fjsp.h) 的 `PT_FJSP` 解析。

## 1. 问题简介

若干作业，每作业含有序工序链；每工序可在**一组可用机器**中择一加工（机器不同耗时不同）。求"每工序选哪台机器 + 全局排序"，最小化 makespan（完工时间）。变量 `x[ops]∈{0..M-1}` = 每工序的机器（机器适用性为硬约束）；可选排序变量。目标经主动调度解码器求 makespan。

## 2. 文件位置与加载

- 路径：`_pdata/fjsp/<name>.fjsp`
- 加载：`PT_FJSP::load("<name>")`（例：`load("sample")`）；也可传完整/相对路径。

## 3. 文件格式

ECFlow + Brandimarte 风格纯文本，空白分隔。

### 3.1 头部键（`KEY: value`）

| 键 | 含义 |
|---|---|
| `NAME:` | 实例名（仅记录） |
| `TYPE:` | 固定 `FJSP`（通用载入器按此分派） |
| `JOBS:` | 作业数 |
| `MACHINES:` | 机器数 `M` |

### 3.2 数据段

- `JOB_SECTION` — 每作业一行，格式（Brandimarte）：

  ```
  nOps  [ nAlt (机器 时间) (机器 时间) … ]×nOps
  ```

  即：先给该作业的工序数 `nOps`；随后对每个工序，先给可选机器数 `nAlt`，再跟 `nAlt` 对 `(机器号 加工时间)`。**机器号 1 基**（`1..M`）。加载校验段顺序、读满、机器号范围。

## 4. 样例（`sample.fjsp`）

```
NAME: sample
TYPE: FJSP
JOBS: 2
MACHINES: 2
JOB_SECTION
2 2 1 3 2 2 2 1 2 2 4
1 2 1 2 2 3
```

- 作业 1：`2` 工序。工序 1 有 `2` 选择：`(机器1,时间3)`、`(机器2,时间2)`；工序 2 有 `2` 选择：`(机器1,时间2)`、`(机器2,时间4)`。
- 作业 2：`1` 工序，有 `2` 选择：`(机器1,时间2)`、`(机器2,时间3)`。
