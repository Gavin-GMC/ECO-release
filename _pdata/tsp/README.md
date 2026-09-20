# TSP 数据格式说明（Travelling Salesman Problem，旅行商）

由 [`src/problem/template/pt-tsp.h`](../../src/problem/template/pt-tsp.h) 的 `PT_TSP` 解析（**TSPLIB** 格式子集）。

## 1. 问题简介

给定城市集与两两距离，求访问每城恰一次并回到起点的最短巡回。变量 `x` 长度 = 城市数、域 `{0..n-1}` = 访问**排列**；`unique` 约束保证排列；目标 = 巡回总长度。对称图用 `sequence_bidiagraph`、有向图用 `sequence_direction`。

## 2. 文件位置与加载

- 路径：`_pdata/tsp/<name>.tsp`
- 加载：`PT_TSP::load("<name>")`（例：`load("sample")`）；也可传完整/相对路径。

## 3. 文件格式

TSPLIB 纯文本，空白分隔。头部为 `KEY: value`，随后是坐标/距离数据段。

### 3.1 头部键

| 键 | 含义 |
|---|---|
| `NAME:` | 实例名 |
| `TYPE:` | `TSP`（对称）或 `ATSP`（非对称）——通用载入器按此分派 |
| `COMMENT:` | 注释（整行跳过） |
| `DIMENSION:` | 城市数 `n` |
| `EDGE_WEIGHT_TYPE:` | 距离类型：`EUC_2D`/`EUC_3D`（欧氏+四舍五入）、`MAN_2D`（曼哈顿）、`MAX_2D`（切比雪夫）、`CEIL_2D`（欧氏+向上取整）、`GEO`（**地理坐标真大圆**：经纬度→弧度+球面余弦，TSPLIB 内建取整；v2.1.2 修正——此前误当欧氏，[[T-DIST-TYPE]]）、`ATT`、`EXPLICIT` … |
| `EDGE_WEIGHT_FORMAT:` | `EXPLICIT` 时的矩阵排布：`FULL_MATRIX`/`UPPER_ROW`/`LOWER_DIAG_ROW` … |

### 3.2 数据段

- 坐标类（`EUC_*`/`GEO`/`ATT` …）：`NODE_COORD_SECTION` 后 `n` 行 `id x y[ z]`（**1 基**），距离由坐标按类型算得。
- 显式矩阵（`EXPLICIT`）：`EDGE_WEIGHT_SECTION` 后按 `EDGE_WEIGHT_FORMAT` 排布的距离数。

## 4. 样例

首方最小样例：

```
NAME: sample
TYPE: TSP
DIMENSION: 3
EDGE_WEIGHT_TYPE: EUC_2D
NODE_COORD_SECTION
1 0 0
2 3 0
3 0 4
EOF
```

公开包只提供上述 EvoFlow 首方样例，不重新分发 TSPLIB 实例或参考 tour。用户可从 TSPLIB 发布方取得数据并按 [`../../tutorial/external-data/README.md`](../../tutorial/external-data/README.md) 解压、校验和接入。
