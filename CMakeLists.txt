# EvoFlow C++17 executable build for Windows and Linux.
#   一份 CMakeLists 保留 Windows(MSVC) / Linux(g++/clang) 源码路径。从**源码**编译(不依赖 MSVC 预编 .lib),
#   Windows x64 为正式发布平台；Linux C++ 核心与 CLI 已在 2.7.5 的 Ubuntu 24.04 WSL2 环境完成源码构建与运行验证。
#   把 basics / muparserx / 领域层 .cpp + src/main.cpp 链成可执行文件 `evoflow`。
#   用法:  cmake -B obj-cmake-release                    # 生成(Win 默认 MSVC;Linux 默认 make/ninja)
#          cmake --build obj-cmake-release --config Release  # 编译 → 项目根 evoflow(.exe)(与 src/ 同级)
#   ★ 运行时路径:exe 按**当前工作目录(CWD)**解析 `_pdata/`、`config/`(非 exe 自身位置)。
#     - 在项目根运行:可用相对/短路径,如 `./evoflow run _pdata/tsp/sample.tsp config/ACS.cfg`。
#     - 从别处运行:须给问题/配置传**绝对路径**。
#   MSVC 用 /MT 静态运行时 → exe 完全自包含(仅依赖系统 ADVAPI32/KERNEL32,无需 VC++ 可再发行运行时)。
#   中间产物在调用方指定的 obj-* 目录；最终 exe 默认落项目根。
cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)   # 允许用 CMAKE_MSVC_RUNTIME_LIBRARY 选择静态/动态运行时
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/src/evoflow-version.h"
     EVOFLOW_VERSION_LINE REGEX "^#define EVOFLOW_VERSION_STRING \"[0-9]+\\.[0-9]+\\.[0-9]+(\\.[0-9]+)?\"$")
if(NOT EVOFLOW_VERSION_LINE)
    message(FATAL_ERROR "Cannot read EVOFLOW_VERSION_STRING from src/evoflow-version.h")
endif()
string(REGEX REPLACE
       "^#define EVOFLOW_VERSION_STRING \"([0-9]+\\.[0-9]+\\.[0-9]+(\\.[0-9]+)?)\"$" "\\1"
       EVOFLOW_PRODUCT_VERSION "${EVOFLOW_VERSION_LINE}")
