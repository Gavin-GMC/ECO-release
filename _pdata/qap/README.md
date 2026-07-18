# QAP 数据格式说明（Quadratic Assignment Problem，二次分配）

由 [`src/problem/template/pt-qap.h`](../../src/problem/template/pt-qap.h) 的 `PT_QAP` 解析。

## 1. 问题简介

`n` 个设施分配到 `n` 个位置（一一对应）。设施间有流量 `flow[i][j]`，位置间有距离 `dist[k][l]`。求分配排列 `π`，最小化 `Σ_{i,j} flow[i][j]·dist[π(i)][π(j)]`。变量 `x` 长度 `n`、域 `{0..n-1}` = 设施→位置的**排列**；`unique` 约束保证排列。

## 2. 文件位置与加载

- 路径：`_pdata/qap/<name>.qap`
- 加载：`PT_QAP::load("<name>")`（例：`load("bur26a")`）；也可传完整/相对路径。

## 3. 文件格式

纯文本，空白分隔。头部键 + 两个矩阵段。

### 3.1 头部键（`KEY: value`）

| 键 | 含义 |
|---|---|
| `NAME:` | 实例名 |
| `TYPE:` | 固定 `QAP`（通用载入器按此分派） |
| `FACILITIES:` | 设施数 `n` |
| `LOCATIONS:` | 位置数（= `n`） |
| `COMMENT:` | 注释（整行跳过） |

### 3.2 数据段

| 段 | 内容 |
|---|---|
| `FLOW_SECTION`     | `n` 行，每行以**行号**开头、后跟 `n` 个数 = 设施间流量矩阵 `flow[i][j]` |
| `Distance_SECTION` | `n` 行，每行以**行号**开头、后跟 `n` 个数 = 位置间距离矩阵 `dist[k][l]` |

> 每行的**行首行号是必需的**（解析时读入并忽略），随后才是该行的 `n` 个矩阵值。

## 4. 样例

标准头（如 `bur26a.qap`）：

```
NAME: bur26a
TYPE: QAP
FACILITIES: 26
LOCATIONS: 26
FLOW_SECTION
 1  53 66 66 …
 2  66 53 66 …
 …
Distance_SECTION
 …
```

本目录含多个标准 QAPLIB 实例，可直接 `load("<名>")`。
