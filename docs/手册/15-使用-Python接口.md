# 15 · 使用 Python 接口

EvoFlow 3.0.1.5 提供预编译 Python 包，覆盖问题建模、配置、运行控制、问题模板/实例发现以及结果分析。Python 层是 C++ 平台的安全门面：算法、配置 codec、校验、问题格式、日志和分析语义仍以原生实现为唯一来源。

## 1. 发布布局与支持范围

```text
EvoFlow-release/
├─ evoflow.exe
├─ install-python.ps1
├─ release-manifest.json
├─ python/
│  ├─ README.md
│  ├─ examples/
│  └─ evoflow/
│     ├─ _core.cp38-win_amd64.pyd
│     ├─ _core.cp39-win_amd64.pyd
│     ├─ _core.cp310-win_amd64.pyd
│     ├─ _core.cp311-win_amd64.pyd
│     ├─ _core.cp312-win_amd64.pyd
│     ├─ *.py / *.pyi
│     └─ py.typed
├─ config/
└─ _pdata/
```

- 正式验证：Windows x64、64 位 CPython 3.8.5、3.9.23、3.10.19、3.11.9、3.12.12，分别对应 `cp38`–`cp312` 五个 tagged `.pyd`。
- 原生二进制必须匹配 CPython 实现、主/次版本、位数和平台；单个 `.pyd` 不跨 CPython 次版本通用。
- 五个文件可在同一包目录并存，解释器按自己的 extension suffix 自动选择；矩阵外版本不宣称已验证。
- Linux 已在 Ubuntu 24.04.4 WSL2 x86_64、GCC 13.3、CPython 3.12.3 环境完成源码构建和 16 个可移植测试，但不提供预编译原生模块，也不承诺其他 Python 小版本的二进制兼容。
- 不支持 32 位 Python、PyPy 或非 Windows 预编译包。
- 不依赖 NumPy；普通 Python 数字和序列会按绑定契约转换，不承诺 NumPy 零拷贝。
- Python 不加入任何 `CMakeLists.txt`。`evoflow.exe` 与 Python 包一起发布，但不放进 `python/`。
- `evoflow.__version__` 与 CLI/UI 共享产品版本 `3.0.1.5`；配置 `v3.1/v3.2` 和持久化文件格式版本保持独立。
- Windows 程序 ZIP 中五个 `.pyd` 同时存在；根 `release-manifest.json` 记录它们的大小与 SHA-256，整个 ZIP 另带 `.sha256`。程序包只带 13 个首方样例，完整 701 文件数据 overlay 独立安装。

## 2. 使用与安装

临时使用可在发布根目录设置：

```powershell
$env:PYTHONPATH = "$PWD\python"
python -m evoflow
```

长期使用推荐为指定解释器写入一个 `.pth` 路径文件：

```powershell
.\install-python.ps1 -PythonExe C:\Path\To\python.exe
python -m evoflow --check-data
```

脚本先按该解释器的 `importlib.machinery.EXTENSION_SUFFIXES` 检查匹配的 `_core`，再验证实际导入。它不复制包，因此发布目录移动后需要重新安装路径。卸载只删除这一个路径文件，即使原发布目录已不存在也可执行：

```powershell
.\install-python.ps1 -PythonExe C:\Path\To\python.exe -Remove
```

虚拟环境可通过 `-TargetDirectory <venv>\Lib\site-packages` 指定目标；暂存构建验证可额外传 `-PackageRoot`。

## 3. 安装诊断

```powershell
python -m evoflow
python -m evoflow --version
python -m evoflow --check-data
python -m evoflow --check-data D:\datasets\EvoFlow\_pdata
```

默认输出包版本、Python/ABI、平台、实际加载的 `.pyd`、串/并行构建标记、`_pdata` 路径及注册 TYPE。`--check-data` 额外复算 manifest 中每个文件的大小和 SHA-256，报告 `missing/minimal/full/incomplete`。程序中可读取安装结构信息：

