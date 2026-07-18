//------------------------Description------------------------
// ECFlow 库统一入口:使用 ECFlow 只需 include 本文件。汇集问题定义(Problem/模板)与优化器装配(OptimizerBuilder)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "problem.h"
#include "optimizer-builder.h"
#include "config-setter.h"   // ConfigBuilder fluent API(R1 修复:达成"一个 include 即用")
