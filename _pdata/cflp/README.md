# CFLP 数据格式说明（Capacitated Facility Location，容量约束设施选址）

由 [`src/problem/template/pt-cflp.h`](../../src/problem/template/pt-cflp.h) 的 `PT_CFLP` 解析。

## 1. 问题简介

`F` 个设施（各有开设成本 `fcost`、容量 `cap`）、`M` 个客户（各有需求 `dem`），分配成本 `c[i][j]`（客户 `i` 用设施 `j`）。**单源**变体（Avella-Boccia Test Bed 1/B）：每客户恰分到一个设施。变量 `x` 长度 `M`、域 `{0..F-1}`；目标 = Σ 分配成本 + Σ 已开设施的开设成本；容量为**硬约束**。

## 2. 文件位置与加载

- 路径：`_pdata/cflp/<name>.cflp`
- 加载：`PT_CFLP::load("<name>")`（例：`load("sample")`）；也可传完整/相对路径。

## 3. 文件格式

纯文本，空白分隔。段的出现须在 `FACILITIES/CUSTOMERS` 计数之后（加载做计数校验）。

### 3.1 头部键（`KEY: value`）

| 键 | 含义 |
|---|---|
| `NAME:` | 实例名（仅记录） |
| `TYPE:` | 固定 `CFLP`（通用载入器按此分派） |
| `FACILITIES:` | 设施数 `F` |
| `CUSTOMERS:` | 客户数 `M` |

### 3.2 数据段

**行号规范**：每逻辑行以 **1-based 索引**开头（读入即忽略）——向量段每元素一行 `idx value`，矩阵段每行 `idx v0 … v_{cols-1}`。

| 段 | 内容 |
|---|---|
| `CAPACITY_SECTION`  | `F` 行，每行 `j cap[j]`（各设施容量） |
| `OPENCOST_SECTION`  | `F` 行，每行 `j fcost[j]`（各设施开设成本） |
| `DEMAND_SECTION`    | `M` 行，每行 `i dem[i]`（各客户需求） |
| `COST_SECTION`      | `M` 行，每行 `i c[i][0] … c[i][F-1]`（客户 `i` 到各设施的分配成本） |

## 4. 样例（`sample.cflp`）

```
NAME: sample
TYPE: CFLP
FACILITIES: 2
CUSTOMERS: 3
CAPACITY_SECTION
1 5
2 5
OPENCOST_SECTION
1 10
2 10
DEMAND_SECTION
1 2
2 2
3 2
COST_SECTION
1 1 9
2 1 9
3 9 1
```

2 设施 / 3 客户；客户 0、1 近设施 0，客户 2 近设施 1。
