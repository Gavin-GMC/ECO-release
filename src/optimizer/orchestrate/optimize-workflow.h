//------------------------Description------------------------
// O_Workflow:子种群内 O_Component 的有序执行编排。单表 components 按 config 顺序,每代顺跑:
//   注入型 Set*** 就地 SET 上下文(ltopology/lstrategy/repairman)、执行型 Run*** 随后消费。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <vector>
#include <queue>
#include "optimize-component.h"
#include "solution-initializer.h"
#include "individual-array.h"

namespace ECFlow
{
    class OptimizerBuilder;

    class O_Workflow
    {
    private:
        friend OptimizerBuilder;

        std::vector<O_Component*> components;   // 单表:按 config 顺序,每代顺跑(Set*** 交错 SET / Run*** 执行)

        SolutionInitializer* initializer;
        LearningTopology*    ltopology;
        LearningStrategy*    lstrategy;
        SolutionRepaireman*  repairman;
        LearningGraph*       last_learning_graph;

        std::queue<LearningStrategy*> after_fes;
        std::queue<LearningStrategy*> after_upd;

    public:
        O_Workflow()
        {
            initializer         = nullptr;
            ltopology           = nullptr;
            lstrategy           = nullptr;
            repairman           = nullptr;
            last_learning_graph = nullptr;
        }

        ~O_Workflow()
        {
            for (int i = 0; i < (int)components.size(); i++)
                delete components[i];
            delete initializer;
            delete last_learning_graph;
            last_learning_graph = nullptr;
        }

        SolutionInitializer* getSolutionInitializer()
        {
            return initializer;
        }

        void ini()
        {
            // 每轮 exe 一次:算子复位(F1)。Set*** 复位其算子(PSO _w/ACO 信息素),Run*** no-op。
            for (int i = 0; i < (int)components.size(); i++)
                components[i]->ini();
        }

        void run(Terminator* terminator_pointer,
                 IndividualArray* offspring,
                 IndividualArray** parent,
                 BestArchive** archive,
                 int subswarm_number = 1)
        {
            for (int i = 0; i < (int)components.size(); i++)
            {
                if (terminator_pointer->termination())
                    break;
                components[i]->run(terminator_pointer, repairman, ltopology, lstrategy,
                                   offspring, parent, archive, subswarm_number,
                                   after_fes, after_upd, last_learning_graph);
            }

            // 迭代收尾:清子代 + 排空两队列 + 释放本代学习图(忠实原语义;续图跨代失效见头注)
            offspring->clear();
            while (!after_fes.empty()) after_fes.pop();
            while (!after_upd.empty()) after_upd.pop();
            delete last_learning_graph;
            last_learning_graph = nullptr;
        }

        void setProblem(ProblemHandle* problem_handle)
        {
            for (int i = 0; i < (int)components.size(); i++)   // F2:转发所有组件(SetStrategy 再转发 strategy)
                components[i]->setProblem(problem_handle);
            initializer->setProblem(problem_handle);
        }
    };
}
