<p align="center">
  <img src="figs/Logo-readme.png" alt="EvoFlow" width="560">
</p>

> This release has completed the full Windows regression suite, release interface validation, and Ubuntu WSL build and portable interface validation.

# EvoFlow

[English](README.md) | [简体中文](README.zh-CN.md)

EvoFlow is a C++17 evolutionary computation platform for combinatorial and continuous optimization. It separates problem modeling, optimizer assembly, execution and evaluation, and statistical analysis. A registry-based component catalog lets you combine topologies, strategies, generators, evaluators, and selectors.

The current public product version is **3.0.1.6**. Configuration versions `v3.1/v3.2` and the format versions of result and execution-history files are independent protocol versions; they do not change with the product version.

Version 3.0.1.6 updates the documentation and source version metadata. Existing prebuilt executables, Python native modules, and download archives remain at 3.0.1.5; this update does not rebuild or republish them.

## Platform Support

| Capability | Current validation scope |
|---|---|
| Windows command line, GUI, and model integration | Windows x64 |
| Prebuilt Windows Python package | Windows x64, 64-bit CPython 3.8–3.12 |
| Linux C++ core and command line | Built from source and validated on Ubuntu 24.04.4 WSL2 x86_64 with GCC 13.3, CMake 3.28.3, and Ninja 1.11.1; no prebuilt Linux release package is provided |
| Linux Python interface | Built from source and passed portability tests with CPython 3.12.3 in the environment above; no prebuilt Linux modules are provided, and binary compatibility across Python minor versions is not guaranteed |
| Linux GUI and model integration | Currently unsupported |
| macOS | Not validated; no support commitment |

## Quick Start

On Windows, double-click `evoflow.exe`, or run the following from the project directory:

```text
evoflow
```

The program starts a local service and opens the GUI, where you can define or load problems, assemble optimizers, run the current configuration, and analyze experimental results.

Run a single problem with a configuration from the command line:

```text
evoflow run <problem-file> <config-file>
```

Display the product version:

```text
evoflow --version
```

Check the minimal/full data packages:

```text
evoflow data
python -m evoflow --check-data
```

For all command-line options, see the [Executable Guide](docs/manual/13-command-line.md).

## Windows Release Assets

The Windows x64 release provides three separate ZIP archives that can be used together:

- `EvoFlow-3.0.1.5-windows-x64.zip`: the executable, public C++/Python source code, five native modules for CPython 3.8–3.12, configurations, tutorials, documentation, and 13 first-party minimal examples.
- `EvoFlow-3.0.1.5-data-minimal.zip`: the same 13 first-party examples, available separately.
- `EvoFlow-3.0.1.5-data-full.zip`: an optional overlay containing 701 additional instances with verified licensing status.

Each ZIP has a matching `.sha256` file. After extracting the program archive, you can run `evoflow.exe` directly. To add the full dataset, extract the full ZIP into the same directory. The release tree includes `release-manifest.json`, which records the byte size and SHA-256 hash of every file except the manifest itself.

## Documentation

The user and developer manuals are available in English. Data-package, build, release-note, and roadmap documents linked below remain in Chinese.

- [Project Overview](docs/manual/00-overview.md)
- [Graphical User Interface](docs/manual/14-gui.md)
- [Python Interface](docs/manual/15-python.md)
- [Problem Modules](docs/manual/10-problems.md)
- [Optimizer Configuration](docs/manual/11-optimizer-configuration.md)
- [Result Analysis](docs/manual/12-analysis.md)
- [Data Packages and External Data](docs/数据包与外部数据.md)
- [EvoFlow 3.0.1.6 Release Notes](发布日志.md)
- [Roadmap](未来更新计划.md)
- [Building from Source](build/README.md)
- [Development Architecture and Workflow](docs/manual/20-architecture.md)
- [Complete User Manual Index](docs/manual/README.md)

## Building from Source

```text
cmake -S . -B obj-cmake-release
cmake --build obj-cmake-release --config Release
```

You can also use the entry point under `build/cmake/`, or open `build/vs/evoflow.sln` in Visual Studio 2022. See [Building from Source](build/README.md) for details. The `build/` directory contains distributable build project files; generated files should go into disposable `obj-*` directories.

Windows release candidates are built from a separate, clean staging tree using Visual Studio 2022/CMake. The `evoflow.exe` executable uses `/MT` and does not require a separately installed Visual C++ Redistributable. Python is not part of the CMake build; prebuilt Python modules are generated through a separate Python build entry point.

In the release directory, `figs/Logo.png` is used by the GUI. This README uses `figs/Logo-readme.png`, which is cropped from the same original image solely to remove blank space above and below it. When moving `evoflow.exe`, keep the `figs` directory alongside it.

## License and External Components

EvoFlow's own source code is licensed under the [BSD 3-Clause License](LICENSE). See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for external component versions, copyright notices, and licenses.

Since `v3`, the source tree, builds, public APIs, and release artifacts have excluded the CEC2013 competition benchmarks. If you have independently obtained an external benchmark lawfully, you can integrate it through the C++ functor interface in [`tutorial/7.external-benchmark.cpp`](tutorial/7.external-benchmark.cpp) or the Python callable interface in [`python/examples/external_benchmark.py`](python/examples/external_benchmark.py). The prebuilt `evoflow.exe` and GUI do not dynamically load arbitrary external native libraries.
