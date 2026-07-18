//------------------------Description------------------------
// 环境选择 ScalarReplace:MOEA/D 的标量化替换——每个子问题 i 的子代按 Tchebycheff 值尝试替换其邻域内 ≤nr 个邻居。
//   行为名(纪律 4):按"标量化值 + 邻域"替换,不绑算法名。
//-------------------------Reference-------------------------
// Q. Zhang, H. Li, "MOEA/D: A Multiobjective Evolutionary Algorithm Based on Decomposition,"
// IEEE Trans. Evolutionary Computation, vol. 11, no. 6, pp. 712-731, 2007.
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
#include "ecflow-rand.h"
#include "registry.h"

namespace ECFlow
{
    class ScalarReplace final : public EnvirSelect
    {
    private:
        int _nr;
        int _nsize;
        std::vector<std::vector<double>> _weights;
        std::vector<std::vector<int>> _B;
        std::vector<double> _z;      // 历史理想点(单调,跨代持久)
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
        ScalarReplace(int nr = 2, int T = 10) : EnvirSelect(new UnconditionalAccept()), _nr(nr), _nsize(T), _cachedN(-1) {}
        ~ScalarReplace() {}

        void reset() override { EnvirSelect::reset(); _z.clear(); }   // exe(n) 每轮复位 z*(权重缓存保留,N 不变)

        void update_subswarm(IndividualArray& parent, IndividualArray& offspring, Terminator* terminator, BestArchive* archive) override
        {
            int N = parent.getSize();
            if (N <= 0) return;
            int m = parent[0].getObjectNumber();
            ensure(N, m);
            int lam = offspring.getSize();

            // 历史理想点 z*(单调更新)
            if ((int)_z.size() != m) _z.assign(m, 1e300);
            for (int i = 0; i < N; i++)
                for (int j = 0; j < m; j++)
                    if (parent[i].solution.fitness[j] < _z[j]) _z[j] = parent[i].solution.fitness[j];
            for (int i = 0; i < lam; i++)
                for (int j = 0; j < m; j++)
                    if (offspring[i].solution.fitness[j] < _z[j]) _z[j] = offspring[i].solution.fitness[j];

            // 逐子问题:标量化替换邻域内 ≤nr 个
            int lim = (lam < N) ? lam : N;
            for (int i = 0; i < lim; i++)
            {
                Individual& child = offspring[i];
                std::vector<int> nb = _B[i];
                for (int k = (int)nb.size() - 1; k > 0; k--) { int r = ECFlow::get_int(0, k); std::swap(nb[k], nb[r]); } // 打乱

                int replaced = 0;
                for (int j : nb)
                {
                    if (replaced >= _nr) break;
                    double gc = MOUtil::tchebycheff(child.solution.fitness, _weights[j], _z, m);
                    double gp = MOUtil::tchebycheff(parent[j].solution.fitness, _weights[j], _z, m);
                    if (gc <= gp) { parent[j].copy(child); replaced++; }   // 拷内容,槽=子问题恒定
                }
            }
        }
    };

    inline Registry<EnvirSelect>::Entry scalarReplaceEntry()
    {
        return { "ScalarReplace", ModuleType::T_selector,
            ParameterTemplate{ { {"nr", ParamKind::Int, 1, 0x3f3f3f3f, false, 1, 3},
                                 {"T",  ParamKind::Int, 1, 0x3f3f3f3f, false, 5, 20} } }, sizeof(ScalarReplace),
            [](const double* p) -> EnvirSelect* { return p ? new ScalarReplace((int)p[0], (int)p[1]) : new ScalarReplace(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    ECFLOW_REGISTER(sel_scalarreplace, EnvirSelect, scalarReplaceEntry());
}
