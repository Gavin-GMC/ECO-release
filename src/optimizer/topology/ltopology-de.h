//------------------------Description------------------------
// 随机学习拓扑 RandomLearning:每个个体从种群中随机挑选 number 个"互不重复"的个体作为学习对象。
//   (差分进化等使用此拓扑;故原文件名 de,但行为=随机学习,tag 用行为名。)
//-------------------------Reference-------------------------
// [1] R. Storn and K. Price, "Differential Evolution — A Simple and Efficient Heuristic for Global
//     Optimization over Continuous Spaces," Journal of Global Optimization, vol. 11, no. 4, pp. 341-359,
//     1997, doi: 10.1023/A:1008202821328.
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "ecflow-constant.h"
#include "ecflow-rand.h"
#include "learning-topology.h"
#include "registry.h"

namespace ECFlow
{
    class RandomLearning : public LearningTopology
    {
    private:
        int _number;
        int* indexs;

        // 检查是否存在重复的个体选择
        bool isRepeat(int index)
        {
            for (int i = 0; i < index; i++)
            {
                if (indexs[i] == indexs[index])
                    return true;
            }
            return false;
        }

    public:
        RandomLearning(int number) : LearningTopology()
        {
            _number = number;
            indexs = new int[_number + 2];
        }

        ~RandomLearning() { delete[] indexs; }   // 迁移修复:原析构为空 → 泄漏 indexs

        static void preAssert(AssertList& list, double* paras) {}

        static void postAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", int(paras[0]), MatchType::postAssert));
            list.add(new Assert(ModuleType::T_learntopology, "graphScale", 100, MatchType::postAssert)); // 输出图规模 = 100% 当前种群
        }

        void ini() override {}

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** best_holder,
                                   const int swarm_number, IndividualArray* offspring, LearningGraph* last_graph) override
        {
            IndividualArray* cswarm = subswarm[0];
            int graph_size = cswarm->getSize();
            LearningGraph* back = new LearningGraph(graph_size, _number);

            int counter = 0;
            Individual* s1;
            Solution* s2;
            while (counter < graph_size)
            {
                // 选取学习亲本
                s1 = &(*cswarm)[counter];
                back->addStart(s1);
                indexs[0] = counter;

                // 选取学习对象（需要不重复）
                for (int i = 1; i <= _number; i++)
                {
                    indexs[i] = ECFlow::get_int(0, graph_size - 1);
                    while (isRepeat(i))
                    {
                        indexs[i] = ECFlow::get_int(0, graph_size - 1);
                    }

                    s2 = &cswarm[0][indexs[i]].solution;
                    back->addEnd(s2);
                }

                counter++;
            }

            return back;
        }
    };

    inline Registry<LearningTopology>::Entry randomLearningTopologyEntry()
    {
        return { "RandomLearning", ModuleType::T_learntopology,
            ParameterTemplate{ { {"number", ParamKind::Int, 1, 0x3f3f3f3f, false, 2, 5} } }, sizeof(RandomLearning),
            [](const double* p) -> LearningTopology* { return new RandomLearning(p ? int(p[0]) : 2); },
            [](AssertList& L, const double* p) { RandomLearning::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { RandomLearning::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(ltopo_randomlearning, LearningTopology, randomLearningTopologyEntry());
}