```python
from evoflow import installation_info

info = installation_info
print(info.native_module, info.python_tag, info.registered_problem_types)
```

若二进制缺失或不匹配，导入会抛出带当前解释器信息、已打包二进制名、构建指令和原始 loader 错误的 `ImportError`。

## 4. 五类用户入口

### 4.1 问题建模与文件问题

`evoflow.problem` 公开 `Problem`、`CompiledProblem`、变量/目标/约束描述、问题文件类型查询、描述与加载。`CompiledProblem.is_better(first, second)` 复用原生目标优先级及最小化/最大化比较；`format_solution(values, objectives, full=False)` 复用原生解码器。两者均先校验序列长度，不公开裸 comparer/decoder 指针。完整建模示例见 `python/examples/problem_model.py`。

### 4.2 配置

`evoflow.config` 公开 `Config`、workflow、subpopulation、组件目录、参数 schema、依赖/供给查询以及静态/深度校验。配置并保存示例见 `python/examples/config_model.py`。

`Config.validate` 的 `ValidationReport.status` 为 `error / warning / info / pass` 中的最高级，`error_files / warning_files / info_files / pass_files` 按每份文件的最高级计数；`ConfigFileReport` 提供同名 `status` 及三种 issue 数量，单条 `ConfigIssue.severity` 为 `error / warning / info`。`report.ok` 只在 Error 或加载失败时为假，故应先展示全部 issue，再决定是否阻止后续操作。`Workflow.check_component` 同步返回 `errors / warnings / infos` 数量；Info 对应 `informational`，Warning 对应 `notfullysatisfied`，两者的 `allowed` 均为真。

### 4.3 运行

`evoflow.runtime` 的 `Optimizer` 适合复用问题/配置快照；`run` 是一次性内存入口；`run_files` 复用 CLI/App 文件门面。三者均支持 `optimizer_name=` 覆盖配置名，默认扫描同一优化器/问题/tag 的磁盘结果并从最大编号后续号。`RunRoundResult.run` 是本次调用内 1-based 序号，`result_index` 是实际文件后缀；`RunResult.run_start` 返回本批首个实际编号。

```python
result = run_files(
    "_pdata/tsp/sample.tsp", "config/ACS.cfg",
    problem_name="sample",
    optimizer_name="ACS_sweep_01",
    tag="experiment_a",
    runs=3,
)
print(result.run_start)
print([round.result_index for round in result.rounds])

# 只有确实要重做旧编号时才同时给出这两个参数：
replacement = run_files(
    "_pdata/tsp/sample.tsp", "config/ACS.cfg",
    problem_name="sample", optimizer_name="ACS_sweep_01",
    tag="experiment_a", runs=3,
    run_start=0, replace_existing=True,
)
```

只给 `run_start` 时，范围内任一旧结果都会让整批在优化开始前失败；`replace_existing=True` 必须同时给显式起点。替换会暂存新文件并备份旧 `.rslt/.prcs`，失败或取消恢复旧结果。进度回调按完成轮次报告，取消返回部分 `RunResult`；`continue_run` 只在成功、未取消的单轮后可用，不等同于检查点或跨进程恢复。

### 4.4 模板与实例目录

`evoflow.templates` 公开 13 个注册 TYPE、12 类具体模板、发布 `_pdata` 根和安全实例发现。数据仍与 exe、Python 包并列，不复制进 Python 包：程序包含 13 个首方样例，可选完整 overlay 再加入 701 个许可明确的外部实例。模板目录示例见 `template_catalog.py`；把外部 benchmark 包或自有函数作为目标 callable 的示例见 `external_benchmark.py`。

### 4.5 结果分析

