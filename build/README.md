# EvoFlow 源码构建入口

发布包同时提供 CMake 和 Visual Studio 两种 C++ 构建入口。两者都调用项目根目录的同一份 `CMakeLists.txt`，不会维护两套容易漂移的源码清单。

## CMake

可直接从项目根目录配置：

```text
cmake -S . -B obj-cmake-release
cmake --build obj-cmake-release --config Release
```

也可以使用与早期发布包一致的分目录入口：

```text
cmake -S build/cmake -B obj-cmake-release
cmake --build obj-cmake-release --config Release
```

默认产物为项目根目录下的 `evoflow` 或 `evoflow.exe`。`obj-*` 目录只保存生成文件，可在构建结束后删除。

## Visual Studio

使用 Visual Studio 2022 打开 `build/vs/evoflow.sln`，选择 `Release | x64` 后执行“生成解决方案”。该工程通过 CMake 构建当前源码，因此需要 `cmake` 可从命令行调用。

## Python

Python 扩展不加入 CMake 或 Visual Studio 工程。Windows 用户可直接使用 `python/evoflow/` 中与 CPython 3.8–3.12 版本匹配的预编译模块；需要从源码构建时使用 `build/python/build-python.ps1`。

```powershell
.\build\python\build-python.ps1 -PythonExe C:\Path\To\python.exe -SkipTests
```

EvoFlow 不随包提供 CEC 源码或数据。连接自行取得的外部 benchmark 请参考 `tutorial/7.external-benchmark.cpp` 和 `python/examples/external_benchmark.py`。
