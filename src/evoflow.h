//------------------------Description------------------------
// EvoFlow 库统一入口:使用 EvoFlow 只需 include 本文件。汇集问题定义(Problem/模板)与优化器装配(OptimizerBuilder)。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include "problem.h"
#include "optimizer-builder.h"
#include "config-setter.h"   // ConfigBuilder fluent API(R1 修复:达成"一个 include 即用")