`evoflow.analysis` 直接复用原生批次发现、报告、统计/指标/比较、TXT/CSV/LaTeX、SVG/PNG 图表及 `.prcs` 读取。非 Windows 平台生成 PNG 时必须显式传入系统中真实存在且可读的字体路径。`ExperimentBatch.run_numbers` 是所有所选优化器/问题共同具备并实际参与统计的编号，允许如 `(3, 5)` 的非零空洞集合；`run_diagnostics` 明列每个单元的 found/missing。`analyze_results.py` 给出最小持久化批次；单个 `.rslt` 的私有原始解析和原生未实现的 Excel/Word 输出不公开空桩。

所有常用名称同时从 `evoflow` 包顶层重导出。

## 5. 类型、异常与所有权

- 包含完整 `.pyi` 和 `py.typed`；原生类、方法、属性、枚举、模块函数及公开门面均有声明。
- 输入校验使用 `TypeError` / `ValueError`；运行失败使用 `RuntimeError`；分析计划或批次错误使用 `AnalysisError`；加载/ABI 错误使用 `ImportError`。
- `Problem.compile`、`Optimizer` 和配置复制形成原生所有权快照；源码对象之后修改或释放不影响已构建对象。
- 长原生段释放 GIL；进入 Python 目标、进度、取消或扫描回调时重新取得 GIL。Python 回调保证正确性，不承诺绕过 GIL 并行加速。
- 同一 `Optimizer` 实例不支持并发或重入；独立实例可分别使用。

## 6. 示例

| 文件 | 路径 |
|---|---|
| `problem_model.py` | 定义、描述、编译和求值问题 |
| `config_model.py` | 构筑、校验并保存配置 |
| `run_optimizer.py` | 内存运行、进度和续跑 |
| `template_catalog.py` | TYPE、实例发现和模板加载 |
| `file_workflow.py` | 问题文件 + 配置文件运行 |
| `analyze_results.py` | 持久化批次发现和原生分析 |
| `external_benchmark.py` | 以目标 callable 接入外部 benchmark |

在已安装或已设置 `PYTHONPATH` 的环境中直接运行：

```powershell
python python\examples\file_workflow.py
```

## 7. 从源码构建

构建不经过 CMake：

```powershell
.\build\python\build-python.ps1 -PythonExe C:\Path\To\python.exe
.\build\python\build-python.ps1 -PythonExe C:\Path\To\python.exe -Parallel
```

单版本脚本把发布用串行 `.pyd` 放入 `python/evoflow`；`-Parallel` 只在 `obj-python-parallel/python/evoflow` 暂存验证，避免覆盖正式产物。脚本要求 Windows x64、MSVC 2022、64 位 CPython 3 和仓库内 pybind11 源码。

维护者源码树中的正式发布入口一次构建并验证五个解释器：

```powershell
.\build-python-matrix.ps1 `
  -Python38Exe C:\Path\To\Python38\python.exe `
  -Python39Exe C:\Path\To\Python39\python.exe `
  -Python310Exe C:\Path\To\Python310\python.exe `
  -Python311Exe C:\Path\To\Python311\python.exe `
  -Python312Exe C:\Path\To\Python312\python.exe
```

公开程序包不携带维护者测试目录和矩阵编排脚本，但保留绑定源码及单版本 `build/python/build-python.ps1`；用户自行重建一个 ABI 时使用 `-SkipTests`。正式发布的五个文件只接受维护者源码树完整矩阵结果。

矩阵脚本要求五个路径恰好对应 CPython 3.8–3.12 Windows x64。它逐版本执行完整 Python 回归，最后检查包内恰好存在五个串行 `.pyd`，并由五个解释器分别实导入自己的 tagged 文件。并行构建不进入发布包。

2.6.8 基线的五个串行版本各通过 16 个测试脚本、341 项检查和 6 个示例；矩阵专项每版本另有 7 项。2.7.4 已从 3.0.0 最终发布源码重新构建五个正式 `.pyd`：每版通过现行 17 个测试脚本，矩阵专项每版 7/7，随后又在组装后的发布树中分别通过精确 ABI 导入、数据验证、文件运行和原生分析；未沿用旧二进制。最终文件摘要以发布根 `release-manifest.json` 为准。
