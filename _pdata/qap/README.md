# QAP 数据格式说明（Quadratic Assignment Problem，二次分配）

由 [`src/problem/template/pt-qap.h`](../../src/problem/template/pt-qap.h) 的 `PT_QAP` 解析。

## 1. 问题简介

`n` 个设施分配到 `n` 个位置（一一对应）。设施间有流量 `flow[i][j]`，位置间有距离 `dist[k][l]`。求分配排列 `π`，最小化 `Σ_{i,j} flow[i][j]·dist[π(i)][π(j)]`。变量 `x` 长度 `n`、域 `{0..n-1}` = 设施→位置的**排列**；`unique` 约束保证排列。

## 2. 文件位置与加载

- 路径：`_pdata/qap/<name>.qap`
- 加载：`PT_QAP::load("<name>")`（例：`load("sample")`）；也可传完整/相对路径。

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

首方最小样例：

```
NAME: sample
TYPE: QAP
FACILITIES: 3
LOCATIONS: 3
FLOW_SECTION
1 0 5 2
2 5 0 3
3 2 3 0
Distance_SECTION
1 0 2 3
2 2 0 1
3 3 1 0
EOF
```

公开包只提供上述 EvoFlow 首方样例，不重新分发 QAPLIB。用户可从 QAPLIB 发布方取得原始 `.dat`，再按 [`../../tutorial/external-data/README.md`](../../tutorial/external-data/README.md) 转换。
