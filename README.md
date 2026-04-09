# ECFC — Evolutionary Computing Framework with Constraints

> **中文** | [English](#english)

---

## 中文

### 项目简介

ECFC 是一个模块化、仅头文件的 C++ 进化计算库，专为求解组合优化问题而设计。框架原生支持约束处理、启发式信息集成与多目标优化，并提供丰富的算法组件和便捷的问题定义接口。

> 版权所有 © 2024–2026 耿明灿（Mingcan Geng）。依据 [BSD 3-Clause 许可证](LICENSE) 发布。

### 核心特性

- **丰富的算法支持** — GA、DE、EDA、PSO、BPSO、SetPSO、ACO（AS / ACS）、进化策略（ES）
- **灵活的拓扑结构** — 隔离、轮盘赌、锦标赛、均匀、随机、PSO、CSO、LLSO、SDLSO 等
- **约束处理** — 支持范围约束、相容性约束、容量约束、唯一性约束、分布约束、最小距离约束，具备约束传播与修复机制
- **多目标优化** — 支持基于排名-拥挤距离和排名-分解的选择策略，集成超体积（HV）、GD、IGD 指标
- **问题模板** — 内置 TSP、MKP、QAP 以及 CEC2013 基准测试（标准与大规模版本），支持基于标准化数据文件的问题载入（TSPLIB `.tsp`、`.mkp`、`.qap`）
- **配置驱动** — 支持代码构建或通过 `.cfg` 文件加载/保存优化器配置
- **结果分析** — 内置统计分析（均值、方差、标准差、中位数、众数）与显著性检验

### 项目结构

```
ECFC/
├── *.h                  # 核心框架头文件（仅头文件库）
├── _config/             # 57 个预配置的优化器配置文件（.cfg）
├── _pdata/              # 基准测试问题数据
│   ├── numerical/       #   CEC2013（标准规模与大规模）
│   ├── tsp/             #   旅行商问题实例
│   ├── qap/             #   二次分配问题实例
│   └── mkp/             #   多维背包问题实例
├── _rels/               # 外部依赖
│   ├── alglib/          #   匈牙利算法
│   └── metriclib/       #   超体积 / GD / IGD 计算
├── tutorial/            # 6 个递进式示例程序
├── Makefile
└── LICENSE
```

### 环境要求

- 支持 C++11 及以上标准的编译器（推荐 g++）
- GNU Make

无需额外安装依赖包，所有依赖已随源码附带于 `_rels/` 目录下。

### 快速上手

#### 1. 编译

```bash
make all       # 编译 main.cpp，生成可执行文件
make clean     # 清除编译产物
make help      # 显示可用目标
```

#### 2. 最小示例

```cpp
#include "ecfc.h"

int main() {
    // --- 问题定义 ---
    F_problem prob;
    prob.addVar("x", -5.0, 5.0, 0.001);   // 连续变量，范围 [-5, 5]
    prob.addVar("y", -5.0, 5.0, 0.001);
    prob.addObj([](const F_individual& ind) {
        double x = ind["x"], y = ind["y"];
        return x * x + y * y;              // 最小化球形函数
    });

    // --- 构建优化器 ---
    ECFC_Builder builder;
    builder.setPopSize(50)
           .setLearningStrategy("GA")
           .setTermination(FES, 10000);

    auto optimizer = builder.build(prob);
    optimizer.run();

    std::cout << "最优值: " << optimizer.getBestFitness() << "\n";
}
```

#### 3. 加载预置配置

```cpp
ECFC_Builder builder;
builder.loadConfig("_config/GA_SBX_PM.cfg");
auto optimizer = builder.build(prob);
optimizer.run();
```

### 示例教程

`tutorial/` 目录下提供 6 个由浅入深的示例：

| 文件 | 主题 |
|------|------|
| `0.problem.cpp` | 定义变量、目标函数、约束与启发式信息 |
| `1.optimzerconfig.cpp` | 构建和配置优化器 |
| `2.subpopulation.cpp` | 多子种群管理 |
| `3.population-manager.cpp` | 种群管理策略 |
| `4.log&analysis.cpp` | 运行日志与结果分析 |
| `5.config_generatre.cpp` | 代码生成配置文件 |

### 支持的算法

#### 学习策略

| 标识 | 算法 |
|------|------|
| `GA` | 遗传算法（SBX+PM、点+位、均匀+高斯、顺序+翻转等） |
| `DE` | 差分进化 |
| `EDA` | 分布估计算法 |
| `PSO` | 粒子群优化 |
| `BPSO` | 二进制粒子群优化 |
| `SetPSO` | 集合粒子群优化 |
| `AS` | 蚁群系统（ACO） |
| `ACS` | 蚁群优化系统（ACO） |
| `ES` | 进化策略 |

#### 学习拓扑

`隔离` · `轮盘赌` · `锦标赛` · `均匀` · `随机` · `PSO` · `CSO` · `LLSO` · `SDLSO` · `SLLSO` · `SSDLSO`

#### 预置混合配置（`.cfg`）

`CS+GA` · `LL+ACS` · `LL+GA` · `SDL+GA` · `championship+SPSO` · `random+SPSO` · `roulette+SPSO` 等 57 种配置

### 问题模板

| 头文件 | 问题类型 | 数据文件格式 |
|--------|----------|-------------|
| `pt-numerical.h` | CEC2013 数值基准测试（标准规模与大规模） | 程序内嵌加载 |
| `pt-tsp.h` | 旅行商问题（对称 / 非对称） | TSPLIB `.tsp` |
| `pt-mkp.h` | 多维背包问题 | `.mkp` |
| `pt-qap.h` | 二次分配问题 | `.qap` |

### 结果分析

使用 `ec-analyzer.h` 对多次实验结果进行汇总与比较：

```cpp
EC_Analyzer analyzer;
analyzer.add(run1_results);
analyzer.add(run2_results);
analyzer.report();   // 输出均值、标准差、中位数及显著性检验
```

支持输出格式：纯文本、Excel 兼容格式、LaTeX 表格。

### 引用声明

若您在研究中使用了 ECFC，请在发表的成果中注明并引用 **ECFC**。

### 许可证

BSD 3-Clause 许可证 — 详见 [LICENSE](LICENSE)。

---

## English

### Overview

ECFC is a modular, header-only C++ library for evolutionary computation and metaheuristic optimization, designed for solving combinatorial optimization problems. It provides a convenient problem-definition interface with native support for combination constraints, heuristic information, and multi-objective optimization.

> Copyright (c) 2024–2026 Mingcan Geng. Licensed under the [BSD 3-Clause License](LICENSE).

### Features

- **Rich algorithm suite** — GA, DE, EDA, PSO, BPSO, SetPSO, ACO (AS / ACS), and Evolution Strategies
- **Flexible topology / learning structures** — Isolate, Roulette, Championship, Uniform, Random, PSO, CSO, LLSO, SDLSO, and more
- **Constraint handling** — range, compatibility, capacity, uniqueness, distributed, minimum-distance constraints with propagation and repair
- **Multi-objective support** — rank-crowding and rank-decomposition selection, hypervolume / GD / IGD metrics
- **Problem templates** — ready-to-use templates for TSP, MKP, QAP, and CEC2013 benchmarks (normal & large-scale), with support for loading from standardized data files (TSPLIB `.tsp`, `.mkp`, `.qap`)
- **Configuration-driven** — build optimizers programmatically or load/save `.cfg` files
- **Analysis utilities** — built-in statistical analysis (mean, variance, std, median, mode) and significance testing

### Project Structure

```
ECFC/
├── *.h                  # Core framework headers (header-only library)
├── _config/             # 57 pre-configured optimizer configurations (.cfg)
├── _pdata/              # Benchmark problem data
│   ├── numerical/       #   CEC2013 (normal & large-scale)
│   ├── tsp/             #   Traveling Salesman Problem instances
│   ├── qap/             #   Quadratic Assignment Problem instances
│   └── mkp/             #   Multi-Dimensional Knapsack Problem instances
├── _rels/               # External dependencies
│   ├── alglib/          #   Hungarian algorithm
│   └── metriclib/       #   Hypervolume / GD / IGD computation
├── tutorial/            # Six progressive example programs
├── Makefile
└── LICENSE
```

### Requirements

- C++11 (or later) compliant compiler (g++ recommended)
- GNU Make

No additional package installation is required — all dependencies are bundled under `_rels/`.

### Quick Start

#### 1. Build

```bash
make all       # compile main.cpp → executable
make clean     # remove build artifacts
make help      # display available targets
```

#### 2. Minimal example

```cpp
#include "ecfc.h"

int main() {
    // --- Problem definition ---
    F_problem prob;
    prob.addVar("x", -5.0, 5.0, 0.001);   // continuous variable, range [-5, 5]
    prob.addVar("y", -5.0, 5.0, 0.001);
    prob.addObj([](const F_individual& ind) {
        double x = ind["x"], y = ind["y"];
        return x * x + y * y;              // minimize sphere function
    });

    // --- Optimizer construction ---
    ECFC_Builder builder;
    builder.setPopSize(50)
           .setLearningStrategy("GA")
           .setTermination(FES, 10000);

    auto optimizer = builder.build(prob);
    optimizer.run();

    std::cout << "Best: " << optimizer.getBestFitness() << "\n";
}
```

#### 3. Load a pre-built configuration

```cpp
ECFC_Builder builder;
builder.loadConfig("_config/GA_SBX_PM.cfg");
auto optimizer = builder.build(prob);
optimizer.run();
```

### Tutorial

Six step-by-step examples are provided in `tutorial/`:

| File | Topic |
|------|-------|
| `0.problem.cpp` | Defining variables, objectives, constraints, and heuristics |
| `1.optimzerconfig.cpp` | Building and configuring an optimizer |
| `2.subpopulation.cpp` | Working with multiple subpopulations |
| `3.population-manager.cpp` | Population management strategies |
| `4.log&analysis.cpp` | Logging runs and analyzing results |
| `5.config_generatre.cpp` | Generating configuration files programmatically |

### Supported Algorithms

#### Learning Strategies

| ID | Algorithm |
|----|-----------|
| `GA` | Genetic Algorithm (SBX+PM, point+bit, uniform+gauss, order+turnover, …) |
| `DE` | Differential Evolution |
| `EDA` | Estimation of Distribution Algorithm |
| `PSO` | Particle Swarm Optimization |
| `BPSO` | Binary PSO |
| `SetPSO` | Set-based PSO |
| `AS` | Ant System (ACO) |
| `ACS` | Ant Colony System (ACO) |
| `ES` | Evolution Strategies |

#### Learning Topologies

`Isolate` · `Roulette` · `Championship` · `Uniform` · `Random` · `PSO` · `CSO` · `LLSO` · `SDLSO` · `SLLSO` · `SSDLSO`

#### Hybrid / Composed Configurations (pre-built `.cfg`)

`CS+GA` · `LL+ACS` · `LL+GA` · `SDL+GA` · `championship+SPSO` · `random+SPSO` · `roulette+SPSO` · and 57 configurations in total.

### Problem Templates

| Template | Problem | Data File Format |
|----------|---------|-----------------|
| `pt-numerical.h` | CEC2013 numerical benchmarks (normal & large-scale) | Embedded (programmatic) |
| `pt-tsp.h` | Traveling Salesman Problem (symmetric / asymmetric) | TSPLIB `.tsp` |
| `pt-mkp.h` | Multi-Dimensional Knapsack Problem | `.mkp` |
| `pt-qap.h` | Quadratic Assignment Problem | `.qap` |

### Analysis

Use `ec-analyzer.h` to collect and compare experimental results across multiple runs:

```cpp
EC_Analyzer analyzer;
analyzer.add(run1_results);
analyzer.add(run2_results);
analyzer.report();   // prints mean, std, median, significance tests
```

Supported output formats: plain text, Excel-compatible, LaTeX table.

### Citation

If you use ECFC in your research, please acknowledge it by citing or referencing **ECFC** in your publication.

### License

BSD 3-Clause License — see [LICENSE](LICENSE) for details.
