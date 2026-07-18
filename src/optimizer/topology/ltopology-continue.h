//------------------------Description------------------------
// 续接/自学习拓扑三种:
//   ContinueTopology  子代向自身学习(变异 / 局部搜索)——当代自足,不依赖上一代图。
//   InheritTopology   继承上一代图的起点、以当前子代为学习范例——跨代续接。
//   NoChangeTopology  原样复制上一代图(起点与范例全继承)——跨代续接。
//-------------------------Reference-------------------------
// 无特定文献:ECFlow 框架自定义的续接/自学习拓扑(Continue 自学习、Inherit 继承起点、NoChange 复用上代图)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "learning-topology.h"
#include "registry.h"

namespace ECFlow
{
    // 子代面向自身的学习(主要用于变异和局部搜索)
    class ContinueTopology : public LearningTopology
    {
    public:
        ContinueTopology() : LearningTopology() {}
        ~ContinueTopology() {}

        static void preAssert(AssertList& list, double* paras) {}
        static void postAssert(AssertList& list, double* paras) {}

        void ini() override {}

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** best_holder,
                                   const int swarm_number, IndividualArray* offspring, LearningGraph* last_graph) override
        {
            int graph_size = offspring->getSize();
            LearningGraph* back = new LearningGraph(graph_size, 1);
            Individual* p_buffer;

            for (int i = 0; i < graph_size; i++)
            {
                p_buffer = &(*offspring)[i];
                back->addStart(p_buffer);
                back->addEnd(&p_buffer->solution);
            }
            return back;
        }
    };

    // 上一代的学习个体向其子代继续学习(继承起点、以当前子代为范例)
    class InheritTopology : public LearningTopology
    {
    public:
        InheritTopology() : LearningTopology() {}
        ~InheritTopology() {}

        static void preAssert(AssertList& list, double* paras)
        {
            // 草案(R1 结构预防标记,待 v3.8 匹配器强化):声明需前序产图状态
            list.add(new Assert(ModuleType::T_learntopology, "priorGraph", 1, MatchType::anyButNotice));
        }
        static void postAssert(AssertList& list, double* paras) {}

        void ini() override {}

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** best_holder,
                                   const int swarm_number, IndividualArray* offspring, LearningGraph* last_graph) override
        {
            if (last_graph == nullptr)
                return new LearningGraph(0, 1);   // R1:无上代图 → 空图兜底

            // R2 语义净版:起点继承 ≤ 上代起点数,终点 ≤ 子代数
            int off_size = offspring->getSize();
            int last_size = int(last_graph->getSize());
            int graph_size = off_size < last_size ? off_size : last_size;

            LearningGraph* back = new LearningGraph(graph_size, 1);
            Individual* p_buffer;

            for (int i = 0; i < graph_size; i++)
            {
                p_buffer = &(*offspring)[i];
                back->addStart(last_graph->getStartPoint(i));   // 继承上代起点
                back->addEnd(&p_buffer->solution);              // 以当前子代解为范例
            }
            return back;
        }
    };

    // 原样复制上一代图(起点与范例全部继承)
    class NoChangeTopology : public LearningTopology
    {
    public:
        NoChangeTopology() : LearningTopology() {}
        ~NoChangeTopology() {}

        static void preAssert(AssertList& list, double* paras)
        {
            // 草案(R1 结构预防标记,待 v3.8 匹配器强化):声明需前序产图状态
            list.add(new Assert(ModuleType::T_learntopology, "priorGraph", 1, MatchType::anyButNotice));
        }
        static void postAssert(AssertList& list, double* paras) {}

        void ini() override {}

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** best_holder,
                                   const int swarm_number, IndividualArray* offspring, LearningGraph* last_graph) override
        {
            if (last_graph == nullptr)
                return new LearningGraph(0, 1);   // R1:无上代图 → 空图兜底

            return last_graph->copy();
        }
    };

    inline Registry<LearningTopology>::Entry continueTopologyEntry()
    {
        return { "Continue", ModuleType::T_learntopology, ParameterTemplate{}, sizeof(ContinueTopology),
            [](const double*) -> LearningTopology* { return new ContinueTopology(); },
            [](AssertList& L, const double* p) { ContinueTopology::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { ContinueTopology::postAssert(L, const_cast<double*>(p)); } };
    }
    inline Registry<LearningTopology>::Entry inheritTopologyEntry()
    {
        return { "Inherit", ModuleType::T_learntopology, ParameterTemplate{}, sizeof(InheritTopology),
            [](const double*) -> LearningTopology* { return new InheritTopology(); },
            [](AssertList& L, const double* p) { InheritTopology::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { InheritTopology::postAssert(L, const_cast<double*>(p)); } };
    }
    inline Registry<LearningTopology>::Entry noChangeTopologyEntry()
    {
        return { "NoChange", ModuleType::T_learntopology, ParameterTemplate{}, sizeof(NoChangeTopology),
            [](const double*) -> LearningTopology* { return new NoChangeTopology(); },
            [](AssertList& L, const double* p) { NoChangeTopology::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { NoChangeTopology::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(ltopo_continue, LearningTopology, continueTopologyEntry());
    ECFLOW_REGISTER(ltopo_inherit,  LearningTopology, inheritTopologyEntry());
    ECFLOW_REGISTER(ltopo_nochange, LearningTopology, noChangeTopologyEntry());
}
