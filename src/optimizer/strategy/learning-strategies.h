//------------------------Description------------------------
// 学习策略聚合头:一次性引入基类 + 全部具体策略(触发各自向 Registry<LearningStrategy> 自注册)。
//   供上层(OffspringGenerator)单头引入整族。
//-------------------------Reference-------------------------
// 对应原 lstrategy-factory.h 的"汇总"作用(注册表模式下由各文件自注册,本头仅聚合 include)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "learning-strategy.h"
#include "lstrategy-pso.h"
#include "lstrategy-bpso.h"
#include "lstrategy-setpso.h"
#include "lstrategy-eda.h"
#include "lstrategy-es.h"
#include "lstrategy-cmaes.h"
#include "lstrategy-tabu.h"
#include "lstrategy-aco.h"
#include "lstrategy-gwo.h"
#include "lstrategy-woa.h"
#include "lstrategy-immune.h"
#include "lstrategy-firework.h"
#include "lstrategy-abc.h"
#include "lstrategy-jaya.h"
#include "lstrategy-ig.h"
#include "lstrategy-wwo.h"
#include "lstrategy-pathrelink.h"
#include "lstrategy-alns.h"
#include "lstrategy-mutation.h"
#include "lstrategy-crossover.h"
#include "lstrategy-crossover-basic.h"
#include "lstrategy-crossover-sequence.h"
