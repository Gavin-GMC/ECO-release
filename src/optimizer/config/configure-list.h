//------------------------Description------------------------
// 配置结构(最小集,v3.4.c):描述一条 workflow 与一个子种群的装配清单,供 OptimizerBuilder 消费。
//   顶层 ConfigureList(cooperation/logger/name/g_archive)与配置文件编解码随 v3.5/v3.6 增量长出。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <string>
#include <vector>
#include "module-type.h"

namespace ECFlow
{
    // 单个组件配置:c_type 决定 Registry + wrapper;tag 行为名;para 变长参数
    struct ComponentConfig
    {
        ModuleType          c_type = ModuleType::T_default;
        std::string         tag;
        std::vector<double> para;
        // INDIV-COMPOSE:特性共享通道覆盖 (role → channel);默认空=各特性按 scope(私有/独占/共有)
        std::vector<std::pair<std::string, std::string>> feature_channels;

        ComponentConfig() = default;
        ComponentConfig(ModuleType type, const std::string& t, std::vector<double> p = {})
            : c_type(type), tag(t), para(std::move(p)) {}
    };

    // workflow 配置:tag + initializer(tag + para) + 有序组件表(Set*** 应列在消费它的 Run*** 之前)
    struct WorkflowConfig
    {
        std::string                  tag;
        std::string                  ini_tag = "Random";   // Random/Greedy/No/R_G/Distribution
        std::vector<double>          ini_para;             // R_G:[概率];Distribution:[分布型]
        std::vector<ComponentConfig> components;
    };

    // 子种群配置
    //   行为全由装配期推断并挂载的特性决定,`Registry<Individual>`(按名造子类)亦已移除。
    //   该字段遂成**只写不读的死字段**:传任何字符串(含拼错的)都不影响所造个体,是个会骗人的接口。
    struct SubpopulationConfig
    {
        std::string         tag;
        int                 size = 1;
        std::string         workflow_tag;
        int                 terminate_conditions[3] = { -1, -1, -1 };   // FES, Convergence, Time
        std::string         archive_tag = "Basic";                      // Registry<BestArchive> tag
        std::vector<double>  archive_para;
    };

    // 子群协作配置:管理器 + 构建器 + 邻域拓扑(三者经各自 Registry 建,由 Pbuild 装配注入)
    struct CooperationConfig
    {
        std::string         manager_tag = "Single";      // Registry<SubpopulationManager>
        std::vector<double> manager_para;
        std::string         constructer_tag = "Fixed";   // Registry<SubpopulationConstructer>
        std::vector<double> constructer_para;
        std::string         topology_tag = "NoConnect";  // Registry<SubpopulationTopology>
        std::vector<double> topology_para;
    };

    // 顶层优化器配置(一个 optimizer 一个 Population,故 population 层字段直接内联,无中间 PopulationConfig 聚合):
    //   name/tag/终止条件/日志开关 + 协作模型 + N 子群 + 全局档案。buildOptimizer 消费它。
    struct OptimizerConfig
    {
        std::string                      name = "optimizer";
        std::string                      tag;                 // 运行标识(空→buildOptimizer 用时间戳)
        int                              terminate_conditions[3] = { -1, -1, -1 };   // FES/Convergence/Time
        bool                             logger_full_result = false;
        bool                             logger_process = false;
        bool                             logger_full_process = false;
        bool                             logger_console_echo = false;   // v1.4.9:进度回显到控制台(CLI -v);不落 .cfg,仅运行期

        CooperationConfig                cooperation;
        std::vector<SubpopulationConfig> subpopulations;
        std::string                      g_archive_tag = "Basic";
        std::vector<double>              g_archive_para;
    };
}
