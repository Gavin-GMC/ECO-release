//------------------------Description------------------------
// 常用子种群邻域拓扑(5 种)→ Registry<SubpopulationTopology>:
//   Connected(全连接,除己外皆邻)/ Star(星形,0 号为中心)/ Toroidal(环形,前后各一)/
//   Preswarm(前序,i 邻接所有 j<i)/ NoConnect(孤立,无邻)。
//-------------------------Reference-------------------------
//   原工厂仅启用此 5 种;Given/Cell/Random 原即注释(WIP,Given 构造有悬空指针 bug)→ 跳过,记未来项 STOPO-MORE。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "subpopulation-topology.h"
#include "registry.h"

namespace ECFlow
{
    // 全连接:每子群邻接除自身外的全部
    class ConnectedTopology : public SubpopulationTopology
    {
    public:
        ConnectedTopology() : SubpopulationTopology() {}
        ~ConnectedTopology() {}

        void build(Subpopulation** subpopulations, int& swarm_number) override
        {
            int counter;
            for (int i = 0; i < swarm_number; i++)
            {
                counter = 0;
                for (int j = 0; j < swarm_number; j++)
                    if (i != j)
                        neighborhoods[i][counter++] = subpopulations[j];
            }
        }

        void ini(Subpopulation** subpopulations, int& swarm_number) override
        {
            delete[] neighborhoods;
            neighborhoods = new NeighborSet[swarm_number];
            for (int i = 0; i < swarm_number; i++)
                neighborhoods[i].setSize(swarm_number - 1);
            build(subpopulations, swarm_number);
        }
    };

    // 星形:0 号为中心,与其余全部互为邻居;其余仅邻接中心
    class StarTopology : public SubpopulationTopology
    {
    public:
        StarTopology() : SubpopulationTopology() {}
        ~StarTopology() {}

        void build(Subpopulation** subpopulations, int& swarm_number) override
        {
            // 迁移修复:原 `neighborhoods[0][i]`(i=1..n-1)越界——中心邻居集尺寸 n-1(下标 0..n-2),
            //   写下标 n-1 越界且下标 0 未写 → 堆损坏。改 `[0][i-1]`,填满 0..n-2。
            for (int i = 1; i < swarm_number; i++)
            {
                neighborhoods[0][i - 1] = subpopulations[i];
                neighborhoods[i][0] = subpopulations[0];
            }
        }

        void ini(Subpopulation** subpopulations, int& swarm_number) override
        {
            delete[] neighborhoods;
            neighborhoods = new NeighborSet[swarm_number];
            neighborhoods[0].setSize(swarm_number - 1);
            for (int i = 1; i < swarm_number; i++)
                neighborhoods[i].setSize(1);
            build(subpopulations, swarm_number);
        }
    };

    // 环形:每子群前后各一个邻居(首尾相接)
    class ToroidalTopology final : public SubpopulationTopology
    {
    public:
        ToroidalTopology() : SubpopulationTopology() {}
        ~ToroidalTopology() {}

        void build(Subpopulation** subpopulations, int& swarm_number) override
        {
            neighborhoods[0][0] = subpopulations[swarm_number - 1];
            for (int i = 1; i < swarm_number; i++)
                neighborhoods[i][0] = subpopulations[i - 1];

            for (int i = 1; i < swarm_number; i++)
                neighborhoods[i - 1][1] = subpopulations[i];
            neighborhoods[swarm_number - 1][1] = subpopulations[0];
        }

        void ini(Subpopulation** subpopulations, int& swarm_number) override
        {
            delete[] neighborhoods;
            neighborhoods = new NeighborSet[swarm_number];
            for (int i = 0; i < swarm_number; i++)
                neighborhoods[i].setSize(2);
            build(subpopulations, swarm_number);
        }
    };

    // 前序:子群 i 邻接所有 j<i(下三角)
    class PreswarmTopology : public SubpopulationTopology
    {
    public:
        PreswarmTopology() : SubpopulationTopology() {}
        ~PreswarmTopology() {}

        void build(Subpopulation** subpopulations, int& swarm_number) override
        {
            for (int i = 0; i < swarm_number; i++)
                for (int j = 0; j < i; j++)
                    neighborhoods[i][j] = subpopulations[j];
        }

        void ini(Subpopulation** subpopulations, int& swarm_number) override
        {
            delete[] neighborhoods;
            neighborhoods = new NeighborSet[swarm_number];
            for (int i = 0; i < swarm_number; i++)
                neighborhoods[i].setSize(i);
            build(subpopulations, swarm_number);
        }
    };

    // 孤立:各子群无邻居
    class NoConnectTopology : public SubpopulationTopology
    {
    public:
        NoConnectTopology() : SubpopulationTopology() {}
        ~NoConnectTopology() {}

        void build(Subpopulation** subpopulations, int& swarm_number) override {}

        void ini(Subpopulation** subpopulations, int& swarm_number) override
        {
            delete[] neighborhoods;
            neighborhoods = new NeighborSet[swarm_number];
            for (int i = 0; i < swarm_number; i++)
                neighborhoods[i].setSize(0);
        }
    };

    // —— 注册进 Registry<SubpopulationTopology>(ModuleType::T_subswarmtopology) ——
    inline Registry<SubpopulationTopology>::Entry connectedTopologyEntry()
    {
        return { "Connected", ModuleType::T_subswarmtopology, ParameterTemplate{}, sizeof(ConnectedTopology),
            [](const double*) -> SubpopulationTopology* { return new ConnectedTopology(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    inline Registry<SubpopulationTopology>::Entry starTopologyEntry()
    {
        return { "Star", ModuleType::T_subswarmtopology, ParameterTemplate{}, sizeof(StarTopology),
            [](const double*) -> SubpopulationTopology* { return new StarTopology(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    inline Registry<SubpopulationTopology>::Entry toroidalTopologyEntry()
    {
        return { "Toroidal", ModuleType::T_subswarmtopology, ParameterTemplate{}, sizeof(ToroidalTopology),
            [](const double*) -> SubpopulationTopology* { return new ToroidalTopology(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    inline Registry<SubpopulationTopology>::Entry preswarmTopologyEntry()
    {
        return { "Preswarm", ModuleType::T_subswarmtopology, ParameterTemplate{}, sizeof(PreswarmTopology),
            [](const double*) -> SubpopulationTopology* { return new PreswarmTopology(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    inline Registry<SubpopulationTopology>::Entry noConnectTopologyEntry()
    {
        return { "NoConnect", ModuleType::T_subswarmtopology, ParameterTemplate{}, sizeof(NoConnectTopology),
            [](const double*) -> SubpopulationTopology* { return new NoConnectTopology(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }

    ECFLOW_REGISTER(stopo_connected, SubpopulationTopology, connectedTopologyEntry());
    ECFLOW_REGISTER(stopo_star,      SubpopulationTopology, starTopologyEntry());
    ECFLOW_REGISTER(stopo_toroidal,  SubpopulationTopology, toroidalTopologyEntry());
    ECFLOW_REGISTER(stopo_preswarm,  SubpopulationTopology, preswarmTopologyEntry());
    ECFLOW_REGISTER(stopo_noconnect, SubpopulationTopology, noConnectTopologyEntry());
}
