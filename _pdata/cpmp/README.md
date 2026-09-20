# CPMP 数据格式说明（Capacitated p-Median Problem，容量约束 p-中位）

由 [`src/problem/template/pt-cpmp.h`](../../src/problem/template/pt-cpmp.h) 的 `PT_CPMP` 解析。

## 1. 问题简介

`N` 个节点，每个既是潜在中位（容量 `cap`）又是需求点（需求 `dem`），距离 `d[i][j]`。选恰好 `p` 个中位，每节点分到一个中位（**单源**），最小化总分配距离，每中位簇内总需求（含中位自身）≤ 其容量。变量 `x` 长度 `N`、域 `{0..N-1}` = 每节点的中位。

## 2. 文件位置与加载

- 路径：`_pdata/cpmp/<name>.cpmp`
- 加载：`PT_CPMP::load("<name>")`（例：`load("sample")`）；也可传完整/相对路径。

## 3. 文件格式

纯文本，空白分隔。段在 `NODES/MEDIANS` 计数之后。

### 3.1 头部键（`KEY: value`）

| 键 | 含义 |
|---|---|
| `NAME:` | 实例名（仅记录） |
| `TYPE:` | 固定 `CPMP`（通用载入器按此分派） |
| `NODES:` | 节点数 `N` |
| `MEDIANS:` | 中位数 `p` |
| `EDGE_WEIGHT_TYPE:` | **可选**，指定几何段类型（仿 pt-tsp）。缺省 `EXPLICIT`。 |

**`EDGE_WEIGHT_TYPE:` 取值**（决定用哪种几何段 + 坐标式的距离公式/取整）：

| 取值 | 几何段 | 距离 |
|---|---|---|
| `EXPLICIT`（缺省） | `DISTANCE_SECTION` | 显式距离矩阵 |
| `EUC_2D`  | `NODE_COORD_SECTION` | 欧氏，四舍五入(Round) |
| `CEIL_2D` | `NODE_COORD_SECTION` | 欧氏，向上取整(Ceil) |
| `FLOOR_2D`| `NODE_COORD_SECTION` | 欧氏，向下取整(Floor)——**FrontierCO CPMP 用**（对齐其 `eval_func` 的 `math.floor(d)`） |
| `MAN_2D`  | `NODE_COORD_SECTION` | 曼哈顿，Round |

### 3.2 数据段

**行号规范**：每逻辑行以 **1-based 索引**开头（读入即忽略）——向量段每元素一行 `idx value`，矩阵段每行 `idx v0 … v_{N-1}`，坐标段每行 `idx x y`。

| 段 | 内容 |
|---|---|
| `CAPACITY_SECTION` | `N` 行，每行 `j cap[j]`（各节点作中位时的容量） |
| `DEMAND_SECTION`   | `N` 行，每行 `i dem[i]`（各节点需求） |
| `DISTANCE_SECTION` | 【`EXPLICIT` 时】`N` 行，每行 `i d[i][0] … d[i][N-1]`（距离矩阵行） |
| `NODE_COORD_SECTION` | 【坐标式时】`N` 行，每行 `i x[i] y[i]`（节点坐标）；矩阵由 `EDGE_WEIGHT_TYPE` 指定公式在 `getProblem` 惰性生成，**对角=0**（节点作自身中位距离 0） |

`DISTANCE_SECTION` 与 `NODE_COORD_SECTION` **二选一**（由 `EDGE_WEIGHT_TYPE` 决定）。坐标式避免大规模实例的 `N×N` 矩阵膨胀（`_pdata` 仅存 `~N` 行）。

## 4. 样例

### 4.1 显式矩阵（`sample.cpmp`）

```
NAME: sample
TYPE: CPMP
NODES: 4
MEDIANS: 2
CAPACITY_SECTION
1 10
2 10
3 10
4 10
DEMAND_SECTION
1 1
2 1
3 1
4 1
DISTANCE_SECTION
1 0 1 2 3
2 1 0 1 2
3 2 1 0 1
4 3 2 1 0
```

4 节点选 2 中位；距离为一维链上的间距。

### 4.2 坐标式（`sample-coord.cpmp`）

```
NAME: sample-coord
TYPE: CPMP
NODES: 4
MEDIANS: 2
EDGE_WEIGHT_TYPE: FLOOR_2D
NODE_COORD_SECTION
1 0 0
2 1 0
3 2 0
4 3 0
CAPACITY_SECTION
1 10
2 10
3 10
4 10
DEMAND_SECTION
1 1
2 1
3 1
4 1
```

同一 4 节点线性链的坐标写法；`FLOOR_2D` 下 `floor(欧氏)` 恰得与 4.1 相同的整数间距矩阵。
