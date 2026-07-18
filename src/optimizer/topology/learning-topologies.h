//------------------------Description------------------------
// 学习拓扑聚合头:一次性引入 LearningGraph + 基类 + 全部具体拓扑(触发各自向 Registry<LearningTopology> 自注册)。
//   供上层(如 OffspringGenerator)单头引入整族。
//-------------------------Reference-------------------------
// 对应原 ltopology-factory.h 的"汇总"作用(注册表模式下由各文件自注册,本头仅聚合 include)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "learning-graph.h"
#include "learning-topology.h"
#include "ltopology-pso.h"
#include "ltopology-de.h"
#include "ltopology-elite.h"
#include "ltopology-isolate.h"
#include "ltopology-cso.h"
#include "ltopology-llso.h"
#include "ltopology-sdlso.h"
#include "ltopology-continue.h"
#include "ltopology-ga.h"
#include "ltopology-ranked.h"
#include "ltopology-leader-random.h"
#include "ltopology-clonal.h"
#include "ltopology-firework.h"
#include "ltopology-neighborhood.h"
#include "ltopology-abc.h"
#include "ltopology-wwo.h"
