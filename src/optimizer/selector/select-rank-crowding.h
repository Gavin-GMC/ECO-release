//------------------------Description------------------------
// 环境选择 RankCrowding:合并亲代+子代,非支配排序后逐前沿保留,末层按前沿内拥挤度截断——NSGA-II 的环境选择。
//   行为名(纪律 4):按"支配 rank + 拥挤度"选择,不绑算法名。
//-------------------------Reference-------------------------
// K. Deb et al., "A fast and elitist multiobjective genetic algorithm: NSGA-II,"
// IEEE Trans. Evolutionary Computation, vol. 6, no. 2, pp. 182-197, 2002.
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <vector>
#include <algorithm>
#include "selector.h"
#include "individual-array.h"
#include "mo-util.h"
#include "registry.h"

namespace ECFlow
{
    class RankCrowding final : public EnvirSelect
    {
    public:
        RankCrowding() : EnvirSelect(new UnconditionalAccept()) {}   // 不走接受层(占位闲置)
        ~RankCrowding() {}

        void update_subswarm(IndividualArray& parent, IndividualArray& offspring, Terminator* terminator, BestArchive* archive) override
        {
            int mu = parent.getSize();
            int lam = offspring.getSize();
            int total = mu + lam;
            if (mu <= 0 || total <= mu) return;

            // 合并池(非拥有指针)
            std::vector<Individual*> pool(total);
            for (int i = 0; i < mu; i++)  pool[i]      = &parent[i];
            for (int i = 0; i < lam; i++) pool[mu + i] = &offspring[i];

            // 非支配排序
            std::vector<double> rank(total);
            MOUtil::fastNonDominatedSort(pool.data(), total, rank.data());

            // 分前沿(rank 从 1 起)
            int maxr = 1;
            for (int i = 0; i < total; i++) if ((int)rank[i] > maxr) maxr = (int)rank[i];
            std::vector<std::vector<int>> fronts(maxr + 1);
            for (int i = 0; i < total; i++) fronts[(int)rank[i]].push_back(i);

            // 逐前沿填,末层按前沿内拥挤度截
            std::vector<char> sel(total, 0);
            int chosen = 0;
            for (int r = 1; r <= maxr && chosen < mu; r++)
            {
                std::vector<int>& fr = fronts[r];
                if (fr.empty()) continue;

                if (chosen + (int)fr.size() <= mu)
                {
                    for (int idx : fr) sel[idx] = 1;
                    chosen += (int)fr.size();
                }
                else
                {
                    // 末层:算前沿内拥挤度,降序取 (mu-chosen)
                    int fs = (int)fr.size();
                    std::vector<Individual*> sub(fs);
                    for (int k = 0; k < fs; k++) sub[k] = pool[fr[k]];
                    std::vector<double> crowd(fs);
                    MOUtil::crowdingDistance(sub.data(), fs, crowd.data());

                    std::vector<int> order(fs);
                    for (int k = 0; k < fs; k++) order[k] = k;
                    std::sort(order.begin(), order.end(), [&crowd](int a, int b) { return crowd[a] > crowd[b]; });

                    int need = mu - chosen;
                    for (int k = 0; k < need; k++) sel[fr[order[k]]] = 1;
                    chosen = mu;
                }
            }

            // 放回:选中的 offspring swap 进未选中的 parent 槽
            std::vector<int> free_slots, sel_off;
            for (int k = 0; k < mu; k++)   if (!sel[k])       free_slots.push_back(k);
            for (int o = 0; o < lam; o++)  if (sel[mu + o])   sel_off.push_back(o);
            int nswap = (int)((free_slots.size() < sel_off.size()) ? free_slots.size() : sel_off.size());
            for (int t = 0; t < nswap; t++)
            {
                parent[free_slots[t]].swap(offspring[sel_off[t]]);
            }
        }
    };

    inline Registry<EnvirSelect>::Entry rankCrowdingEntry()
    {
        return { "RankCrowding", ModuleType::T_selector, ParameterTemplate{}, sizeof(RankCrowding),
            [](const double*) -> EnvirSelect* { return new RankCrowding(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    ECFLOW_REGISTER(sel_rankcrowding, EnvirSelect, rankCrowdingEntry());
}
