//------------------------Description------------------------
// 禁忌搜索(Tabu Search)学习策略——落 strategy(生成侧记忆引导),两种形态:
//   ① TabuDecorator(装饰器):继承策略 + 内部组合一个邻域变异(黑盒),diff 推断移动特征 + 拒绝采样(禁忌则重产)。
//   ② TabuBit(自带点变异邻域):自实现点变异(选一维取值域内随机值),知道改哪维(精确移动属性)+ 主动从非禁忌维选(生成时引导)。
//-------------------------Reference-------------------------
// 新增算子(非迁移)。禁忌搜索:Glover 1986。落位分析见 v3.11.2 讨论(禁忌属生成引导=strategy,非接受过滤=selector)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
//-----------------------------------------------------------

#pragma once
#include <deque>
#include <vector>
#include <algorithm>
#include "solution.h"
#include "individual.h"
#include "individual-array.h"
#include "learning-strategy.h"
#include "ecflow-rand.h"
#include "registry.h"

namespace ECFlow
{
    // 装饰器包裹的内部邻域(枚举 → tag);para[0]=此枚举,para[1]=tenure,para[2..]=inner 参数
    inline const char* const* tabuInnerTags()
    {
        static const char* const tags[] = { "Overturn", "Insert", "Exchange", "SegmentRelocate", "Reorder" };
        return tags;
    }
    inline int tabuInnerCount() { return 5; }

    // ① 装饰器禁忌:内部组合一个邻域变异(黑盒),diff + 拒绝采样。整体构建。
    class TabuDecorator : public LearningStrategy
    {
    private:
        LearningStrategy* _inner;                 // 被装饰的邻域变异(拥有)
        int _tenure;
        std::deque<std::vector<int>> _tabu;        // 全局禁忌表:最近移动的变化位置集
        static const int MAX_RETRY = 20;

        static std::vector<int> moveFeature(Individual* parent, Individual* child)
        {
            std::vector<int> feat;                 // 变化位置集(升序)
            int n = child->getSolutionSize();
            for (int d = 0; d < n; d++)
                if (parent->solution.result[d] != child->solution.result[d]) feat.push_back(d);
            return feat;
        }
        bool isTabu(const std::vector<int>& feat) const
        {
            for (const auto& t : _tabu) if (t == feat) return true;   // 位置集相等 = 禁忌
            return false;
        }
        void pushTabu(const std::vector<int>& feat)
        {
            _tabu.push_back(feat);
            while ((int)_tabu.size() > _tenure) _tabu.pop_front();
        }

    public:
        TabuDecorator(LearningStrategy* inner, int tenure) : _inner(inner), _tenure(tenure) {}
        ~TabuDecorator() { delete _inner; }

        void ini(ProblemHandle* h) override { if (_inner) _inner->ini(h); _tabu.clear(); }   // 复位禁忌表
        void setProblem(ProblemHandle* h) override { if (_inner) _inner->setProblem(h); }

        void getNewIndividual(Individual* child, Individual* parent, Solution** lo, ProblemHandle* h) override
        {
            for (int attempt = 0; attempt < MAX_RETRY; attempt++)
            {
                _inner->getNewIndividual(child, parent, lo, h);       // 黑盒变异产候选(整体)
                std::vector<int> feat = moveFeature(parent, child);
                if (feat.empty() || !isTabu(feat)) { pushTabu(feat); return; }   // 非禁忌 → 采纳 + 记忆
            }
            pushTabu(moveFeature(parent, child));   // 重试超限:接受最后一个(兜底,防死循环)
        }
    };

    // ② 自带点变异邻域禁忌:选一维(主动避非禁忌维)+ 该维取值域内随机值(点变异,通用编码;二元域下退化为翻转)。
    //   知道改哪一维=精确移动属性,无需拒绝采样。
    class TabuBit : public LearningStrategy
    {
    private:
        int _tenure;
        std::deque<int> _tabu_pos;   // 全局禁忌表:最近变异的维

    public:
        TabuBit(int tenure = 7) : _tenure(tenure) {}
        ~TabuBit() {}

        void ini(ProblemHandle*) override { _tabu_pos.clear(); }

