//------------------------Description------------------------
// SubpopulationTopology:子种群间"邻域拓扑"模型基类。持 NeighborSet 数组(每子群一份邻居集),
//   `ini`(按 swarm_number 分配各邻居集尺寸)+ `build`(填入具体邻居)+ `update`(动态拓扑每代刷新)。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include "subpopulation.h"
#include "neighbor-set.h"
#include "parameter-template.h"

namespace EvoFlow
{
    class SubpopulationTopology
    {
    public:
        NeighborSet* neighborhoods;

        SubpopulationTopology()
        {
            neighborhoods = nullptr;
        }

        virtual ~SubpopulationTopology()
        {
            delete[] neighborhoods;
        }

        static ParameterTemplate getParameterTemplate() { return ParameterTemplate{}; }

        // 填入具体邻居(neighborhoods 已按尺寸分配)。
        //   由 SubpopulationManager::runEpoch **每轮驱动** → 随机/动态拓扑在此重新抽样即获得"每轮刷新",
        //   静态拓扑填回相同内容(空操作级开销)。故"是否动态"由本方法**是否随机**自身决定,无需声明位。
        virtual void build(Subpopulation** subpopulations, int& swarm_number) = 0;

        // 分配各子群邻居集尺寸 + build。仅在子群**数目**变化时需要(ini 期 / 交互改了数目)。
        virtual void ini(Subpopulation** subpopulations, int& swarm_number) = 0;

        // v1.4.8 删 `virtual void update(...)`(原注"动态拓扑每代刷新"):**语义与 build 重叠**
        //   ——"数目未变时重新填邻居"正是 build 本身;且它在本仓与 ECFC原代码/稳定版**三处皆零调用、
        //   五个拓扑无一覆写**,是又一个"声明了职责却无人驱动"的扩展点(与终止器协作同一失败模式)。
        //   现由 runEpoch 每轮无条件驱动 build 兑现该意图,不再保留第二套机制。
    };
}
