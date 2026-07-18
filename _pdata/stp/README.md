# STP 数据格式说明（Steiner Tree Problem，斯坦纳树）

由 [`src/problem/template/pt-stp.h`](../../src/problem/template/pt-stp.h) 的 `PT_STP` 解析。

## 1. 问题简介

带边权无向图 `G=(V,E,w)` 与终端集 `T⊆V`，求最小权边子集使所有终端连通（可借道非终端"斯坦纳点"）。变量 `y` 长度 `|E|`、域 `{0,1}`（`y[e]=1` 选中边 `e`），目标最小化 `Σw[e]·y[e]`，连通性作纯惩罚（违反度 = 终端所在连通分量数 − 1）。模板按权升序预排序边，`getPermutation()` 还原到原边编号。

## 2. 文件位置与加载

- 路径：`_pdata/stp/<name>.stp`
- 加载：`PT_STP::load("<name>")` 读 `_pdata/stp/<name>.stp`（例：`load("sample")`）；也可传完整/相对路径。
- 注：这是 **ECFlow 自有格式**；真实 SteinLib/DIMACS `.stp`（`SECTION Graph` / `E u v w`）需另写读取器。

## 3. 文件格式

纯文本，空白分隔。由「头部键」与「数据段」组成。段的出现顺序须满足其依赖（边段在 `NODES/EDGES` 之后、终端段在 `TERMINALS` 之后）。

### 3.1 头部键（`KEY: value`）

| 键 | 含义 |
|---|---|
| `NAME:` | 实例名（任意字符串，仅记录） |
| `TYPE:` | 固定 `STP`（通用载入器按此分派） |
| `NODES:` | 顶点数 `n` |
| `EDGES:` | 边数 `m` |
| `TERMINALS:` | 终端数 `k` |

### 3.2 数据段

- `EDGE_SECTION` — 随后 `m` 行，每行一条带权无向边 `u v w`（顶点 **1 基**，`w` 为边权）。
- `TERMINAL_SECTION` — 随后 `k` 行，每行一个终端顶点编号（**1 基**）。

## 4. 样例（`sample.stp`）

```
NAME: sample
TYPE: STP
NODES: 5
EDGES: 5
EDGE_SECTION
1 2 1
2 3 1
3 4 1
4 5 1
1 5 10
TERMINALS: 3
TERMINAL_SECTION
1
3
5
```

终端 `{1,3,5}`；沿链 `1-2-3-4-5`（权和 4）连通，优于用边 `1-5`（权 10）。
