//------------------------Description------------------------
// 学习策略聚合头:一次性引入基类 + 全部具体策略(触发各自向 Registry<LearningStrategy> 自注册)。
//   供上层(OffspringGenerator)单头引入整族。

// SPDX-License-Identifier: BSD-3-Clause

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
#include "lstrategy-sinecosine.h"   // SineCosineOscillation(SCA):单向导正弦余弦振荡,连续域
#include "lstrategy-arithmetic.h"   // ArithmeticOperatorUpdate(AOA):单向导四则运算 + MOA/MOP 进度门控,连续域
#include "lstrategy-reptile.h"      // ReptileEncircling(RSA):最优+随机+中心,4 相位包围/狩猎,连续域
#include "lstrategy-goldenjackal.h" // GoldenJackalPincer(GJO):雌雄双豺(TopRanked(2))钳形 + 能量衰减 + Levy,连续域
#include "lstrategy-tunicate.h"     // TunicateJetSwarm(TSA):单向导喷射推进 + 群聚阻尼,无状态无进度,连续域
#include "lstrategy-mpa.h"          // MarinePredatorMotion(MPA):Elite+两随机,3 相位 Brownian/Levy + FADs,连续域
#include "lstrategy-aquila.h"       // AquilaHunt(AO):最优+随机+中心,4 method 相位切换 + Levy + 螺旋,连续域
#include "lstrategy-sma.h"          // SlimeMouldOscillation(SMA):最优+双随机,适应度权重 W(只读表)+ 振荡,连续域
#include "lstrategy-eo.h"           // EquilibriumUpdate(EO):前 4 最优平衡池 + 均值,指数质量平衡,连续域
#include "lstrategy-avoa.h"         // VultureForaging(AVOA):两领头秃鹫,饱食率 F 分相位多分支 + 螺旋 + Levy,连续域
#include "lstrategy-bwo.h"          // BelugaWhalePhases(BWO):最优+随机,游动(跨维)/捕食(Levy)/鲸落 三相,连续域
#include "lstrategy-info.h"         // WeightedMeanUpdate + WeightedMeanLocalSearch(INFO):加权均值向量,两段可组合,连续域
#include "lstrategy-ssa.h"          // SalpChainUpdate(SSA):领导跟食物+跟随者链式平均,连续域
#include "lstrategy-goa.h"          // GrasshopperSwarm(GOA):全群两两社会力 O(n²) + 目标吸引,连续域
#include "lstrategy-dbo.h"          // DungBeetleRoles(DBO):四角色(滚球/繁殖/觅食/偷窃)+ prevpos 特性,连续域
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
