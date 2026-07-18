# MIS 数据格式说明（Maximum Independent Set，最大独立集）

由 [`src/problem/template/pt-mis.h`](../../src/problem/template/pt-mis.h) 的 `PT_MIS` 解析。

## 1. 问题简介

无向图 `G=(V,E)`，求最大的顶点子集 `S⊆V`，使 `S` 内任意两顶点**不相邻**。变量 `x` 长度 `|V|`、域 `{0,1}`（`x[v]=1` 选入 `S`），目标最大化 `Σx[v]`，相邻却同选的顶点对计入违反度惩罚。模板按度升序预排序顶点，`getPermutation()` 还原到原顶点编号。

## 2. 文件位置与加载

- 路径：`_pdata/mis/<name>.mis`
- 加载：`PT_MIS::load("<name>")` 读 `_pdata/mis/<name>.mis`（例：`load("sample")`）；也可传完整/相对路径。

## 3. 文件格式

TSPLIB 风格纯文本，空白分隔。由「头部键」与「数据段」组成。

### 3.1 头部键（`KEY: value`）

| 键 | 含义 |
|---|---|
| `NAME:` | 实例名（任意字符串，仅记录） |
| `TYPE:` | 固定 `MIS`（通用载入器按此分派） |
| `VERTICES:` | 顶点数 `n` |
| `EDGES:` | 边数 `m` |

### 3.2 数据段

- `EDGE_SECTION` — 随后 `m` 行，每行一条无向边 `u v`（顶点 **1 基**）。

## 4. 样例（`sample.mis`）

```
NAME: sample
TYPE: MIS
VERTICES: 5
EDGES: 4
EDGE_SECTION
1 2
2 3
3 4
4 5
```

链 `1-2-3-4-5`；最大独立集如 `{1,3,5}`（大小 3）。
