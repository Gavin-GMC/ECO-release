//------------------------Description------------------------
// 路径重连 PathRelinking:在「发起解(起点个体)」与「引导解(学习对象)」之间的**路径**上取中间解——
//   逐维消解二者差异,消解比例由该子代的**随机深度 β** 决定 → 一个子代 = 路径上的一个采样点。
//-------------------------Reference-------------------------
// Glover, F. "Tabu search and adaptive memory programming — advances, applications and challenges." 1997.
// Resende, M.G.C., Ribeiro, C.C. (GRASP with path-relinking 系列)。
// 原文要义:从发起解出发,**逐步**把它变成引导解(每步消解一个差异属性),沿途中间解常含双亲优点,取**路径上最优**。
//-----------------------------------------------------------

#pragma once
#include <vector>
#include <string>
#include "learning-strategy.h"
#include "ecflow-rand.h"
#include "registry.h"

namespace ECFlow
{
    // 路径中间解:差异维按随机深度 β 取引导值,相同维保持。配 RepeatWithBest(k) + kinshipGreedy。
    class PathRelinking : public LearningStrategy
    {
    public:
        PathRelinking() {}
        ~PathRelinking() {}

        // 每子代一个路径深度 β(k 个同源子代 → k 个不同深度 = k 点采样)
        std::vector<FeatureDemand> featureDemands() const override
        {
            return { { "relinkdepth", "scalar", FeatureScope::Private, { 1 } } };
        }

        static void preAssert(AssertList& list, double* /*paras*/)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::notLessButNotice));   // 需引导解
        }
        static void postAssert(AssertList& list, double* /*paras*/)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert));    // 逐维构造
        }

        // 掷该子代的路径深度 β ~ U(0,1):0=贴近发起解、1=贴近引导解
        void preparation_i(Individual* /*individual*/, Solution** /*learning_object*/, Individual* child) override
        {
            child->feature<ScalarFeature>(featureKey("relinkdepth"))->vals[0] = rand01();
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object,
                            ProblemHandle*, Individual* child) override
        {
            double x = (*individual)[decision_d];
            if (learning_object[0] == nullptr) return x;
            double g = (*learning_object[0])[decision_d];
            if (x == g) return x;                                                        // 无差异维 → 保持

            double beta = child->feature<ScalarFeature>(featureKey("relinkdepth"))->vals[0];
            return (rand01() < beta) ? g : x;                                            // 差异维:按深度 β 消解
        }
    };

    inline Registry<LearningStrategy>::Entry pathRelinkingEntry()
    {
        return { "PathRelinking", ModuleType::T_learnstrategy, ParameterTemplate{}, sizeof(PathRelinking),   // 无参数(深度逐子代随机)
            [](const double*) -> LearningStrategy* { return new PathRelinking(); },
            [](AssertList& L, const double* p) { PathRelinking::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { PathRelinking::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(lstrat_pathrelinking, LearningStrategy, pathRelinkingEntry());
}
