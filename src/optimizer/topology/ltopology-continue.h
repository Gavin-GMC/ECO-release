//------------------------Description------------------------
// 续接/自学习拓扑三种:
//   ContinueTopology  子代向自身学习(变异 / 局部搜索)——当代自足,不依赖上一代图。
//   InheritTopology   继承上一代图的起点、以当前子代为学习范例——跨代续接。
//   NoChangeTopology  原样复制上一代图(起点与范例全继承)——跨代续接。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include "learning-topology.h"
#include "registry.h"

namespace EvoFlow
{
    // 子代面向自身的学习(主要用于变异和局部搜索)
    class ContinueTopology : public LearningTopology
    {
    public:
        ContinueTopology() : LearningTopology() {}
        ~ContinueTopology() {}

        static void preAssert(AssertList& list, double* paras)
        {
            // Continue 从当前 offspring 取起点并以自身为唯一学习对象；Selector 清空 offspring 后即不可用。
            list.add(new Assert(ModuleType::T_offspringgenerator, "offspringAvailable", 1, MatchType::notLess));
        }
        static void postAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::postAssert));
            // 起点直接取当前 offspring,可供 inplace generator 原地续接。
            list.add(new Assert(ModuleType::T_learntopology, "offspringStart", 1, MatchType::postAssert));
        }

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
            // Inherit 的起点来自 last_graph，学习对象来自当前 offspring，二者缺一不可。
            list.add(new Assert(ModuleType::T_offspringgenerator, "offspringAvailable", 1, MatchType::notLess));
            list.add(new Assert(ModuleType::T_offspringgenerator, "lastGraphAvailable", 1, MatchType::notLess));
        }
        static void postAssert(AssertList& list, double* paras)
        {
            // getTopology 固定为每条边一个当前子代学习对象，不继承上一拓扑的 objects/coupled。
            list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::postAssert));
        }

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
            // NoChange 只复制 last_graph，不读取当前 offspring。
            list.add(new Assert(ModuleType::T_offspringgenerator, "lastGraphAvailable", 1, MatchType::notLess));
        }
        static void postAssert(AssertList& list, double* paras)
        {
            // Matcher 仅继承上一张图的 objects/coupled/graphScale；行为和起点来源不继承。
            list.add(new Assert(ModuleType::T_learntopology, "reuseGraphStructure", 1, MatchType::postAssert));
        }

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
    EVOFLOW_REGISTER(ltopo_continue, LearningTopology, continueTopologyEntry());
    EVOFLOW_REGISTER(ltopo_inherit,  LearningTopology, inheritTopologyEntry());
    EVOFLOW_REGISTER(ltopo_nochange, LearningTopology, noChangeTopologyEntry());
}
