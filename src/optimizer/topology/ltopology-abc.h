//------------------------Description------------------------
// ABC(人工蜂群)学习拓扑。本文件承载 ABC 竖切的拓扑:
//   RouletteSource —— onlooker(观察蜂):按 fitness 轮盘**有放回**选源作起点(多对一)+ 1 个随机同伴 k≠源作学习对象。
//   AgeActivation  —— scout(侦察蜂):见 scout 段(老化个体自学重生、未老化空指针透传);本段暂不含。
//-------------------------Reference-------------------------
// 新增(ABC 重做,1.4.4.5)。轮盘 fitness→概率归一化(min 化假设 + 防除零护栏)沿用 ltopology-ga.h/Roulette。
//   与 Roulette(GA 交叉)区别:仅对**起点**轮盘(源被加权多搜),学习对象取**均匀随机同伴**(非轮盘),不做对称对插。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "learning-topology.h"
#include "ecflow-rand.h"
#include "ecflow-constant.h"
#include "registry.h"

namespace ECFlow
{
    // onlooker:fitness 轮盘有放回选源(起点,多对一)+ 1 个随机同伴 k≠源(学习对象)。
    class RouletteSource : public LearningTopology
    {
    private:
        double* _board;
        int     _board_size;
        void setSize(int n) { if (_board_size != n) { delete[] _board; _board_size = n; _board = new double[n]; } }

    public:
        RouletteSource() : LearningTopology(), _board(nullptr), _board_size(0) {}
        ~RouletteSource() { delete[] _board; }

        static void preAssert(AssertList& /*list*/, double* /*paras*/) {}
        static void postAssert(AssertList& list, double* /*paras*/)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::postAssert));       // 供 1 同伴
            list.add(new Assert(ModuleType::T_learntopology, "graphScale", 100, MatchType::postAssert));  // 子代 = 100% 种群
        }

        void ini() override {}

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** /*best_holder*/,
                                   const int /*swarm_number*/, IndividualArray* /*offspring*/, LearningGraph* /*last_graph*/) override
        {
            IndividualArray* cs = subswarm[0];
            int n = cs->getSize();
            setSize(n);
            LearningGraph* g = new LearningGraph(n, 1);   // 每条 1 个学习对象(同伴)

            // fitness → 选择概率(min 化假设:越小越优,权重越大;防除零护栏,同 Roulette)
            double mx = -ECFLOW_MAX, mn = ECFLOW_MAX;
            for (int i = 0; i < n; i++)
            {
                double f = (*cs)[i].solution.fitness[0];
                if (f > mx) mx = f;
                if (f < mn) mn = f;
            }
            mx *= 1.001;   // 保证最差个体权重非 0
            double range = mx - mn, total = 0;
            if (range <= 0) { for (int i = 0; i < n; i++) { _board[i] = 1.0; total += 1.0; } }
            else            { for (int i = 0; i < n; i++) { _board[i] = (mx - (*cs)[i].solution.fitness[0]) / range; total += _board[i]; } }
            for (int i = 0; i < n; i++) _board[i] /= total;

            // SN 条边:轮盘选源(有放回) + 随机同伴 k≠源
            for (int j = 0; j < n; j++)
            {
                double p = rand01();
                int s = 0; double acc = _board[0];
                while (s < n - 1 && p > acc) { s++; acc += _board[s]; }   // 轮盘定源

                g->addStart(&(*cs)[s]);
                int k = get_int(0, n - 1);
                while (k == s && n > 1) k = get_int(0, n - 1);            // 同伴 k≠源
                g->addEnd(&(*cs)[k].solution);
            }
            return g;
        }
    };

    inline Registry<LearningTopology>::Entry rouletteSourceTopologyEntry()
    {
        return { "RouletteSource", ModuleType::T_learntopology, ParameterTemplate{}, sizeof(RouletteSource),
            [](const double*) -> LearningTopology* { return new RouletteSource(); },
            [](AssertList& L, const double* p) { RouletteSource::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { RouletteSource::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(ltopo_roulettesource, LearningTopology, rouletteSourceTopologyEntry());

    // scout 年龄拓扑:每个体作起点。老化(age>limit)→ 学习对象=自身(非空)→ 交 Reinitialize 重生;
    //   未老化 → 学习对象=空指针 → generator 契约 copy 进子代、不评估(age 不变、不耗 FES)。配 index(无条件) 位置替换。
    //   读 age(Singular 键 "age",消费方);preAssert 需 ageMaintained(= 计龄接受提供)→ 无计龄选择器则装配硬报错。
    class AgeActivation : public LearningTopology
    {
    private:
        int _limit;   // 停滞上限,超限即重生
    public:
        AgeActivation(int limit = 100) : LearningTopology(), _limit(limit) {}
        ~AgeActivation() {}

        static void preAssert(AssertList& list, double* /*paras*/)
        {
            list.add(new Assert(ModuleType::T_selector, "ageMaintained", 1, MatchType::notLess));   // 需 age 生产方(计龄接受)
        }
        static void postAssert(AssertList& list, double* /*paras*/)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::postAssert));
            list.add(new Assert(ModuleType::T_learntopology, "graphScale", 100, MatchType::postAssert));
        }

        void ini() override {}

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** /*best_holder*/,
                                   const int /*swarm_number*/, IndividualArray* /*offspring*/, LearningGraph* /*last_graph*/) override
        {
            IndividualArray* cs = subswarm[0];
            int n = cs->getSize();
            LearningGraph* g = new LearningGraph(n, 1);   // 每条 1 个学习对象槽(自身 或 空指针)
            for (int i = 0; i < n; i++)
            {
                g->addStart(&(*cs)[i]);
                bool aged = (*cs)[i].hasFeature("age") && (*cs)[i].feature<AgeFeature>("age")->get() > _limit;
                if (aged) g->addEnd(&(*cs)[i].solution);   // 自学(非空)→ Reinitialize 重生
                else      g->addEnd(nullptr);              // 透传(空指针契约)→ copy 进子代、不评估
            }
            return g;
        }
    };

    inline Registry<LearningTopology>::Entry ageActivationTopologyEntry()
    {
        return { "AgeActivation", ModuleType::T_learntopology,
            ParameterTemplate{ { {"limit", ParamKind::Int, 1, 0x3f3f3f3f, false, 20, 200} } }, sizeof(AgeActivation),
            [](const double* p) -> LearningTopology* { return new AgeActivation(p ? (int)p[0] : 100); },
            [](AssertList& L, const double* p) { AgeActivation::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { AgeActivation::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(ltopo_ageactivation, LearningTopology, ageActivationTopologyEntry());
}
