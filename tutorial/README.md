# EvoFlow C++ tutorials

Each `.cpp` is an independent host program. Add the selected source file to the
user's CMake or Visual Studio project, include EvoFlow's public source directories,
and link the same platform dependencies as the host application. The stock root
CMake project builds the `evoflow` executable rather than separate tutorial binaries.

## Connecting an external benchmark

`7.external-benchmark.cpp` demonstrates the supported native integration boundary:

1. obtain the benchmark source/library and data under its own license;
2. include its public header in the user's host project;
3. wrap one benchmark instance in an `eccalcul_functor`;
4. implement `copy` as a real independent clone when the benchmark has mutable
   evaluation scratch or run state;
5. declare dimension, bounds, accuracy and optimization direction through `Problem`;
6. link the external source/library into the host program, not into EvoFlow's release.

The included `external-benchmark/example-benchmark.*` is small EvoFlow-owned demonstration
code, not a redistributed third-party suite. Add both
`tutorial/7.external-benchmark.cpp` and
`tutorial/external-benchmark/example-benchmark.cpp` to the same user-owned build target.

For a prebuilt library, reproduce the same adapter in the user's CMake/Visual Studio
project and link that library there. The stock EvoFlow executable/UI does not load
arbitrary native libraries or source paths. Python users can delegate an objective to
an imported package callable; see `python/examples/external_benchmark.py`.

## Connecting external problem data

`external-data/convert_external_data.py` is a dependency-free Python 3.8+ tool for
user-downloaded TSPLIB, QAPLIB and standard MKP/GK files. It does not download or
redistribute third-party data. See `external-data/README.md` for publisher links,
conversion commands and the data-package verification flow.
