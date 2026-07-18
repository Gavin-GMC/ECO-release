//------------------------Description------------------------
// 烟花算法环境选择 DistanceSelect:候选池(烟花+全部火花)中保留最优 1 个,其余 n-1 个按**距离密度**概率选。
//   距离越大(越孤立)选中概率越高 → 保多样性(避免收敛到同一簇),是 FWA 原文选择策略。
//-------------------------Reference-------------------------
// Y. Tan, Y. Zhu, "Fireworks Algorithm for Optimization," ICSI 2010, LNCS 6145, pp. 355-364.
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <vector>
#include "selector.h"
#include "individual-array.h"
#include "ecflow-math.h"
#include "ecflow-rand.h"
#include "registry.h"

namespace ECFlow
{
    class DistanceSelect final : public EnvirSelect
    {
    public:
        DistanceSelect() : EnvirSelect(new UnconditionalAccept()) {}   // 不走接受层(占位闲置)
        ~DistanceSelect() {}

        void update_subswarm(IndividualArray& parent, IndividualArray& offspring, Terminator* terminator, BestArchive* archive) override
        {
            int mu = parent.getSize();
            int lam = offspring.getSize();
            int total = mu + lam;
            if (mu <= 0 || total <= mu) return;   // 无候选补充,保持不变

            auto cand = [&](int i) -> Individual& { return (i < mu) ? parent[i] : offspring[i - mu]; };

            // 距离密度 R[i] = Σ_j dist(i,j)
            std::vector<double> R(total, 0.0);
            for (int i = 0; i < total; i++)
                for (int j = i + 1; j < total; j++)
                {
                    double d = eu_distance(cand(i).solution.result, cand(j).solution.result);
                    R[i] += d; R[j] += d;
                }

            // 保留最优(operator< 方向感知)
            std::vector<char> sel(total, 0);
            int best = 0;
            for (int i = 1; i < total; i++)
                if (cand(i) < cand(best)) best = i;
            sel[best] = 1;
            int chosen = 1;

            // 其余按 R roulette 选够 μ 个
            double sumR = 0.0;
            for (int i = 0; i < total; i++) if (!sel[i]) sumR += R[i];
            while (chosen < mu)
            {
                int pick = -1;
                if (sumR > 1e-12)
                {
                    double r = rand01() * sumR, acc = 0.0;
                    for (int i = 0; i < total; i++)
                        if (!sel[i]) { acc += R[i]; if (acc >= r) { pick = i; break; } }
                }
                if (pick < 0)   // 兜底:取第一个未选中
                    for (int i = 0; i < total; i++) if (!sel[i]) { pick = i; break; }
                if (pick < 0) break;
                sel[pick] = 1; chosen++; sumR -= R[pick];
            }

            // 选中的 offspring 放回未选中的 parent 槽
            std::vector<int> free_slots, sel_off;
            for (int k = 0; k < mu; k++)        if (!sel[k])       free_slots.push_back(k);
            for (int o = 0; o < lam; o++)       if (sel[mu + o])   sel_off.push_back(o);
            int n = (int)((free_slots.size() < sel_off.size()) ? free_slots.size() : sel_off.size());
            for (int t = 0; t < n; t++)
            {
                parent[free_slots[t]].swap(offspring[sel_off[t]]);
            }
        }
    };

    inline Registry<EnvirSelect>::Entry distanceSelectEntry()
    {
        return { "DistanceSelect", ModuleType::T_selector, ParameterTemplate{}, sizeof(DistanceSelect),
            [](const double*) -> EnvirSelect* { return new DistanceSelect(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    ECFLOW_REGISTER(sel_distanceselect, EnvirSelect, distanceSelectEntry());
}