        void getNewIndividual(Individual* child, Individual* parent, Solution**, ProblemHandle* h) override
        {
            child->solution.copy(parent->solution);
            int n = child->getSolutionSize();
            if (n < 1) return;

            // 主动引导:从非禁忌维选一维(自带邻域知道自己改哪一维)
            int pos = -1;
            for (int t = 0; t < 10 * n; t++)
            {
                int p = ECFlow::get_int(0, n - 1);
                bool tabu = false; for (int tp : _tabu_pos) if (tp == p) { tabu = true; break; }
                if (!tabu) { pos = p; break; }
            }
            if (pos < 0) pos = ECFlow::get_int(0, n - 1);   // 全禁忌:兜底随机

            // 点变异:该维取值域内随机值(循环确保 ≠ 原值 → 构成移动;二元域下即翻转)
            double old_v = child->solution.result[pos], v = old_v;
            for (int t = 0; t < 10 && v == old_v; t++) v = h->getRandomChoiceInspace(pos);
            child->solution.result[pos] = v;
            _tabu_pos.push_back(pos);
            while ((int)_tabu_pos.size() > _tenure) _tabu_pos.pop_front();
        }
    };

    // 装饰器:para[0]=inner 邻域枚举、para[1]=tenure、para[2..]=inner 参数;create/断言均按枚举转发 inner。
    inline Registry<LearningStrategy>::Entry tabuDecoratorEntry()
    {
        return { "TabuDecorator", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"inner",  ParamKind::Enum, 0, 4, false, 0, 0},
                                 {"tenure", ParamKind::Int,  1, 10000, false, 7, 7} },
                               // next:下一级 = **内层算子自己的完整模板**(schema 由本级 `inner` 的取值决定,指向另一个组件)。
                               //   与本 Entry 的 pre/post **同形** —— 它们也按 inner 委托内层(create/preAssert/postAssert 均 p+2)。
                               //   内层若也有 next(如内层为 ALNS),会被解析方继续逐级展开,**无需在此手写递归**。
                               //   my_para 为本级切片 → inner 在 my_para[0];内层参数从本级偏移 2 起(与 create 的 p+2 一致)。
                               [](const double* my_para, size_t n) -> ParameterTemplate {
                                   if (!my_para || n < 1) return ParameterTemplate{};
                                   int i = (int)my_para[0];
                                   if (i < 0 || i >= tabuInnerCount()) return ParameterTemplate{};
                                   const ParameterTemplate* t = Registry<LearningStrategy>::instance().params(tabuInnerTags()[i]);
                                   return t ? *t : ParameterTemplate{};
                               } },
            sizeof(TabuDecorator),
            [](const double* p) -> LearningStrategy* {
                int innerId = p ? (int)p[0] : 0; if (innerId < 0 || innerId >= tabuInnerCount()) innerId = 0;
                int tenure = p ? (int)p[1] : 7;
                LearningStrategy* inner = Registry<LearningStrategy>::instance().create(tabuInnerTags()[innerId], p ? p + 2 : nullptr);
                return new TabuDecorator(inner, tenure);
            },
            [](AssertList& L, const double* p) { if (p) { int i = (int)p[0]; if (i >= 0 && i < tabuInnerCount()) Registry<LearningStrategy>::instance().preAssert(tabuInnerTags()[i], L, p + 2); } },
            [](AssertList& L, const double* p) { if (p) { int i = (int)p[0]; if (i >= 0 && i < tabuInnerCount()) Registry<LearningStrategy>::instance().postAssert(tabuInnerTags()[i], L, p + 2); } } };
    }
    ECFLOW_REGISTER(lstrat_tabu_decorator, LearningStrategy, tabuDecoratorEntry());

    inline Registry<LearningStrategy>::Entry tabuBitEntry()
    {
        return { "TabuBit", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"tenure", ParamKind::Int, 1, 10000, false, 5, 10} } }, sizeof(TabuBit),
            [](const double* p) -> LearningStrategy* { return p ? new TabuBit((int)p[0]) : new TabuBit(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    ECFLOW_REGISTER(lstrat_tabu_bit, LearningStrategy, tabuBitEntry());
}
