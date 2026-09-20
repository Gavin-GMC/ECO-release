# MKP 数据格式说明（Multidimensional Knapsack Problem，多维背包）

由 [`src/problem/template/pt-mkp.h`](../../src/problem/template/pt-mkp.h) 的 `PT_MKP` 解析。

## 1. 问题简介

`ITEMS` 个物品，每个有利润 `p[i]` 与在 `RESOURCES` 个维度上的资源占用 `r[k][i]`；每维有容量上限。选一个物品子集使总利润最大、且各维总占用不超容量。变量 `x` 长度 = 物品数、域 `{0,1}`；容量为约束。

## 2. 文件位置与加载

- 路径：`_pdata/mkp/<name>.mkp`
- 加载：`PT_MKP::load("<name>")`（例：`load("sample")`）；也可传完整/相对路径。

## 3. 文件格式

纯文本，空白分隔。头部键 + 数据段。

### 3.1 头部键（`KEY: value`）

| 键 | 含义 |
|---|---|
| `NAME:` | 实例名 |
| `TYPE:` | 固定 `MKP`（通用载入器按此分派） |
| `ITEMS:` | 物品数 `n` |
| `BACKPACKS:` | 背包数（标准 MKP = 1） |
| `RESOURCES:` | 资源维度数 `m` |
| `OPTIMAL:` | （可选）已知最优值，仅记录 |
| `COMMENT:` | 注释（整行跳过） |

### 3.2 数据段

| 段 | 内容 |
|---|---|
| `PROFIT_SECTION`   | `n` 行，每行 `id profit`（各物品利润 `p[i]`；`id` 仅作行标） |
| `RESOURCE_SECTION` | `n` 行，每行 `id r[0] r[1] … r[m-1]`（物品 `i` 在 `m` 个资源维上的占用） |
| `CAPACITY_SECTION` | `BACKPACKS` 行，每行 `id cap[0] … cap[m-1]`（各背包在 `m` 维上的容量） |
| `EOF` | 结束标记（可选） |

> 三个数据段每行均以一个行标 token 开头（解析时读入并忽略）。

## 4. 样例

首方最小样例头：

```
NAME: sample
TYPE: MKP
ITEMS: 5
BACKPACKS: 1
RESOURCES: 2
PROFIT_SECTION
1 10
2 7
…
```

程序最小包只带首方 `sample.mkp`；完整数据 overlay 增加 324 个具有明确 MIT 许可证据的 OR-Library 转换实例。GK 实例不再分发，外部下载/转换边界见 [`../../tutorial/external-data/README.md`](../../tutorial/external-data/README.md)。
