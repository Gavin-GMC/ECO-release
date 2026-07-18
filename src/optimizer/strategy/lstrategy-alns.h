//------------------------Description------------------------
// 自适应算子选择 AdaptiveOperatorSelection(源自 ALNS 的自适应层):持一个**低层邻域算子池**,每代按**权重轮盘**
//   选一个算子委托生成;依本代结果给该算子**评分**,每 segment_len 代按反应因子更新权重 → 好算子被选中更频繁。
//-------------------------Reference-------------------------
// Ropke, S., Pisinger, D. "An adaptive large neighborhood search heuristic for the pickup and delivery
//   problem with time windows." Transportation Science, 40(4):455-472, 2006.
// 忠实原文自适应层三要素:
//   ①**轮盘选算子**:P(i) = w_i / Σw_j;
//   ②**评分 σ**:σ1=产生**新全局最优**、σ2=优于当前解、σ3=被接受但更差(原文经验值 33/9/13);
//   ③**权重更新**(每 segment 结束):w_i = w_i(1−ρ) + ρ·(π_i/θ_i)(π=本段累计得分、θ=本段使用次数、ρ=反应因子);
//      未使用(θ=0)的算子权重保持。
// 原文的**破坏-重建大邻域**由池内算子承担(如 `DestructRebuild`);原文的 **SA 式接受**由**选择器**承担
//   (`indexAnneal`),与框架"生成/接受"两层分工一致,不塞进算子。
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
    // 可入池的低层邻域算子(枚举 → tag);均无特性需求、默认参数即适合局搜
    inline const char* const* alnsOpTags()
    {
        static const char* const tags[] = { "Overturn", "Insert", "Exchange", "SegmentRelocate", "Reorder", "Gauss", "DestructRebuild" };
        return tags;
    }
    inline int alnsOpCount() { return 7; }

    class AdaptiveOperatorSelection : public LearningStrategy
    {
    private:
        std::vector<LearningStrategy*> _ops;   // 算子池(拥有)
        std::vector<double> _w;                // 权重
        std::vector<double> _pi;               // 本段累计得分 π
        std::vector<double> _theta;            // 本段使用次数 θ
        double _rho;                           // 反应因子
        int    _segment_len;                   // 权重更新周期(代)
        int    _cur;                           // 本代选中的算子下标
        int    _gen;                           // 代计数
        Comparer* _comparer;
        std::vector<double> _snapshot;         // 每代适应度快照(每代一次,成员安全)
        Individual _last_best, _cur_best;      // 自持上轮全局最优拷贝(判 σ1);借个体的 comparer/规模
        bool _bound, _has_last_best;

        // 原文经验得分
        static double S1() { return 33.0; }    // 新全局最优
        static double S2() { return 9.0; }     // 优于当前
        static double S3() { return 13.0; }    // 被接受但更差

        int rouletteByWeight()
        {
            double total = 0.0;
            for (double w : _w) total += w;
            if (total <= 0.0) return get_int(0, (int)_w.size() - 1);
            double p = rand01() * total, acc = 0.0;
            for (int i = 0; i < (int)_w.size(); i++) { acc += _w[i]; if (p <= acc) return i; }
            return (int)_w.size() - 1;
        }

        void updateWeights()   // w = w(1−ρ) + ρ·(π/θ);θ=0(本段未用)则保持
        {
            for (int i = 0; i < (int)_w.size(); i++)
            {
                if (_theta[i] > 0.0) _w[i] = _w[i] * (1.0 - _rho) + _rho * (_pi[i] / _theta[i]);
                _pi[i] = 0.0; _theta[i] = 0.0;
            }
        }

    public:
        AdaptiveOperatorSelection(std::vector<LearningStrategy*> ops, double rho, int segment_len)
            : _ops(std::move(ops)), _rho(rho), _segment_len(segment_len < 1 ? 1 : segment_len),
              _cur(0), _gen(0), _comparer(nullptr), _bound(false), _has_last_best(false)
        {
            _w.assign(_ops.size(), 1.0);   // 初始等权
            _pi.assign(_ops.size(), 0.0);
            _theta.assign(_ops.size(), 0.0);
        }
        ~AdaptiveOperatorSelection() { for (auto* o : _ops) delete o; }

        void ini(ProblemHandle* h) override
        {
            for (auto* o : _ops) if (o) o->ini(h);
            _w.assign(_ops.size(), 1.0); _pi.assign(_ops.size(), 0.0); _theta.assign(_ops.size(), 0.0);
            _cur = 0; _gen = 0; _has_last_best = false; _snapshot.clear();
        }
        void setProblem(ProblemHandle* h) override
        {
            for (auto* o : _ops) if (o) o->setProblem(h);
            _comparer = h->getSolutionComparer();
        }

        // 每代:轮盘选算子 + 快照 + 委托
        void preparation_s(IndividualArray& population, Terminator* terminator) override
        {
            _cur = rouletteByWeight();
            _theta[_cur] += 1.0;

            int n = population.getSize();
            _snapshot.assign(n, 0.0);
            for (int i = 0; i < n; i++) _snapshot[i] = population[i].solution.fitness[0];

            if (!_bound && n > 0) { _last_best.setProblem(population[0]); _cur_best.setProblem(population[0]); _bound = true; }
            if (_ops[_cur]) _ops[_cur]->preparation_s(population, terminator);
        }

        // ---- 生成钩子:一律委托本代选中的算子 ----
        void preparation_i(Individual* individual, Solution** lo, Individual* child) override
        { if (_ops[_cur]) _ops[_cur]->preparation_i(individual, lo, child); }
        void preparation_d(const int d, Individual* individual, Solution** lo, ProblemHandle* h, Individual* child) override
        { if (_ops[_cur]) _ops[_cur]->preparation_d(d, individual, lo, h, child); }
        double nextDecision(const int d, Individual* individual, Solution** lo, ProblemHandle* h, Individual* child) override
        { return _ops[_cur] ? _ops[_cur]->nextDecision(d, individual, lo, h, child) : (*individual)[d]; }
        void update_d(Individual* child, const int d) override
        { if (_ops[_cur]) _ops[_cur]->update_d(child, d); }
        void update_i(Individual* child) override
        { if (_ops[_cur]) _ops[_cur]->update_i(child); }
        void getNewIndividual(Individual* child, Individual* individual, Solution** lo, ProblemHandle* h) override
        { if (_ops[_cur]) _ops[_cur]->getNewIndividual(child, individual, lo, h); }

        // 选择之后:按结果给本代算子评分;每 segment_len 代更新权重
        void update_s(IndividualArray& population, IndividualArray& offspring, BestArchive* archive) override
        {
            int n = population.getSize();
            double score = 0.0;

            // σ1:全局最优是否被刷新(比对自持的上轮拷贝)
            Solution* elite = archive ? archive->getElite() : nullptr;
            bool new_global = false;
            if (elite && _bound)
            {
                _cur_best.solution.copy(*elite);
                if (!_has_last_best) new_global = true;
                else if (_cur_best < _last_best) new_global = true;
                if (new_global) { _last_best.solution.copy(*elite); _has_last_best = true; }
            }

            if (new_global) score = S1();
            else if ((int)_snapshot.size() >= n && _comparer)
            {
                int improved = 0, accepted_worse = 0;
                for (int i = 0; i < n; i++)
                {
                    double f = population[i].solution.fitness[0];
                    if (_comparer->isBetter(&f, &_snapshot[i])) improved++;
                    else if (f != _snapshot[i]) accepted_worse++;   // 变了但未更优 = 接受了更差(仅退火类接受下出现)
                }
                if (improved > 0)            score = S2();
                else if (accepted_worse > 0) score = S3();
            }
            _pi[_cur] += score;

            if (_ops[_cur]) _ops[_cur]->update_s(population, offspring, archive);

            _gen++;
            if (_gen % _segment_len == 0) updateWeights();
        }

        // 只读:供测试/诊断查看自适应权重
        const std::vector<double>& weights() const { return _w; }
        int currentOp() const { return _cur; }
    };

    inline Registry<LearningStrategy>::Entry adaptiveOperatorSelectionEntry()
    {
        return { "AdaptiveOperatorSelection", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"rho",         ParamKind::Real, 0.0, 1.0,   false, 0.05, 0.2},   // 反应因子
                                 {"segment_len", ParamKind::Int,  1, 0x3f3f3f3f, false, 10, 50},   // 权重更新周期(代)
                                 {"op_count",    ParamKind::Int,  1, 64,      false, 2, 5} },      // 池内算子数(其后跟 op_count 个枚举)
                               // next:下一级 = op_count 个算子枚举(长度由**本级 op_count 的取值**决定 → 通用规则表达不了)。
                               //   池内算子一律以默认参数构造(create(tag, nullptr)),故下一级只有枚举、不含算子自身的参数。
                               //   my_para 为本级切片 → op_count 在 my_para[2]。
                               [](const double* my_para, size_t n) -> ParameterTemplate {
                                   ParameterTemplate t;
                                   if (!my_para || n < 3) return t;
                                   int cnt = (int)my_para[2];
                                   if (cnt < 0) cnt = 0;
                                   for (int i = 0; i < cnt; i++)
                                       t.params.push_back({ "op" + std::to_string(i), ParamKind::Enum, 0, (double)(alnsOpCount() - 1), false, 0, (double)(alnsOpCount() - 1) });
                                   return t;
                               } },
            sizeof(AdaptiveOperatorSelection),
            [](const double* p) -> LearningStrategy* {
                double rho = p ? p[0] : 0.1;
                int seg = p ? (int)p[1] : 20;
                int cnt = p ? (int)p[2] : 0;
                std::vector<LearningStrategy*> ops;
                for (int i = 0; i < cnt; i++)
                {
                    int id = (int)p[3 + i];
                    if (id < 0 || id >= alnsOpCount()) continue;
                    LearningStrategy* o = Registry<LearningStrategy>::instance().create(alnsOpTags()[id], nullptr);   // 默认参数
                    if (o) ops.push_back(o);
                }
                if (ops.empty()) ops.push_back(Registry<LearningStrategy>::instance().create(alnsOpTags()[0], nullptr));
                return new AdaptiveOperatorSelection(std::move(ops), rho, seg);
            },
            [](AssertList& L, const double* p) {   // 池内算子 preAssert 的并集
                if (!p) return; int cnt = (int)p[2];
                for (int i = 0; i < cnt; i++) { int id = (int)p[3 + i];
                    if (id >= 0 && id < alnsOpCount()) Registry<LearningStrategy>::instance().preAssert(alnsOpTags()[id], L, nullptr); }
            },
            [](AssertList& L, const double* p) {   // 池内算子 postAssert 的并集
                if (!p) return; int cnt = (int)p[2];
                for (int i = 0; i < cnt; i++) { int id = (int)p[3 + i];
                    if (id >= 0 && id < alnsOpCount()) Registry<LearningStrategy>::instance().postAssert(alnsOpTags()[id], L, nullptr); }
            } };
    }
    ECFLOW_REGISTER(lstrat_adaptiveopselect, LearningStrategy, adaptiveOperatorSelectionEntry());
}
