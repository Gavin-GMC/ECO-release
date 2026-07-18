# ECFlow

**ECFlow** 是一个 header-only 的 C++17 演化计算框架，面向组合优化与连续数值优化。它把"问题建模"与"优化器装配"两条主线解耦：用统一的接口描述问题（变量 / 目标 / 约束 / 启发），用可插拔的算子流水线装配优化算法，二者在运行期通过问题句柄对接。

## 特性

- **问题建模**：变量域、目标、约束（域 / 基数 / 累积 / 图 / 表达式）、启发式；内置 **14 类问题模板**（TSP、MKP、QAP、CFLP、CPMP、CVRP、FJSP、MDS、MIS、SR、STP、WFS、数值基准）。
- **优化算法**：可插拔算子（学习拓扑 / 学习策略 / 子代生成 / 评估 / 环境选择 / 修复），覆盖 PSO、GA、DE、ACO、EDA、CMA-ES、ES、免疫、烟花、灰狼 / 鲸鱼等系列及局部搜索。
- **配置驱动**：换算法 = 换配置，不改代码结构；配置可存取为 `.cfg`（文本 / JSON）。
- **可执行程序**：命令行工具 `ecflow`，从文件读入「问题 + 配置」跑优化、输出结果，可被脚本调用。
- **跨平台构建**：CMake（Windows / Linux）与 Visual Studio 工程两种方式；MSVC 静态运行时，产物自包含。

## 快速开始

### 1. 构建可执行程序

**CMake（跨平台）**
```sh
cmake -S build/cmake -B build/cmake/out
cmake --build build/cmake/out --config Release   # → 项目根 ecflow(.exe)
```

**Visual Studio（Windows）**
```
打开 build/vs/ecflow.sln，选 Release|x64，生成 → 项目根 ecflow.exe
```

### 2. 运行

```sh
ecflow <问题文件> <配置文件> [选项]

# 例:跑 TSP 算例 burma14,用蚁群配置 ACS,种子 42
./ecflow _pdata/tsp/burma14.tsp config/ACS.cfg -s 42
```

常用选项：`-s <种子>`、`-n <运行次数>`、`-o <输出文件>`、`--max-fes <N>`（覆盖预算）、`-q`（只输出 fitness）、`-v`（进度上屏）、`-h`（帮助）。

> 运行时 `ecflow` 按**当前工作目录**解析 `_pdata/`、`config/`；在项目根运行可用相对路径，从别处运行请传绝对路径。详见 [手册 · 可执行程序](docs/手册/13-使用-可执行程序.md)。

### 3. 作为库使用

```cpp
#include "ecflow.h"        // 统一入口
using namespace ECFlow;
```
见 `tutorial/` 的示例与 [用户手册](docs/手册/README.md)。

## 目录结构

| 目录 | 内容 |
|---|---|
| `src/` | 框架源码（header-only + 少量编译单元） |
| `_pdata/` | 问题算例数据（各家族格式见 `_pdata/<kind>/README.md`） |
| `config/` | 优化器配置表（`.cfg`） |
| `tutorial/` | 使用示例 |
| `docs/手册/` | 用户手册（使用者篇 + 开发者篇） |
| `build/` | 构建工程（`cmake/`、`vs/`） |
| `thirdparty/` | 第三方依赖（muparserx 等，已内置） |

## 文档

用户手册见 [`docs/手册/`](docs/手册/README.md)：问题建模、优化器配置、结果分析、可执行程序，以及扩展开发指南。

## 许可

本项目采用 **BSD 3-Clause License**。完整条款见 [`LICENSE`](LICENSE)。

```
BSD 3-Clause License

Copyright (c) 2025, Mingcan Geng

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```
