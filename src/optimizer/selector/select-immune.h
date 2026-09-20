//------------------------Description------------------------
// 免疫算法用环境选择:WorstReplace —— 无条件用子代替换种群中最差的 λ 个个体(λ=子代数)。
//   用于感受器编辑/metadynamics:强制换血保多样性(不比较优劣,与择优的 Rank 互补)。

//-------------------------Reference-------------------------
// L. N. de Castro, F. J. Von Zuben, "Learning and optimization using the clonal selection principle,"
// IEEE Trans. Evolutionary Computation, vol. 6, no. 3, pp. 239-251, 2002 (metadynamics / receptor editing).

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include "selector.h"
#include "individual-array.h"
#include "registry.h"

namespace EvoFlow
{
    // 无条件替换最差 λ 个(感受器编辑/换血)
    class WorstReplace final : public EnvirSelect
    {
    public:
        WorstReplace() : EnvirSelect(new UnconditionalAccept()) {}   // 无条件,不走接受层(占位闲置)
        ~WorstReplace() {}

        void update_subswarm(IndividualArray& parent, IndividualArray& offspring, Terminator* terminator, BestArchive* archive) override
        {
            int mu = parent.getSize();
            int lambda = offspring.getSize();
            if (lambda > mu) lambda = mu;
            if (lambda <= 0) return;

            parent.sort();   // 最优在前、最差在尾
            for (int j = 0; j < lambda; j++)
            {
                parent[mu - 1 - j].swap(offspring[j]);   // 种群最差 λ 个 ← 子代(随机移民),无条件
            }
        }
    };

    inline Registry<EnvirSelect>::Entry worstReplaceEntry()
    {
        return { "WorstReplace", ModuleType::T_selector, ParameterTemplate{}, sizeof(WorstReplace),
            [](const double*) -> EnvirSelect* { return new WorstReplace(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    EVOFLOW_REGISTER(sel_worstreplace, EnvirSelect, worstReplaceEntry());
}
