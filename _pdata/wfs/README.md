# WFS 数据格式说明（Computational Workflow Scheduling）

计算工作流调度问题的实例数据格式。由 [`src/problem/template/pt-wfs.h`](../../src/problem/template/pt-wfs.h) 的 `PT_WFS` 解析。

## 1. 问题简介

工作流 = **任务 DAG**（有向无环图，边 = 数据依赖）。把任务映射到**异构机器**并定序，最小化 **makespan**（完工时间）。参考 HEFT（Topcuoglu et al. 2002, *Heterogeneous Earliest-Finish-Time*）。

- **计算**：任务 `i` 在机器 `m` 的计算时间 `w[i][m]`，异构（不同机器耗时不同）。
- **通信**：数据依赖 `i → j` 传输数据量 `data(i,j)`，耗时 `= data / speed[机器(i)][机器(j)]`；同机 `= 0`。
- **目标**：各工作流完成时间的 **最大**（MaxMakespan，默认）或 **平均**（AvgMakespan）。
  一个"工作流" = DAG 的**弱连通分量**（自动识别）。

## 2. 文件位置与加载

- 路径：`_pdata/wfs/<name>.wfs`
- 加载：`PT_WFS::load("<name>")` → 读取 `_pdata/wfs/<name>.wfs`（例：`load("sample")`）。

## 3. 文件格式

纯文本，**空白（空格/换行）分隔**，行列排布仅为可读性、不影响解析。由「头部键」和「数据段」组成。

### 3.1 头部键（`KEY: value`）

| 键 | 含义 |
|---|---|
| `NAME:` | 实例名（任意字符串，仅记录） |
| `TYPE:` | 固定 `WFS`（标识用，解析不强制） |
| `NODES:` | 任务数 `n` |
| `MACHINES:` | 机器数 `M` |
| `EDGES:` | 边数 `E` |

**行号规范（v1.4.9）**：矩阵/向量段每逻辑行以 **1-based 索引**开头（读入即忽略）——矩阵每行 `idx v0 … v_{cols-1}`，向量每元素一行 `idx value`。**`EDGE_SECTION` 是边表、不加索引**（每行仍为 `u v d`）。

### 3.2 必需数据段

| 段 | 规模 | 含义 |
|---|---|---|
| `COMP_SECTION` | `n` 行，每行 `i w[i][0] … w[i][M-1]` | 计算时间 `w[i][m]`：**行 = 任务 `i`（行首索引），列 = 机器 `m`**。须在 `NODES`/`MACHINES` 之后。 |
| `EDGE_SECTION` | `E` 行，每行 `u v d`（**无行首索引**） | 数据依赖边：任务 `u → v`（**1 基编号**），数据量 `d`。端点须 ∈ `[1, n]`。须在 `EDGES`/`NODES` 之后。 |
| `SPEED_SECTION` | `M` 行，每行 `a speed[a][0] … speed[a][M-1]` | 机器间带宽 `speed[a][b]` = 机器 `a → b` 的速度。**对角线（`a==b`）为占位**（同机通信恒 0，不使用）。须在 `MACHINES` 之后。 |

### 3.3 可选数据段（缺省则不启用对应特性）

| 段 | 规模 | 含义 |
|---|---|---|
| `DEADLINE_SECTION` | `n` 行，每行 `i deadline[i]` | 任务截止期。启用后为**软惩罚**：`目标 += penalty · Σ max(0, finish[i] − deadline[i])`。 |
| `MEM_SECTION` | `n` 行，每行 `i mem[i]` | 任务内存需求。 |
| `MEMCAP_SECTION` | `M` 行，每行 `m memCap[m]` | 机器内存容量。**与 `MEM_SECTION` 配合** → 机器适用性硬约束（`mem[i] ≤ memCap[m]` 才可映射）。 |
| `STORCAP_SECTION` | `M` 行，每行 `m storCap[m]` | 机器存储容量。任务出边数据总量 `≤ storCap[m]` → 硬约束（容量）。 |

