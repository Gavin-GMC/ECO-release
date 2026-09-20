//------------------------Description------------------------
// 学习拓扑聚合头:一次性引入 LearningGraph + 基类 + 全部具体拓扑(触发各自向 Registry<LearningTopology> 自注册)。
//   供上层(如 OffspringGenerator)单头引入整族。

// SPDX-License-Identifier: BSD-3-Clause

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
#include "ltopology-leaderrandomcenter.h"   // LeaderRandomCenter:最优+随机+逐维种群均值(供 RSA/ReptileEncircling)
#include "ltopology-leadertworandom.h"      // LeaderTwoRandom:最优+两随机(供 MPA/MarinePredatorMotion 的 FADs 差分)
#include "ltopology-rankextremestrirandom.h" // RankExtremesTriRandom / ContinueRankExtremesTriRandom:best/better/worst+3排序随机(供 INFO 加权均值)
#include "ltopology-salpchain.h"            // SalpChain:食物+链上前一只(供 SSA/SalpChainUpdate 链式)
#include "ltopology-dungbeetlerefs.h"       // DungBeetleRefs:全局最优+当代最优+当代最差(供 DBO/DungBeetleRoles)
#include "ltopology-clonal.h"
#include "ltopology-firework.h"
#include "ltopology-neighborhood.h"
#include "ltopology-abc.h"
#include "ltopology-wwo.h"
