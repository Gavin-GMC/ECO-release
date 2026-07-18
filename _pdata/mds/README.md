# MDS 数据格式说明（Minimum Dominating Set，最小支配集）

由 [`src/problem/template/pt-mds.h`](../../src/problem/template/pt-mds.h) 的 `PT_MDS` 解析。

## 1. 问题简介

无向图 `G=(V,E)`，求最小的顶点子集 `D⊆V`，使每个顶点要么在 `D` 中、要么与 `D` 中某顶点相邻（被支配）。变量 `x` 长度 `|V|`、域 `{0,1}`（`x[v]=1` 选入 `D`），目标最小化 `Σx[v]`，未被支配的顶点计入违反度惩罚。

## 2. 文件位置与加载

- 路径：`_pdata/mds/<name>.mds`
- 加载：`PT_MDS::load("<name>")` 读 `_pdata/mds/<name>.mds`（例：`load("sample")`）；也可传完整/相对路径。

## 3. 文件格式

纯文本，空白（空格/换行）分隔；行列排布仅为可读性。由「头部键」与「数据段」组成。

### 3.1 头部键（`KEY: value`）

| 键 | 含义 |
|---|---|
| `NAME:` | 实例名（任意字符串，仅记录） |
| `TYPE:` | 固定 `MDS`（通用载入器按此分派） |
| `VERTICES:` | 顶点数 `n` |
| `EDGES:` | 边数 `m` |

### 3.2 数据段

- `EDGE_SECTION` — 随后 `m` 行，每行一条无向边 `u v`（顶点 **1 基**，即 `1..n`）。加载时校验端点不越界。

## 4. 样例（`sample.mds`）

```
NAME: sample
TYPE: MDS
VERTICES: 5
EDGES: 4
EDGE_SECTION
1 2
2 3
3 4
4 5
```

一条 5 顶点的链 `1-2-3-4-5`；最小支配集如 `{2,4}`（大小 2）。
