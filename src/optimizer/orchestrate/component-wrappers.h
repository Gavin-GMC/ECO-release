//------------------------Description------------------------
// O_Component 包装(6 个):把已迁纯算子桥接进 workflow。注入型 Set***(SET 进上下文)/ 执行型 Run***(调算子方法 + 队列编排)。
//-------------------------Reference-------------------------
//   Run*** 由原 EnvirSelect::run / Evaluator::run / OffspringGenerator::run 的 run() 体抽出(算子本体已迁,此处为其 O_Component 适配)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "optimize-component.h"
#include "selector.h"
#include "evaluator.h"
#include "generator.h"

namespace ECFlow
{
    // ============ 注入型 Set*** ============

    class SetLearningTopology final : public O_Component
    {
        LearningTopology* _topology;
    public:
        SetLearningTopology(LearningTopology* topology) : _topology(topology) {}
        ~SetLearningTopology() { delete _topology; }

        std::vector<FeatureDemand> featureDemands() const override { return _topology->featureDemands(); }
        void setFeatureKey(const std::string& role, const std::string& key) override { _topology->setFeatureKey(role, key); }

        void ini() override { _topology->ini(); }

        void run(Terminator*, SolutionRepaireman*&, LearningTopology*& ltopology, LearningStrategy*&,
                 IndividualArray*, IndividualArray**, BestArchive**, int,
                 std::queue<LearningStrategy*>&, std::queue<LearningStrategy*>&, LearningGraph*&) override
        {
            ltopology = _topology;   // SET 进上下文
        }
    };

    class SetLearningStrategy final : public O_Component
    {
        LearningStrategy* _strategy;
    public:
        SetLearningStrategy(LearningStrategy* strategy) : _strategy(strategy) {}
        ~SetLearningStrategy() { delete _strategy; }

        std::vector<FeatureDemand> featureDemands() const override { return _strategy->featureDemands(); }
        void setFeatureKey(const std::string& role, const std::string& key) override { _strategy->setFeatureKey(role, key); }

        void setProblem(ProblemHandle* h) override { O_Component::setProblem(h); _strategy->setProblem(h); }   // 转发(ACO/EDA/SetPSO/Difference 需要)
        void ini() override { _strategy->ini(_problem); }

        void run(Terminator*, SolutionRepaireman*&, LearningTopology*&, LearningStrategy*& lstrategy,
                 IndividualArray*, IndividualArray**, BestArchive**, int,
                 std::queue<LearningStrategy*>&, std::queue<LearningStrategy*>&, LearningGraph*&) override
        {
            lstrategy = _strategy;
        }
    };

    class SetRepaire final : public O_Component
    {
        SolutionRepaireman* _repaire;
    public:
        SetRepaire(SolutionRepaireman* repaire) : _repaire(repaire) {}
        ~SetRepaire() { delete _repaire; }

        void run(Terminator*, SolutionRepaireman*& repairman, LearningTopology*&, LearningStrategy*&,
                 IndividualArray*, IndividualArray**, BestArchive**, int,
                 std::queue<LearningStrategy*>&, std::queue<LearningStrategy*>&, LearningGraph*&) override
        {
            repairman = _repaire;
        }
    };

    // ============ 执行型 Run*** ============

    class RunSelect final : public O_Component
    {
        EnvirSelect* _selector;
    public:
        RunSelect(EnvirSelect* selector) : _selector(selector) {}
        ~RunSelect() { delete _selector; }

        // INDIV-COMPOSE:转发接受层(AgingAccept)的 age 特性声明/键回填
        std::vector<FeatureDemand> featureDemands() const override { return _selector->featureDemands(); }
        void setFeatureKey(const std::string& role, const std::string& key) override { _selector->setFeatureKey(role, key); }

        void ini() override { _selector->reset(); }   // 每轮 exe 复位接受准则(退火温度归 T0);无状态准则 no-op

        void run(Terminator* terminator, SolutionRepaireman*&, LearningTopology*&, LearningStrategy*&,
                 IndividualArray* offspring, IndividualArray** parent, BestArchive** archive, int,
                 std::queue<LearningStrategy*>&, std::queue<LearningStrategy*>& after_upd, LearningGraph*&) override
        {
            _selector->update_subswarm(*parent[0], *offspring, terminator, archive[0]);   // 透传 terminator/archive(AgingAccept scout 用)
            while (!after_upd.empty())
            {
                after_upd.front()->update_s(*parent[0], *offspring, archive[0]);   // 传全局最优档案(MMAS getElite)
                after_upd.pop();   // 修复:原缺 pop(与按值队列 bug 抵消);按引用后必须 pop
            }
            offspring->clear();
        }
    };

    class RunEvaluate final : public O_Component
    {
        Evaluator* _evaluator;
    public:
        RunEvaluate(Evaluator* evaluator) : _evaluator(evaluator) {}
        ~RunEvaluate() { delete _evaluator; }

        void setProblem(ProblemHandle* h) override { O_Component::setProblem(h); _evaluator->setProblem(h); }

        void run(Terminator* terminator_pointer, SolutionRepaireman*&, LearningTopology*&, LearningStrategy*&,
                 IndividualArray* offspring, IndividualArray**, BestArchive** archive, int,
                 std::queue<LearningStrategy*>& after_fes, std::queue<LearningStrategy*>&, LearningGraph*&) override
        {
            _evaluator->evaluate(terminator_pointer, archive[0], *offspring);

            // 评估后:after_fes 里的策略逐个体 update_i
            while (!after_fes.empty())
            {
                LearningStrategy* strategy = after_fes.front();
                for (int i = 0; i < offspring->getSize(); i++)
                    strategy->update_i(&(*offspring)[i]);
                after_fes.pop();
            }
        }
    };

    class RunGenerate final : public O_Component
    {
        OffspringGenerator* _generator;
    public:
        RunGenerate(OffspringGenerator* generator) : _generator(generator) {}
        ~RunGenerate() { delete _generator; }

        void setProblem(ProblemHandle* h) override { O_Component::setProblem(h); _generator->setProblem(h); }

        void run(Terminator* terminator_pointer, SolutionRepaireman*& repairman, LearningTopology*& ltopology, LearningStrategy*& lstrategy,
                 IndividualArray* offspring, IndividualArray** parent, BestArchive** archive, int subswarm_number,
                 std::queue<LearningStrategy*>& after_fes, std::queue<LearningStrategy*>& after_upd, LearningGraph*& last_graph) override
        {
            // KINSHIP-ID 通用盖章:血缘特性存在时,给每源盖 id=当前槽序号(消费方朴素值),
            //   使 generator 出生派生的 offspring.parent_id 直接是源槽 → 血缘定向选择器免查找定位。
            //   非血缘工作流:hasFeature 守卫 → no-op。每段生成各自重盖(swap 打乱后自动纠正)。
            IndividualArray& pop = *parent[0];
            if (pop.getSize() > 0 && pop[0].hasFeature("kinship"))
                for (int i = 0; i < pop.getSize(); i++)
                    pop[i].feature<KinshipFeature>("kinship")->setId(i);

            _generator->generate(terminator_pointer, repairman, ltopology, lstrategy,
                offspring, parent, archive, subswarm_number, after_fes, after_upd, last_graph);
        }
    };
}
