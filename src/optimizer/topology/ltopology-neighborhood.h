//------------------------Description------------------------
// 邻域学习拓扑 Neighborhood:MOEA/D 的分解式邻域交配——每个子问题 i(=槽位下标)从其 T 个最近权重邻居中取 2 个
//   作交配对象(以概率 δ 邻域内、否则全群逃局部)。权重/邻域由 N 确定式生成并按 N 缓存。
//-------------------------Reference-------------------------
// Q. Zhang, H. Li, "MOEA/D: A Multiobjective Evolutionary Algorithm Based on Decomposition,"
// IEEE Trans. Evolutionary Computation, vol. 11, no. 6, pp. 712-731, 2007, doi: 10.1109/TEVC.2007.892759.
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <vector>
#include "learning-topology.h"
#include "mo-util.h"
#include "ecflow-rand.h"
#include "registry.h"

namespace ECFlow
{
    class Neighborhood : public LearningTopology
    {
    private:
        int _nsize;
        double _delta;
        std::vector<std::vector<double>> _weights;
        std::vector<std::vector<int>> _B;
        int _cachedN;

        void ensure(int N, int m)
        {
            if (_cachedN != N)
            {
                _weights = MOUtil::generateWeights(N, m);
                _B = MOUtil::neighborhoods(_weights, _nsize);
                _cachedN = N;
            }
        }

    public:
        Neighborhood(int T = 10, double delta = 0.9) : LearningTopology(), _nsize(T), _delta(delta), _cachedN(-1) {}
        ~Neighborhood() {}

        static void preAssert(AssertList& list, double* paras) {}
        static void postAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 2, MatchType::postAssert)); // 两亲代(交配)
        }

        void ini() override {}   // 权重仅依赖 N(跨轮不变),缓存保留

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** best_holder,
                                   const int swarm_number, IndividualArray* offspring, LearningGraph* last_graph) override
        {
            IndividualArray* cswarm = subswarm[0];
            int N = cswarm->getSize();
            if (N <= 0) return new LearningGraph(0, 2);
            int m = (*cswarm)[0].getObjectNumber();
            ensure(N, m);

            LearningGraph* graph = new LearningGraph(N, 2);
            for (int i = 0; i < N; i++)
            {
                graph->addStart(&(*cswarm)[i]);
                for (int e = 0; e < 2; e++)
                {
                    int idx;
                    if (rand01() < _delta) idx = _B[i][ECFlow::get_int(0, (int)_B[i].size() - 1)];   // 邻域内
                    else                   idx = ECFlow::get_int(0, N - 1);                            // 全群(逃局部)
                    graph->addEnd(&(*cswarm)[idx].solution);
                }
            }
            return graph;
        }
    };

    inline Registry<LearningTopology>::Entry neighborhoodEntry()
    {
        return { "Neighborhood", ModuleType::T_learntopology,
            ParameterTemplate{ { {"T", ParamKind::Int, 1, 0x3f3f3f3f, false, 5, 20},
                                 {"delta", ParamKind::Real, 0.0, 1.0, false, 0.8, 1.0} } }, sizeof(Neighborhood),
            [](const double* p) -> LearningTopology* { return p ? new Neighborhood((int)p[0], p[1]) : new Neighborhood(); },
            [](AssertList& L, const double* p) { Neighborhood::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { Neighborhood::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(ltopo_neighborhood, LearningTopology, neighborhoodEntry());
}