project(evoflow VERSION ${EVOFLOW_PRODUCT_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# MSVC:静态链接运行时(/MT,Debug 用 /MTd)→ exe 完全自包含,不需 VC++ 可再发行运行时 DLL。
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(EVOFLOW_RUNTIME_OUTPUT_DIRECTORY "${ROOT}" CACHE PATH
    "Directory for the evoflow executable")

# ---- 源文件集合 ----
# libbasics 等价:平台基础
file(GLOB BASICS_SRC   "${ROOT}/src/basics/*.cpp")
# EvoFlow 可执行程序使用的领域层编译单元
set(DOMAIN_SRC
    "${ROOT}/src/problem/variable/variable.cpp"
    "${ROOT}/src/problem/objective/objective.cpp"
    "${ROOT}/src/solution/solution.cpp"
    "${ROOT}/src/solution/solution-decoder.cpp"
    "${ROOT}/src/calculator/func-calculator.cpp"
    "${ROOT}/src/calculator/tree-calculator.cpp"
    "${ROOT}/src/exprTree/ExprTree.cpp"
    "${ROOT}/src/analyzer/out/out-png.cpp"
    "${ROOT}/src/app/app-facade.cpp"
    "${ROOT}/src/app/component-catalog.cpp"
    "${ROOT}/src/app/config-editor.cpp"
    "${ROOT}/src/app/config-validator.cpp"
    "${ROOT}/src/app/llm-client.cpp"
    "${ROOT}/src/app/llm-config-recommender.cpp"
    "${ROOT}/src/app/llm-problem-catalog.cpp"
    "${ROOT}/src/app/llm-problem-builder.cpp"
    "${ROOT}/src/app/run-reporter.cpp")   # CLI/UI shared facade; PNG implementation is compiled in one translation unit.

set(UI_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
if(WIN32)
    find_program(POWERSHELL_EXECUTABLE powershell REQUIRED)
    set(UI_ASSETS_HEADER "${UI_GENERATED_DIR}/ui-assets.generated.h")
    add_custom_command(
        OUTPUT "${UI_ASSETS_HEADER}"
        COMMAND "${POWERSHELL_EXECUTABLE}" -NoProfile -ExecutionPolicy Bypass
                -File "${ROOT}/src/ui/embed-assets.ps1" -Output "${UI_ASSETS_HEADER}"
        DEPENDS
            "${ROOT}/src/ui/embed-assets.ps1"
            "${ROOT}/src/ui/assets/index.html"
            "${ROOT}/src/ui/assets/app.css"
            "${ROOT}/src/ui/assets/i18n.js"
            "${ROOT}/src/ui/assets/app.js"
            "${ROOT}/src/ui/assets/llm-settings.js"
            "${ROOT}/src/ui/assets/problem-builder.js"
            "${ROOT}/src/ui/assets/config-editor.js"
            "${ROOT}/src/ui/assets/config-recommender.js"
            "${ROOT}/src/ui/assets/flow-editor.js"
            "${ROOT}/src/ui/assets/report-viewer.js"
        VERBATIM)
    list(APPEND DOMAIN_SRC "${UI_ASSETS_HEADER}")
endif()
list(APPEND DOMAIN_SRC
    "${ROOT}/src/ui/http-server.cpp"
    "${ROOT}/src/ui/ui-app.cpp")
# libmuparserx 等价:表达式引擎
file(GLOB_RECURSE MUPARSERX_SRC "${ROOT}/thirdparty/muparserx/src/*.cpp")
add_executable(evoflow
    "${ROOT}/src/main.cpp"
    ${BASICS_SRC} ${DOMAIN_SRC} ${MUPARSERX_SRC})

# 产物 evoflow(.exe) 默认落项目根(与 src/、_pdata/、config/ 同级)——因 exe 按 CWD 解析 _pdata/config,
#   放这里并从此目录运行,短名/相对路径才对得上。中间产物放在调用方指定的 obj-* 目录。
#   MSVC 多配置默认会加 Release/ Debug/ 子目录,故 per-config 均显式指向可覆盖的输出目录。
set_target_properties(evoflow PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY         "${EVOFLOW_RUNTIME_OUTPUT_DIRECTORY}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${EVOFLOW_RUNTIME_OUTPUT_DIRECTORY}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${EVOFLOW_RUNTIME_OUTPUT_DIRECTORY}")

# ---- EvoFlow 可执行程序的头文件搜索路径 ----
target_include_directories(evoflow PRIVATE
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
    "${ROOT}/thirdparty/muparserx/include" "${ROOT}/thirdparty/muparserx/src/parser"
    "${ROOT}/thirdparty/sortlib/include" "${ROOT}/thirdparty/randlib/include"
    "${ROOT}/thirdparty/timelib/include" "${ROOT}/thirdparty/doublelib/include"
    "${ROOT}/thirdparty/distancelib/include" "${ROOT}/thirdparty/filelib/include"
    "${ROOT}/thirdparty/statslib/include"
    "${ROOT}/thirdparty/stringlib/src/stringlib/stringlib"
    "${ROOT}/thirdparty/metriclib/src/metriclib/metriclib"
    "${ROOT}/thirdparty/stb"
    "${ROOT}/src/app"
    "${ROOT}/src/ui"
    "${UI_GENERATED_DIR}")

find_package(Threads REQUIRED)
target_link_libraries(evoflow PRIVATE Threads::Threads)

# ---- 编译器差异 ----
if(MSVC)
    target_compile_options(evoflow PRIVATE /utf-8 /EHsc /bigobj /W1 /wd4828)   # /bigobj:evoflow.h 聚合算子/拓扑注册增多→main.cpp 节数越 COFF 上限(C1128)
else()
    # g++/clang:源码为 UTF-8;thirdparty(muparserx)告警较多,降噪不影响 src。
    target_compile_options(evoflow PRIVATE -finput-charset=UTF-8 -w)
endif()
if(WIN32)
    target_link_libraries(evoflow PRIVATE advapi32 ws2_32 shell32 bcrypt winhttp)
endif()
