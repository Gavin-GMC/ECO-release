//------------------------Description------------------------
// 优化器可注册/替换的全部组件类别枚举(ModuleType)——即自注册表的"类别"维度。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once

namespace ECFlow
{
    enum class ModuleType {
        T_default,             // 占位
        T_individual,          // 个体
        T_feature,             // 个体特性(INDIV-COMPOSE:特性组件)
        T_learnstrategy,       // 学习策略
        T_learntopology,       // 学习拓扑
        T_offspringgenerator,  // 子代生成器
        T_selector,            // 选择器(环境选择 / 种群更新)
        T_evaluator,           // 评估器
        T_Repair,              // 修复
        T_subswarbuilder,      // 子群构建器
        T_subswarmtopology,    // 子群拓扑
        T_subswarmmanager,     // 子群管理器
        T_bestarchive,         // 最优档案
        T_end                  // 哨兵
    };
}
