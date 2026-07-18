# CVRP 数据格式说明（Capacitated Vehicle Routing，容量约束车辆路径）

由 [`src/problem/template/pt-cvrp.h`](../../src/problem/template/pt-cvrp.h) 的 `PT_CVRP` 解析。

## 1. 问题简介

节点 `0` = 车场，`1..N` = 客户（各有需求 `demand`），车辆容量 `Q`。变量 `x` 长度 `N`、域 `{0..N-1}` = 客户访问**排列**（giant-tour）；解码时按容量把排列**切段**为若干车场闭合路，目标 = 各路总距离。`unique` 约束保证排列；不限车辆数（`K=0`）时容量恒可满足（只影响代价），`K>0` 时可能无可行划分。

## 2. 文件位置与加载

- 路径：`_pdata/cvrp/<name>.cvrp`
- 加载：`PT_CVRP::load("<name>")`（例：`load("sample")`）；也可传完整/相对路径。

## 3. 文件格式

TSPLIB/VRPLIB 风格纯文本，空白分隔。

### 3.1 头部键（`KEY: value`）

| 键 | 含义 |
|---|---|
| `NAME:` | 实例名（仅记录） |
| `TYPE:` | 固定 `CVRP`（通用载入器按此分派） |
| `DIMENSION:` | 节点数 = `N+1`（含车场 0） |
| `CAPACITY:` | 车辆容量 `Q` |
| `VEHICLES:` | （可选）车辆数上限 `K`；缺省 `0` = 不限 |

### 3.2 数据段

| 段 | 内容 |
|---|---|
| `NODE_COORD_SECTION` | `DIMENSION` 行，每行 `id x y`（`id` **1 基**，坐标用于欧氏距离；节点 `1`=车场） |
| `DEMAND_SECTION`     | `DIMENSION` 行，每行 `id demand`（车场需求 = 0） |

## 4. 样例（`sample.cvrp`）

```
NAME: sample
TYPE: CVRP
DIMENSION: 5
CAPACITY: 6
NODE_COORD_SECTION
1 0 0
2 1 0
3 2 0
4 0 1
5 0 2
DEMAND_SECTION
1 0
2 3
3 3
4 3
5 3
```

车场在原点，4 客户各需求 3，容量 6 → 每路至多 2 客户。
