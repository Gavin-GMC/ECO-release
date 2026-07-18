//------------------------Description------------------------
// ABC(人工蜂群)学习策略。本文件承载 ABC 竖切的两个新算子:
//   DifferencePerturbation —— 邻域算子 v=x_i+φ(x_i−x_k):随机 1 维、φ∈[−1,1]、k=1 个同伴(拓扑供)。employed/onlooker 共用。
//   Reinitialize          —— scout 重初始化(随机重生 + 重置特性,含 age 归零);见 scout 段(AgeActivation 拓扑 + 本策略)。
//-------------------------Reference-------------------------
// Karaboga 2005 ABC。邻域搜索 v_ij=x_ij+φ_ij(x_ij−x_kj)(单随机维 j、φ∈[−1,1]、k≠i 随机同伴)。
//   与 DE 的 DifferenceCrossover 区别:自参照(self+1 同伴,非 2 同伴)、单维(非全维/cr 逐维)、φ 随机(非固定 F)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "learning-strategy.h"
#include "ecflow-rand.h"
#include "registry.h"

namespace ECFlow
{
    // ABC 邻域:v = x_i + φ(x_i − x_k),随机 1 维、φ∈[−1,1]、k = 1 个同伴(学习对象)。其余维保持 x_i。
    class DifferencePerturbation : public LearningStrategy
    {
    public:
        DifferencePerturbation() {}
        ~DifferencePerturbation() {}

        static void preAssert(AssertList& list, double* /*paras*/)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::notLessButNotice));   // 需 1 个同伴
        }
        static void postAssert(AssertList& /*list*/, double* /*paras*/) {}

        void getNewIndividual(Individual* child, Individual* individual, Solution** learning_object, ProblemHandle* /*problem_handle*/) override
        {
            Solution* c  = &child->solution;
            Solution* xi = &individual->solution;
            Solution* xk = learning_object[0];   // 同伴(拓扑保非空:空指针者已被 generator 契约 copy-through、不入此)

            int dim = c->getSolutionSize();
            for (int d = 0; d < dim; d++) c->result[d] = xi->result[d];   // 复制起点

            int d = get_int(0, dim - 1);                                  // 随机 1 维
            double phi = 2.0 * rand01() - 1.0;                            // φ ∈ [−1,1]
            c->result[d] = xi->result[d] + phi * (xi->result[d] - xk->result[d]);
        }
    };

    inline Registry<LearningStrategy>::Entry differencePerturbationEntry()
    {
        return { "DifferencePerturbation", ModuleType::T_learnstrategy, ParameterTemplate{}, sizeof(DifferencePerturbation),
            [](const double*) -> LearningStrategy* { return new DifferencePerturbation(); },
            [](AssertList& L, const double* p) { DifferencePerturbation::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { DifferencePerturbation::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(lstrat_diffperturb, LearningStrategy, differencePerturbationEntry());

    // scout 重初始化:随机重解 + 重置特性(经 ini_speciality;含 AgeFeature 归零)。**对 age/血缘零依赖**——
    //   由个体自带初始化器(offspring 已 setInitializer)重生,age 归零是特性 ini 的副作用,本策略不认识 age。不评估(RunEvaluate 负责)。
    class Reinitialize : public LearningStrategy
    {
    public:
        Reinitialize() {}
        ~Reinitialize() {}
        static void preAssert(AssertList& /*list*/, double* /*paras*/) {}
        static void postAssert(AssertList& /*list*/, double* /*paras*/) {}

        void getNewIndividual(Individual* child, Individual* /*individual*/, Solution** /*learning_object*/, ProblemHandle* /*h*/) override
        {
            child->ini(true, false, true);   // 重解(随机初始化器) + 重置特性(age→0);不评估
        }
    };

    inline Registry<LearningStrategy>::Entry reinitializeEntry()
    {
        return { "Reinitialize", ModuleType::T_learnstrategy, ParameterTemplate{}, sizeof(Reinitialize),
            [](const double*) -> LearningStrategy* { return new Reinitialize(); },
            [](AssertList& L, const double* p) { Reinitialize::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { Reinitialize::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(lstrat_reinit, LearningStrategy, reinitializeEntry());
}
