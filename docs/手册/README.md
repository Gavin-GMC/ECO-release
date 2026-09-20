# EvoFlow 3.0.1.5 用户手册

> 面向**使用者**与**二次开发者**的文档。按读者分三篇，配 `tutorial/` 可运行示例。
> 设计和内部规划类文档见 `docs/` 根目录其它文件；本目录只描述用户和二次开发者可依赖的公开行为。

## 阅读路线

| 你是 | 从这里开始 |
|---|---|
| 想快速跑通一个优化 | [00-总览](00-总览.md) → `tutorial/0.problem.cpp`、`tutorial/1.optimizerconfig.cpp` |
| 要建模自己的问题或接入外部 benchmark | 使用者篇 · [问题模块](10-使用-问题模块.md) → `tutorial/7.external-benchmark.cpp` |
| 要从 Python 定义并编译问题 | 使用者篇 · [问题模块 §7](10-使用-问题模块.md#7-python-问题建模262) → `python/examples/problem_model.py` |
| 要用 Python 完成配置、运行、模板发现和结果分析 | 使用者篇 · [Python 接口](15-使用-Python接口.md) → `python/examples/` |
| 要配置/调参优化器 | 使用者篇 · [优化器配置](11-使用-优化器配置.md) |
| 要做多算法实验对比 | 使用者篇 · [结果分析](12-使用-结果分析.md) → `tutorial/4.log-analysis.cpp` |
| 要用图形界面检查问题、构筑配置或查看报告 | 使用者篇 · [图形界面](14-使用-图形界面.md) → 双击 `evoflow.exe` |
| 要用命令行 exe 跑(不写代码) | 使用者篇 · [可执行程序](13-使用-可执行程序.md) → `evoflow run <问题> <配置>` |
| 要检查数据包或导入外部 benchmark 数据 | [可执行程序 §2.5](13-使用-可执行程序.md#25-检查数据包)、[Python 接口](15-使用-Python接口.md) → `tutorial/external-data/README.md` |

## 目录

**总览**
- [00-总览](00-总览.md) — 三层架构、执行链、两条使用主线、术语表

**使用者篇（模块接口与用例）**
- [10-使用-问题模块](10-使用-问题模块.md) — C++ 变量/目标/约束（含 §3.1 约束详表）/启发定义、句柄接口、12 个问题模板（含 §6.1 各模板构造 API）、外部 benchmark 接入，以及 §7 Python 问题建模/回调/构造接口 ✅
- [11-使用-优化器配置](11-使用-优化器配置.md) — ConfigBuilder fluent API、算子目录、**§6 算法配方表**（PSO/GA/DE/ACO/ES/CMA-ES/GWO/WOA/免疫/FWA/局搜… → 组件配置） ✅
- [12-使用-结果分析](12-使用-结果分析.md) — ECAnalyzer：读 `.rslt` → 三阶段流水线（归约/聚合/比较）、统计/质量指标（含 peakRatio）、Txt/Csv/LaTeX、SVG/PNG；Excel/Word 尚未实现 ✅
- [13-使用-可执行程序](13-使用-可执行程序.md) — 命令行 exe `evoflow run <问题> <配置> [选项]` 与 `evoflow data [路径]`：Windows x64 已验证构建；Linux 已在 Ubuntu 24.04.4 WSL2 完成核心/CLI、Sanitizer 与 CPython 源码验证，但不提供预编译包；Windows `/MT` 构建无 VC++ 运行时依赖；并说明参数、路径、输出、问题文件载入与数据包结构诊断 ✅
- [14-使用-图形界面](14-使用-图形界面.md) — 裸启动与退出、问题定义读取、表单/积木式优化器构筑、`.cfg` 保存、共用校验链、统计报告与 SVG/PNG 下载、安全边界和常见问题 ✅
- [15-使用-Python接口](15-使用-Python接口.md) — 发布布局、CPython 3.8–3.12 ABI 矩阵、`.pth` 安装、数据包 SHA-256 诊断、五类用户接口、类型提示、异常与 7 个可运行示例（含外部 benchmark callable）✅

**开发者篇（底层架构 / 流程 / 内部组件 / 规范）**
- [20-开发-架构与流程](20-开发-架构与流程.md) — 4 域 Define→compile→Manager、执行编排、装配流程 ✅
- [21-开发-内部组件](21-开发-内部组件.md) — 注册表机制、断言匹配器、如何加一个新算子（含逐个体状态三范式） ✅
- [23-开发-组件基类参考](23-开发-组件基类参考.md) — 14 个基类（拓扑/策略/生成器/选择器/评估/终止/档案/初始化/修复/个体/子群/管理器/子群构建器/子群拓扑）职责 + 虚接口签名与语义 + 契约 ✅
- [24-开发-组件清单](24-开发-组件清单.md) — 各族**逐个**列出全部具体组件（tag·语义·参数）：拓扑 19 · 策略 37 · 生成器 4 · 选择器 9 · 修复 5 · 个体 7 · 管理器/构建器/子群拓扑 等 ✅
- [25-开发-扩展问题模块](25-开发-扩展问题模块.md) — 加新约束（Constrain 子类→Kind→Spec→compile→Problem API）/ 加问题模板 / 加目标后端；问题侧⇄优化器侧扩展对照 ✅
- [27-开发-目录结构](27-开发-目录结构.md) — `src/` 全量目录 + 每处职责 + **注册组件族⇄目录速查**（加组件放哪一目查） ✅
- [26-表达式公式语法](26-开发-表达式公式语法.md) — 约束 cp（算术+关系，无函数，F≤0/域传播）vs 目标·启发 muParserX（+数学函数，直接求值）；`x[i]` 引用、指示逻辑、速查对照 ✅

**过程记录**

## tutorial 示例（参照稳定版 0–5 组织，适配当前 API）

| 文件 | 内容 | 状态 |
|---|---|---|
| `0.problem.cpp` | 问题定义全流程（变量/目标/约束/启发 + 编译 + 解评估/比较/约束传播/启发排序） | 可运行 |
| `1.optimizerconfig.cpp` | 单子群优化器配置与运行（ConfigBuilder → buildOptimizer → exe → 取最优） | 可运行 |
| `2.subpopulation.cpp` | 多段 workflow（交叉段+变异段）+ 子种群层访问 | 可运行 |
| `3.population-manager.cpp` | 多子群协作（异构子群 PSO+GA、NoInteraction 管理器） | 可运行 |
| `4.log-analysis.cpp` | 日志两轴（sys_logger + 每优化器结果/过程）+ **ECAnalyzer 分析**（读 `.rslt` → 对比表 → Txt/Csv/LaTeX，v4 迁移） | ✅ |
| `5.template.cpp` | 用问题模板端到端（PT_TSP 一键构造 → ACO 跑 → 取最优巡回） | 可运行 |
| `5.config-generate*.cpp` | 配置表批量生成（已有，见 tutorial 现有三文件） | ✅ 既有 |
| `python/examples/problem_model.py` | Python 定义 Problem → 描述 → 编译快照 → 贪心构造 → 求值/合法性 | ✅ 2.6.2 |
| `python/examples/config_model.py` | Python 构筑、深度校验并保存配置 | ✅ 2.6.3 |
| `python/examples/run_optimizer.py` | 内存问题运行、进度与受限续跑 | ✅ 2.6.4 |
| `python/examples/template_catalog.py` | 注册 TYPE、发布数据实例发现与模板加载 | ✅ 2.6.7 |
| `python/examples/file_workflow.py` | 复用问题文件和配置文件运行 | ✅ 2.6.7 |
| `python/examples/analyze_results.py` | 发现并分析持久化实验批次 | ✅ 2.6.7 |
