//------------------------Description------------------------
// 常用子种群管理器(4 种)→ Registry<SubpopulationManager>:
//   NoInteraction(子群互不干预)/ RebuildTopology(每代重构子群+重建拓扑)/
//   ImmigrantModel(向邻居迁入本群最优个体)/ SingleSwarm(单子群,自建 Fixed+NoConnect)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "subpopulation-manager.h"
#include "subpopulation-constructer.h"
#include "subpopulation-topology-basic.h"
#include "registry.h"
#include "ecflow-rand.h"
#include "ecflow-constant.h"

namespace ECFlow
{
    // 子群互不干预(支持多子群:各群独立演化,群间无动作 —— 与 SingleSwarm 语义不同,见文件头注)
    class NoInteraction final : public SubpopulationManager
    {
    public:
        NoInteraction(SubpopulationConstructer* builder, SubpopulationTopology* model)
            : SubpopulationManager(builder, model) {}
        ~NoInteraction() {}
        // 无群间动作:两个钩子均不覆写(终止器协作由基类 runEpoch 包办)
    };

    // 交互方式 = 每轮驱动构建器**重构子群**(个体重新划分)。
    //   拓扑重建不在此处:它是交互的**后果**,由基类 runEpoch 统一处理(见 SubpopulationManager 头注)。
    class RebuildTopology final : public SubpopulationManager
    {
    protected:
        void interact() override
        {
            _builder->build(_subpopulations, _swarm_number);
        }

    public:
        RebuildTopology(SubpopulationConstructer* builder, SubpopulationTopology* model)
            : SubpopulationManager(builder, model) {}
        ~RebuildTopology() {}
    };

    // 迁徙:把本群最优个体迁入其邻居(random_replace:替换随机个体 / 否则替换最差个体)
    class ImmigrantModel final : public SubpopulationManager
    {
    private:
        bool   _random_replace;
        double _immigrant_rate;    // 忠实保留:存而未用(原 update 未消费)
        int    _immigrant_number;  // 同上

    protected:
        void interact() override
        {
            Solution* optimal;
            int optimal_size = 0;

            if (_random_replace)
            {
                for (int i = 0; i < _swarm_number; i++)
                {
                    _subpopulations[i]->getBest(optimal, optimal_size);
                    for (int j = 0; j < _model->neighborhoods[i].getSize(); j++)
                        _model->neighborhoods[i][j]->replaceIndividualR(optimal + get_int(0, optimal_size - 1));
                }
            }
            else
            {
                for (int i = 0; i < _swarm_number; i++)
                {
                    _subpopulations[i]->getBest(optimal, optimal_size);
                    for (int j = 0; j < _model->neighborhoods[i].getSize(); j++)
                        _model->neighborhoods[i][j]->replaceIndividualW(optimal + get_int(0, optimal_size - 1));
                }
            }
        }

    public:
        ImmigrantModel(SubpopulationConstructer* builder, SubpopulationTopology* model,
                       bool random_replace, double immigrant_rate = EMPTYVALUE, int immigrant_number = 1)
            : SubpopulationManager(builder, model)
        {
            _random_replace = random_replace;
            _immigrant_rate = immigrant_rate;
            _immigrant_number = immigrant_number;
        }
        ~ImmigrantModel() {}
    };

    // 单子群(最常用):构造即自建 Fixed 构建器 + NoConnect 拓扑
    class SingleSwarm final : public SubpopulationManager
    {
    public:
        SingleSwarm() : SubpopulationManager(nullptr, nullptr)
        {
            _builder = new FixedConstructer();
            _model = new NoConnectTopology();
        }
        ~SingleSwarm() {}
        // 单子群无群间结构:两个钩子均不覆写。
        //   另外三个 manager 一概没写 → 全局预算在它们下面完全失效。现已上移至基类 runEpoch 统一包办。
    };

    // —— 注册进 Registry<SubpopulationManager>(ModuleType::T_subswarmmanager) ——
    inline Registry<SubpopulationManager>::Entry noInteractionManagerEntry()
    {
        return { "NoInteraction", ModuleType::T_subswarmmanager, ParameterTemplate{}, sizeof(NoInteraction),
            [](const double*) -> SubpopulationManager* { return new NoInteraction(nullptr, nullptr); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    inline Registry<SubpopulationManager>::Entry rebuildTopologyManagerEntry()
    {
        return { "RebuildTopology", ModuleType::T_subswarmmanager, ParameterTemplate{}, sizeof(RebuildTopology),
            [](const double*) -> SubpopulationManager* { return new RebuildTopology(nullptr, nullptr); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    inline Registry<SubpopulationManager>::Entry immigrantManagerEntry()
    {
        return { "Immigrant", ModuleType::T_subswarmmanager,
            ParameterTemplate{ { {"random_replace", ParamKind::Enum, 0, 1, false, 0, 1} } }, sizeof(ImmigrantModel),
            [](const double* p) -> SubpopulationManager* {
                return new ImmigrantModel(nullptr, nullptr, p ? p[0] != 0 : false); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    inline Registry<SubpopulationManager>::Entry singleSwarmManagerEntry()
    {
        return { "Single", ModuleType::T_subswarmmanager, ParameterTemplate{}, sizeof(SingleSwarm),
            [](const double*) -> SubpopulationManager* { return new SingleSwarm(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }

    ECFLOW_REGISTER(smgr_nointeraction, SubpopulationManager, noInteractionManagerEntry());
    ECFLOW_REGISTER(smgr_rebuild,       SubpopulationManager, rebuildTopologyManagerEntry());
    ECFLOW_REGISTER(smgr_immigrant,     SubpopulationManager, immigrantManagerEntry());
    ECFLOW_REGISTER(smgr_single,        SubpopulationManager, singleSwarmManagerEntry());
}
