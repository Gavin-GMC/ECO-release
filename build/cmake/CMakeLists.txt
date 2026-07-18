# ECFlow 可执行程序 —— CMake 构建(跨平台:Windows MSVC / Linux g++·clang)
#   从**源码**编译(basics / muparserx / CEC2013 / 领域层 + main.cpp),不依赖预编 .lib。
#   用法(在 build/cmake/ 下,或指定 -S):
#     cmake -S . -B out
#     cmake --build out --config Release      # → 项目根 ecflow(.exe)(与 src/ 同级)
#   ★ 运行时:exe 按**当前工作目录(CWD)**解析 `_pdata/`、`config/`。在项目根运行可用相对/短路径;
#     从别处运行须给问题/配置传绝对路径。
#   MSVC 用 /MT 静态运行时 → exe 完全自包含(仅依赖系统 ADVAPI32/KERNEL32)。
cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)
project(ecflow LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# 本 CMakeLists 在 build/cmake/ 下 → 项目根为上两级
get_filename_component(ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

file(GLOB BASICS_SRC "${ROOT}/src/basics/*.cpp")
set(DOMAIN_SRC
    "${ROOT}/src/problem/variable/variable.cpp"
    "${ROOT}/src/problem/objective/objective.cpp"
    "${ROOT}/src/solution/solution.cpp"
    "${ROOT}/src/solution/solution-decoder.cpp"
    "${ROOT}/src/calculator/func-calculator.cpp"
    "${ROOT}/src/calculator/tree-calculator.cpp"
    "${ROOT}/src/exprTree/ExprTree.cpp")
file(GLOB_RECURSE MUPARSERX_SRC "${ROOT}/thirdparty/muparserx/src/*.cpp")
file(GLOB CEC_SRC "${ROOT}/_pdata/numerical/CEC2013_N/*.cpp" "${ROOT}/_pdata/numerical/CEC2013_L/*.cpp")

add_executable(ecflow
    "${ROOT}/src/main.cpp"
    ${BASICS_SRC} ${DOMAIN_SRC} ${MUPARSERX_SRC} ${CEC_SRC})

# 产物落项目根(与 _pdata/config 同级,便于 CWD-相对路径运行);中间产物在 out/。
set_target_properties(ecflow PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY         "${ROOT}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${ROOT}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${ROOT}")

target_include_directories(ecflow PRIVATE
    "${ROOT}/src"
    "${ROOT}/src/basics" "${ROOT}/src/calculator" "${ROOT}/src/exprTree" "${ROOT}/src/solution"
    "${ROOT}/src/logger" "${ROOT}/src/cp" "${ROOT}/src/distribution"
    "${ROOT}/src/problem/core" "${ROOT}/src/problem/define" "${ROOT}/src/problem/manager"
    "${ROOT}/src/problem/variable" "${ROOT}/src/problem/objective" "${ROOT}/src/problem/constrain"
    "${ROOT}/src/problem/heuristic" "${ROOT}/src/problem/template"
    "${ROOT}/src/optimizer/core" "${ROOT}/src/optimizer/individual" "${ROOT}/src/optimizer/solution-op"
    "${ROOT}/src/optimizer/population" "${ROOT}/src/optimizer/subpopulation-constructer"
    "${ROOT}/src/optimizer/subpopulation-topology" "${ROOT}/src/optimizer/subpopulation-manager"
    "${ROOT}/src/optimizer/strategy" "${ROOT}/src/optimizer/topology" "${ROOT}/src/optimizer/generator"
    "${ROOT}/src/optimizer/selector" "${ROOT}/src/optimizer/evaluator" "${ROOT}/src/optimizer/terminator"
    "${ROOT}/src/optimizer/archive" "${ROOT}/src/optimizer/orchestrate" "${ROOT}/src/optimizer/config"
    "${ROOT}/src/multiobjective" "${ROOT}/src/analyzer" "${ROOT}/src/analyzer/out"
    "${ROOT}/_pdata/numerical/CEC2013_N" "${ROOT}/_pdata/numerical/CEC2013_L"
    "${ROOT}/thirdparty/muparserx/include" "${ROOT}/thirdparty/muparserx/src/parser"
    "${ROOT}/thirdparty/sortlib/include" "${ROOT}/thirdparty/randlib/include"
    "${ROOT}/thirdparty/timelib/include" "${ROOT}/thirdparty/doublelib/include"
    "${ROOT}/thirdparty/distancelib/include" "${ROOT}/thirdparty/filelib/include"
    "${ROOT}/thirdparty/statslib/include"
    "${ROOT}/thirdparty/stringlib/src/stringlib/stringlib"
    "${ROOT}/thirdparty/metriclib/src/metriclib/metriclib")

if(MSVC)
    target_compile_options(ecflow PRIVATE /utf-8 /EHsc /W1 /wd4828)
    target_link_libraries(ecflow PRIVATE advapi32)
else()
    target_compile_options(ecflow PRIVATE -finput-charset=UTF-8 -w)
endif()