## 4. 示例 `sample.wfs` 逐行解读

```text
NAME: sample          # 实例名
TYPE: WFS             # 类型标识
NODES: 4              # 4 个任务(0..3,文件内边用 1..4)
MACHINES: 2           # 2 台机器(0,1)
EDGES: 4              # 4 条依赖边
COMP_SECTION          # 计算时间 w[任务][机器],每行"任务号 + M 值"
1 1 3                 #   任务1: 机器0=1, 机器1=3
2 2 4                 #   任务2: 机器0=2, 机器1=4
3 2 4                 #   任务3: 机器0=2, 机器1=4
4 1 3                 #   任务4: 机器0=1, 机器1=3
EDGE_SECTION          # 边 "u v data"(1 基,无行首索引)
1 2 2                 #   任务1 → 任务2, 数据量 2
1 3 2                 #   任务1 → 任务3, 数据量 2
2 4 2                 #   任务2 → 任务4, 数据量 2
3 4 2                 #   任务3 → 任务4, 数据量 2
SPEED_SECTION         # 机器间带宽 speed[a][b],每行"机器号 + M 值"
1 1 1                 #   机器0→0=1(占位), 机器0→1=1
2 1 1                 #   机器1→0=1,       机器1→1=1(占位)
```

即：4 个任务构成菱形 DAG（`1→{2,3}→4`），2 台机器、带宽均为 1。

## 5. 解析规则与约束

- **按行读取（带行首索引）**：矩阵/向量段每逻辑行以 1-based 索引开头（读入即忽略），随后是该行的值；`EDGE_SECTION` 无索引、每行 `u v d`。
- **段顺序依赖**：依赖的头部键须先出现——`COMP_SECTION` 需 `NODES`+`MACHINES`；`EDGE_SECTION` 需 `EDGES`+`NODES`；`SPEED_SECTION`/`MEMCAP_SECTION`/`STORCAP_SECTION` 需 `MACHINES`；`MEM_SECTION`/`DEADLINE_SECTION` 需 `NODES`。违反 → 报错。
- **边编号 1 基**：`EDGE_SECTION` 用 `1..n`，解析时转 0 基；端点越界 → 报错。
- **任务号应为拓扑序**（每条边 `u < v`）。若不满足，模板**自动按拓扑序内部重排** + 输出日志警告，`getPermutation()` 提供「新序 → 原任务号」映射（makespan 不受重排影响）。**含环**（非 DAG）→ 报错，`getProblem()` 返回 `nullptr`。
- **未知 `KEY:` 行**被跳过（读到行尾）。段被截断 / 缺 `NODES`/`MACHINES` → 报错。

## 6. 数据如何被使用（便于理解各字段用途）

- **决策**：映射 `x[i] ∈ {0..M-1}`（任务 `i` 分到哪台机器）+（可选）排序优先级 `s[i]`。
- **调度解码**：DAG 主动调度——在「就绪」任务（前驱全部已排）中按优先级取一个，排到机器 `x[i]`：
  `start = max( 机器可用时刻, max_{前驱 k}( finish[k] + comm(k,i) ) )`，`finish = start + w[i][x[i]]`。
- 因此：`COMP_SECTION` 决定计算耗时、`EDGE_SECTION` 的 `data` 与 `SPEED_SECTION` 共同决定通信耗时、DAG 结构决定就绪次序与工作流划分。

## 7. 最小可用实例（复制即用）

```text
NAME: tiny
TYPE: WFS
NODES: 3
MACHINES: 2
EDGES: 2
COMP_SECTION
1 2 3
2 2 2
3 3 1
EDGE_SECTION
1 2 1
1 3 1
SPEED_SECTION
1 1 1
2 1 1
```

> 需要截止期/资源时，追加 `DEADLINE_SECTION` / `MEM_SECTION`+`MEMCAP_SECTION` / `STORCAP_SECTION` 即可（见 §3.3）。
