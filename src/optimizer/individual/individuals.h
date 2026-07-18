//------------------------Description------------------------
// 个体汇总头:INDIV-COMPOSE 收官(S5)后只剩基类 Individual + 特性(velocity/pbest/sigma/whalestate/
//   fireworkstate/setvelocity 等经装配期推断挂载),旧子类(Particle/Pbest/Step/Whale/Firework/SetParticle)已删。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
//-----------------------------------------------------------

#pragma once
#include "individual.h"            // 基类 + 特性袋 + Registry<Feature> 基础特性(vector/scalar/solution/pbest)
#include "feature-setvelocity.h"   // setvelocity 特性(集合式 PSO;包裹 SetVelocity)
