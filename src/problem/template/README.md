# problem-template — 经典问题模板（从 ECFC 原代码提取）

四个**并列独立**模板类（无公共基类，原样自 ECFC 原代码），各有 `getProblem()` 用 v2.2 的
`Problem` API 构建问题定义；编译得 `ProblemHandle` 后即可求解。

| 类 | 头 | 数据来源 | 冒烟测试 |
|---|---|---|---|
| `PT_TSP` | `pt-tsp.h` | `setCitys/setMap` 或 `load("_pdata/tsp/<name>")` | `smoke-tsp.cpp` |
| `PT_MKP` | `pt-mkp.h` | `addBackpacks/addItems` 或 `load("_pdata/mkp/<name>")` | `smoke-mkp.cpp` |
| `PT_QAP` | `pt-qap.h` | `setFacilities/setLocations` 或 `load("_pdata/qap/<name>")` | `smoke-qap.cpp` |

`problem-template.h` 聚合 include 四者。

## 用法（以 TSP 为例）
```cpp
PT_TSP tsp; tsp.setName("x"); tsp.setMap(dist, n, true);
Problem* p = tsp.getProblem();
ProblemHandle* h = p->compile();
// ... 用 h 求解 ...
delete h; delete p;
```

## 数据路径约定
- **运行期实例**（TSP/MKP/QAP 的 `load`）：硬编码 `"_pdata/<kind>/<name>.<ext>"`，相对**进程 CWD**。从发布根目录运行程序时会直接命中根目录下的 `_pdata/`。
- **外部 benchmark**：不作为内建问题模板分发。C++ 用户通过 `Problem::addObjective`
  包装并链接自行取得的源码或库；Python 用户直接传 objective callable。完整示例见
  `tutorial/7.external-benchmark.cpp` 和 `python/examples/external_benchmark.py`。

## 提取时所做的最小适配（相对原代码）
1. 四个 pt-* 补回原本经 `problem.h` 传递获得的标准库/日志 include（`<fstream>/<sstream>/<string>/<cstring>` + `logger.hpp`）——v2.2 精简的 `problem.h` 不再传递。
2. `Problem::addVariable` 增可选 `VariableType type = normal` 参数（PT_TSP 用 `sequence_*`）；加性、默认值，既有调用零影响。
4. 域层修复：`evoflow-math.h` 的 `eu/man/che_distance` 去掉错误的 `inline`（声明 inline 却在 `.cpp` 定义会导致符号不导出）。

## 构建依赖
- include：`src/problem/template`（与问题核心及具体模板依赖一并加入宿主工程）。
- 库：根目录 CMake 工程会从源码编译平台基础组件和 `muparserx`；外部 benchmark 由用户宿主工程自行链接。
