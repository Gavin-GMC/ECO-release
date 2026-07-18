//------------------------Description------------------------
// 分层学习拓扑 LevelBasedLearningTopology:按适应度把种群分为若干层级,较低层个体从"更高的两个层级"
//   各随机选一个优势个体作为学习对象;最高层(最优)不学习。
//-------------------------Reference-------------------------
// [1] Q. Yang, W.-N. Chen, J. D. Deng, Y. Li, T. Gu, and J. Zhang, "A Level-Based Learning Swarm
//     Optimizer for Large-Scale Optimization," IEEE Transactions on Evolutionary Computation,
//     vol. 22, no. 4, pp. 578-594, Aug. 2018, doi: 10.1109/TEVC.2017.2743016.
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <cmath>
#include <utility>
#include "learning-topology.h"
#include "ecflow-rand.h"
#include "registry.h"

namespace ECFlow
{
    class LevelBasedLearningTopology : public LearningTopology
    {
    private:
        int _levels;
        int _order_size;
        int* _orders;

        void setSize(int size)
        {
            if (_order_size < size)
            {
                delete[] _orders;
                _order_size = size;
                _orders = new int[_order_size];
            }
        }

        void update(IndividualArray& swarm)
        {
            int swarm_size = swarm.getSize();
            setSize(swarm_size);
            for (int i = 0; i < swarm_size; i++)
            {
                _orders[i] = i;
            }

            // 插入排序(因为后代很可能也是序良好的)
            for (int i = 1; i < swarm_size; i++)
            {
                int j = i - 1;
                int key = _orders[i];
                while ((j >= 0) && (swarm[key] < swarm[_orders[j]]))
                {
                    _orders[j + 1] = _orders[j];
                    j--;
                }
                _orders[j + 1] = key;
            }
        }

    public:
        LevelBasedLearningTopology(int level_number = 4) : LearningTopology()
        {
            _levels = level_number;
            _order_size = 0;
            _orders = nullptr;
        }

        ~LevelBasedLearningTopology() { delete[] _orders; }

        static void preAssert(AssertList& list, double* paras) {}

        static void postAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 2, MatchType::postAssert));
            list.add(new Assert(ModuleType::T_learntopology, "graphScale", 100, MatchType::postAssert)); // 输出图规模 = 100% 当前种群
        }

        void ini() override {}

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** best_holder,
                                   const int swarm_number, IndividualArray* offspring, LearningGraph* last_graph) override
        {
            IndividualArray* cswarm = subswarm[0];
            int graph_size = cswarm->getSize();
            LearningGraph* back = new LearningGraph(graph_size, 2);

            int sid1, sid2;
            Individual* s1;
            Solution* s2;
            int level_capacity = int(ceil(double(graph_size) / _levels));

            // 更新排序
            update(*cswarm);

            // 设置第一层级(最优,不学习)
            for (int cid = 0; cid < level_capacity; cid++)
            {
                s1 = &cswarm[0][_orders[cid]];
                back->addStart(s1);

                back->addEnd(nullptr);
                back->addEnd(nullptr);
            }

            // 设置后续层级
            int counter = level_capacity;
            for (int lid = 1; lid < _levels; lid++)
            {
                for (int cid = 0; cid < level_capacity && counter < graph_size; cid++) // 计数器避免因最后一层不全导致越界
                {
                    // 获取学习对象编号:高层 [0,lid) 内先随机选层、再随机选层内位置
                    sid1 = ECFlow::get_int(0, lid - 1) * level_capacity + ECFlow::get_int(0, level_capacity - 1);
                    sid2 = ECFlow::get_int(0, lid - 1) * level_capacity + ECFlow::get_int(0, level_capacity - 1);

                    // 保证 sid1 优于 sid2(_orders 最优在前,下标小者更优)
                    if (sid2 < sid1)
                        std::swap(sid1, sid2);

                    // 插入拓扑图
                    s1 = &cswarm[0][counter];
                    back->addStart(s1);
                    counter++;

                    s2 = &cswarm[0][_orders[sid1]].solution;
                    back->addEnd(s2);
                    s2 = &cswarm[0][_orders[sid2]].solution;
                    back->addEnd(s2);
                }
            }

            return back;
        }
    };

    inline Registry<LearningTopology>::Entry levelBasedLearningTopologyEntry()
    {
        return { "LevelBasedLearning", ModuleType::T_learntopology,
            ParameterTemplate{ { {"level_number", ParamKind::Int, 1, 10, false, 2, 6} } }, sizeof(LevelBasedLearningTopology),
            [](const double* p) -> LearningTopology* { return new LevelBasedLearningTopology(p ? int(p[0]) : 4); },
            [](AssertList& L, const double* p) { LevelBasedLearningTopology::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { LevelBasedLearningTopology::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(ltopo_levelbased, LearningTopology, levelBasedLearningTopologyEntry());
}
