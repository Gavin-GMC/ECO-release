//------------------------Description------------------------
// 破坏-重建策略 DestructRebuild(源自 Iterated Greedy):每代把当前解的 **k 个决策"拆掉"**,其余保留,
//   被拆的维**逐维向问题句柄要贪婪选择**重建 → 得到一个"部分继承 + 部分贪婪重构"的候选。
//-------------------------Reference-------------------------
// Ruiz, R., Stützle, T. "A simple and effective iterated greedy algorithm for the permutation
//   flowshop scheduling problem." European Journal of Operational Research, 177(3):2033-2049, 2007.
// 忠实原文三要素:①**destruction**——随机移除 d 个决策(原文 d 个工件);②**construction**——被移除者逐个
//   按**贪婪**(最优位置/选择)重新插回;③**acceptance**——优则接受(或常温 Metropolis 式)。
// 分工:①②由本算子承担;③**由选择器承担**(index(true)=仅更优才接受;indexAnneal=原文 RS 常温接受变体),
//   与框架"生成/接受"两层分工一致,不塞进算子。可选的局部搜索作为**独立段**追加,不混入本算子。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <vector>
#include <string>
#include "learning-strategy.h"
#include "ecflow-rand.h"
#include "registry.h"

namespace ECFlow
{
    // 破坏 k 维 + 逐维贪婪重建(贪婪来自问题句柄);接受交选择器。
    class DestructRebuild : public LearningStrategy
    {
    private:
        int _destruct_size;   // 每次拆掉的决策数 d

    public:
        explicit DestructRebuild(int destruct_size = 4) : _destruct_size(destruct_size) {}
        ~DestructRebuild() {}

        // 每个体一份"破坏掩码"(1=拆/待贪婪重建, 0=保留亲代值);按解维定尺,Zero 初始化
        std::vector<FeatureDemand> featureDemands() const override
        {
            return { { "destruct", "vector", FeatureScope::Private, {} } };
        }

        static void preAssert(AssertList& /*list*/, double* /*paras*/) {}
        static void postAssert(AssertList& list, double* /*paras*/)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert));   // 逐维构造
        }

        // 破坏:随机选 d 个维标记待重建(其余保留亲代值)
        void preparation_i(Individual* /*individual*/, Solution** /*learning_object*/, Individual* child) override
        {
            VectorFeature* mask = child->feature<VectorFeature>(featureKey("destruct"));
            int n = (int)mask->data.size();
            if (n <= 0) return;
            for (int i = 0; i < n; i++) mask->data[i] = 0.0;                 // 先全保留

            int k = (_destruct_size < n) ? _destruct_size : n;               // d 超解维时截断
            int picked = 0;
            while (picked < k)                                               // 无放回随机选 k 维
            {
                int d = get_int(0, n - 1);
                if (mask->data[d] == 0.0) { mask->data[d] = 1.0; picked++; }
            }
        }

        // 保留维 → 亲代值;被拆维 → 问句柄要贪婪选择(此时约束状态已推进到前 d-1 维)
        double nextDecision(const int decision_d, Individual* individual, Solution** /*learning_object*/,
                            ProblemHandle* problem_handle, Individual* child) override
        {
            VectorFeature* mask = child->feature<VectorFeature>(featureKey("destruct"));
            if (mask->data[decision_d] == 0.0) return (*individual)[decision_d];   // 保留
            return problem_handle->getPrioriChoice(decision_d);                    // 贪婪重建(问题侧知识)
        }
    };

    inline Registry<LearningStrategy>::Entry destructRebuildEntry()
    {
        return { "DestructRebuild", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"destruct_size", ParamKind::Int, 1, 0x3f3f3f3f, false, 2, 8} } }, sizeof(DestructRebuild),
            [](const double* p) -> LearningStrategy* { return new DestructRebuild(p ? (int)p[0] : 4); },
            [](AssertList& L, const double* p) { DestructRebuild::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { DestructRebuild::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(lstrat_destructrebuild, LearningStrategy, destructRebuildEntry());
}
