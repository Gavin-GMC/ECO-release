//------------------------Description------------------------
// 子代生成器 OffspringGenerator:亲代个体依学习图(topology 产出)+ 学习策略生成子代。4 种生成框架。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <queue>
#include <cstring>
#include "solution.h"
#include "individual-array.h"
#include "learning-topology.h"
#include "learning-graph.h"
#include "learning-strategy.h"
#include "problem-handle.h"
#include "best-archive.h"
#include "terminator.h"
#include "solution-repairman.h"
#include "registry.h"

namespace ECFlow
{
    class OffspringGenerator
    {
    protected:
        bool _inplace;
        ProblemHandle* _problem;

    public:
        OffspringGenerator(bool inplace) : _inplace(inplace), _problem(nullptr) {}

        virtual ~OffspringGenerator() {}

        virtual void setProblem(ProblemHandle* problem_handle) { _problem = problem_handle; }

        // 子代生成(原 O_Component::run 的核心动作;队列/编排由 v3.7 的 RunGenerate 包装传入)
        virtual void generate(Terminator* terminator_pointer, SolutionRepaireman*& repairman, LearningTopology*& ltopology, LearningStrategy*& lstrategy,
            IndividualArray* offspring_ptr, IndividualArray** parent, BestArchive** archive, int subswarm_number,
            std::queue<LearningStrategy*>& after_fes, std::queue<LearningStrategy*>& after_upd, LearningGraph*& last_graph) = 0;
    };

    // 整体生成 + 约束修复
    class GeneratorWithGenerationFramework final : public OffspringGenerator
    {
    public:
        GeneratorWithGenerationFramework(bool inplace = false) : OffspringGenerator(inplace) {}
        ~GeneratorWithGenerationFramework() {}

        void generate(Terminator* terminator_pointer, SolutionRepaireman*& repairman, LearningTopology*& ltopology, LearningStrategy*& lstrategy,
            IndividualArray* offspring_ptr, IndividualArray** parent, BestArchive** archive, int subswarm_number,
            std::queue<LearningStrategy*>& after_fes, std::queue<LearningStrategy*>& after_upd, LearningGraph*& last_graph) override
        {
            IndividualArray& offspring = *offspring_ptr;

            Individual* individual;
            Solution** learning_objects;
            LearningGraph* graph = ltopology->getTopology(parent, archive, subswarm_number, offspring_ptr, last_graph);

            int offspring_size = offspring.getSize();
            size_t graph_size = graph->getSize();
            if (!_inplace)
                offspring.extend(int(graph_size));

            lstrategy->preparation_s(*(parent[0]), terminator_pointer);

            int solution_size = offspring[0].getSolutionSize();
            Individual* child;

            for (int i = 0; i < (int)graph_size; i++)
            {
                individual = graph->getStartPoint(i);
                learning_objects = graph->getEndPoint(i);

                if (_inplace)
                {
                    if (individual->has_evaluted) continue;   // 已评估,不支持继续操作
                    else child = individual;
                }
                else
                {
                    child = &offspring[i + offspring_size];
                }

                if (learning_objects[0] == nullptr && graph->getEndSize() != 0) // 不学习,直接进入子代
                {
                    child->copy(*individual);
                    child->has_evaluted = true;
                    continue;
                }
                child->has_evaluted = false;
                if (child != individual) child->inheritFeaturesFrom(*individual);   // 继承 pbest 等记忆特性(父→子)

                lstrategy->preparation_i(individual, learning_objects, child); // 个体准备(部分策略会改内部变量内存)
                _problem->setResult(child->solution);
                _problem->constrainReset();

                lstrategy->getNewIndividual(child, individual, learning_objects, _problem);

                // 约束修复
                for (int d = 0; d < solution_size; d++)
                {
                    child->solution.result[d] = _problem->choiceDiscretized(d, child->solution.result[d]);
                    if (!_problem->constrainCheck(d, child->solution.result[d]))
                        child->solution.result[d] = repairman->repair(child->solution, d, _problem);
                    _problem->constrainChange(d, child->solution.result[d]);
                }
            }

            after_fes.push(lstrategy);
            after_upd.push(lstrategy);
            delete last_graph;
            last_graph = graph;
        }
    };

    // 整体生成,无约束修复
    class GeneratorWithNoCheckGeneration final : public OffspringGenerator
    {
    public:
        GeneratorWithNoCheckGeneration(bool inplace = false) : OffspringGenerator(inplace) {}
        ~GeneratorWithNoCheckGeneration() {}

        void generate(Terminator* terminator_pointer, SolutionRepaireman*& repairman, LearningTopology*& ltopology, LearningStrategy*& lstrategy,
            IndividualArray* offspring_ptr, IndividualArray** parent, BestArchive** archive, int subswarm_number,
            std::queue<LearningStrategy*>& after_fes, std::queue<LearningStrategy*>& after_upd, LearningGraph*& last_graph) override
        {
            IndividualArray& offspring = *offspring_ptr;

            Individual* individual;
            Solution** learning_objects;
            LearningGraph* graph = ltopology->getTopology(parent, archive, subswarm_number, offspring_ptr, last_graph);

            int offspring_size = offspring.getSize();
            size_t graph_size = graph->getSize();
            if (!_inplace)
                offspring.extend(int(graph_size));

            lstrategy->preparation_s(*(parent[0]), terminator_pointer);

            for (int i = 0; i < (int)graph_size; i++)
            {
                individual = graph->getStartPoint(i);
                learning_objects = graph->getEndPoint(i);
                Individual* child;

                if (_inplace)
                {
                    if (individual->has_evaluted) continue;
                    else child = individual;
                }
                else
                {
                    child = &offspring[i + offspring_size];
                }

                if (learning_objects[0] == nullptr && graph->getEndSize() != 0)
                {
                    child->copy(*individual);
                    child->has_evaluted = true;
                    continue;
                }
                child->has_evaluted = false;
                if (child != individual) child->inheritFeaturesFrom(*individual);   // 继承 pbest 等记忆特性(父→子)

                lstrategy->preparation_i(individual, learning_objects, child);
                _problem->setResult(child->solution);
                lstrategy->getNewIndividual(child, individual, learning_objects, _problem);
            }

            after_fes.push(lstrategy);
            after_upd.push(lstrategy);
            delete last_graph;
            last_graph = graph;
        }
    };

    // 逐维顺序构造
    class GeneratorWithOrderedConstruct final : public OffspringGenerator
    {
    public:
        GeneratorWithOrderedConstruct(bool inplace = false) : OffspringGenerator(inplace) {}
        ~GeneratorWithOrderedConstruct() {}

        void generate(Terminator* terminator_pointer, SolutionRepaireman*& repairman, LearningTopology*& ltopology, LearningStrategy*& lstrategy,
            IndividualArray* offspring_ptr, IndividualArray** parent, BestArchive** archive, int subswarm_number,
            std::queue<LearningStrategy*>& after_fes, std::queue<LearningStrategy*>& after_upd, LearningGraph*& last_graph) override
        {
            IndividualArray& offspring = *offspring_ptr;

            Individual* individual;
            Solution** learning_objects;
            LearningGraph* graph = ltopology->getTopology(parent, archive, subswarm_number, offspring_ptr, last_graph);

            int offspring_size = offspring.getSize();
            size_t graph_size = graph->getSize();
            if (!_inplace)
                offspring.extend(int(graph_size));

            lstrategy->preparation_s(*(parent[0]), terminator_pointer);

            for (int i = 0; i < (int)graph_size; i++)
            {
                _problem->constrainReset();
                individual = graph->getStartPoint(i);
                learning_objects = graph->getEndPoint(i);

                Individual* child;
                if (_inplace)
                {
                    if (individual->has_evaluted) continue;
                    else child = individual;
                }
                else
                {
                    child = &offspring[i + offspring_size];
                }

                if (learning_objects[0] == nullptr && graph->getEndSize() != 0)
                {
                    child->copy(*individual);
                    child->has_evaluted = true;
                    continue;
                }
                child->has_evaluted = false;
                if (child != individual) child->inheritFeaturesFrom(*individual);   // 继承 pbest 等记忆特性(父→子)

                lstrategy->preparation_i(individual, learning_objects, child);
                _problem->setResult(child->solution);

                // 逐维构造
                for (int d = 0; d < child->getSolutionSize(); d++)
                {
                    lstrategy->preparation_d(d, individual, learning_objects, _problem, child);
                    child->solution.result[d] = lstrategy->nextDecision(d, individual, learning_objects, _problem, child);
                    child->solution.result[d] = _problem->choiceDiscretized(d, child->solution.result[d]);
                    if (!_problem->constrainCheck(d, child->solution.result[d]))
                        child->solution.result[d] = repairman->repair(child->solution, d, _problem);
                    _problem->constrainChange(d, child->solution.result[d]);
                    lstrategy->update_d(child, d);
                }
            }

            after_fes.push(lstrategy);
            after_upd.push(lstrategy);
            delete last_graph;
            last_graph = graph;
        }
    };

    // 逐维并行构造(每个体独立问题句柄)
    class GeneratorWithParallelConstruct final : public OffspringGenerator
    {
    protected:
        ProblemHandle** _problem_handles;
        int _handle_buffer_number;

        void extend_buffer(int size)
        {
            if (_handle_buffer_number >= size) return;

            ProblemHandle** _new_handles = new ProblemHandle * [size];
            memcpy(_new_handles, _problem_handles, sizeof(ProblemHandle*) * _handle_buffer_number);
            for (int i = _handle_buffer_number; i < size; i++)
                _new_handles[i] = new ProblemHandle(*_problem);

            delete[] _problem_handles;
            _problem_handles = _new_handles;
            _handle_buffer_number = size;
        }

        void delete_buffer()
        {
            for (int i = 0; i < _handle_buffer_number; i++)
                delete _problem_handles[i];
            delete[] _problem_handles;
            _handle_buffer_number = 0;
        }

    public:
        GeneratorWithParallelConstruct(bool inplace = false) : OffspringGenerator(inplace)
        {
            _problem_handles = nullptr;
            _handle_buffer_number = 0;
        }

        ~GeneratorWithParallelConstruct() { delete_buffer(); }

        void setProblem(ProblemHandle* problem_handle) override
        {
            delete_buffer();
            OffspringGenerator::setProblem(problem_handle);
        }

        void generate(Terminator* terminator_pointer, SolutionRepaireman*& repairman, LearningTopology*& ltopology, LearningStrategy*& lstrategy,
            IndividualArray* offspring_ptr, IndividualArray** parent, BestArchive** archive, int subswarm_number,
            std::queue<LearningStrategy*>& after_fes, std::queue<LearningStrategy*>& after_upd, LearningGraph*& last_graph) override
        {
            IndividualArray& offspring = *offspring_ptr;

            Individual* individual;
            Solution** learning_objects;
            LearningGraph* graph = ltopology->getTopology(parent, archive, subswarm_number, offspring_ptr, last_graph);

            int offspring_size = offspring.getSize();
            size_t graph_size = graph->getSize();
            if (!_inplace)
                offspring.extend(int(graph_size));

            lstrategy->preparation_s(*(parent[0]), terminator_pointer);

            // 每个体独立句柄 + 准备
            extend_buffer(int(graph_size));
            for (int i = 0; i < (int)graph_size; i++)
                _problem_handles[i]->constrainReset();
            for (int i = 0; i < (int)graph_size; i++)
            {
                individual = graph->getStartPoint(i);
                learning_objects = graph->getEndPoint(i);
                lstrategy->preparation_i(individual, learning_objects, &offspring[i]);
                _problem_handles[i]->setResult(offspring[i].solution);
            }

            // 逐维(外)× 个体(内)并行构造
            Individual* child;
            ProblemHandle* corr_handle;
            int solution_size = offspring[0].getSolutionSize();
            for (int d = 0; d < solution_size; d++)
            {
                for (int i = 0; i < (int)graph_size; i++)
                {
                    individual = graph->getStartPoint(i);
                    learning_objects = graph->getEndPoint(i);

                    if (_inplace)
                    {
                        if (individual->has_evaluted) continue;
                        else child = individual;
                    }
                    else
                    {
                        child = &offspring[i + offspring_size];
                    }

                    if (learning_objects[0] == nullptr && graph->getEndSize() != 0)
                    {
                        child->copy(*individual);
                        child->has_evaluted = true;
                        continue;
                    }
                    child->has_evaluted = false;
                    if (child != individual) child->inheritFeaturesFrom(*individual);   // 继承 pbest 等记忆特性(父→子)

                    corr_handle = _problem_handles[i];
                    lstrategy->preparation_d(d, individual, learning_objects, corr_handle, child);
                    child->solution.result[d] = lstrategy->nextDecision(d, individual, learning_objects, corr_handle, child);
                    child->solution.result[d] = corr_handle->choiceDiscretized(d, child->solution.result[d]);
                    if (!corr_handle->constrainCheck(d, child->solution.result[d]))
                        child->solution.result[d] = repairman->repair(child->solution, d, corr_handle);
                    corr_handle->constrainChange(d, child->solution.result[d]);
                    lstrategy->update_d(child, d);
                }
            }

            after_fes.push(lstrategy);
            after_upd.push(lstrategy);
            delete last_graph;
            last_graph = graph;
        }
    };

    // 自注册进 Registry<OffspringGenerator>(T_offspringgenerator)。参数:inplace(bool)。
    inline Registry<OffspringGenerator>::Entry generationEntry()
    {
        return { "Generation", ModuleType::T_offspringgenerator,
            ParameterTemplate{ { {"inplace", ParamKind::Enum, 0, 1, false, 0, 0} } }, sizeof(GeneratorWithGenerationFramework),
            [](const double* p) -> OffspringGenerator* { return p ? new GeneratorWithGenerationFramework(p[0] != 0) : new GeneratorWithGenerationFramework(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    inline Registry<OffspringGenerator>::Entry generationNoCheckEntry()
    {
        return { "GenerationNoCheck", ModuleType::T_offspringgenerator,
            ParameterTemplate{ { {"inplace", ParamKind::Enum, 0, 1, false, 0, 0} } }, sizeof(GeneratorWithNoCheckGeneration),
            [](const double* p) -> OffspringGenerator* { return p ? new GeneratorWithNoCheckGeneration(p[0] != 0) : new GeneratorWithNoCheckGeneration(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    inline Registry<OffspringGenerator>::Entry orderedConstructEntry()
    {
        return { "OrderedConstruct", ModuleType::T_offspringgenerator,
            ParameterTemplate{ { {"inplace", ParamKind::Enum, 0, 1, false, 0, 0} } }, sizeof(GeneratorWithOrderedConstruct),
            [](const double* p) -> OffspringGenerator* { return p ? new GeneratorWithOrderedConstruct(p[0] != 0) : new GeneratorWithOrderedConstruct(); },
            // preAssert:要求 constructive 策略(拼写 "constructive" 沿用原样,记 ASSERT-REVIEW)
            [](AssertList& L, const double*) { L.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::equal)); },
            [](AssertList&, const double*) {} };
    }
    inline Registry<OffspringGenerator>::Entry parallelConstructEntry()
    {
        return { "ParallelConstruct", ModuleType::T_offspringgenerator,
            ParameterTemplate{ { {"inplace", ParamKind::Enum, 0, 1, false, 0, 0} } }, sizeof(GeneratorWithParallelConstruct),
            [](const double* p) -> OffspringGenerator* { return p ? new GeneratorWithParallelConstruct(p[0] != 0) : new GeneratorWithParallelConstruct(); },
            [](AssertList& L, const double*) { L.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::equal)); },
            [](AssertList&, const double*) {} };
    }
    ECFLOW_REGISTER(gen_generation,        OffspringGenerator, generationEntry());
    ECFLOW_REGISTER(gen_generationnocheck, OffspringGenerator, generationNoCheckEntry());
    ECFLOW_REGISTER(gen_orderedconstruct,  OffspringGenerator, orderedConstructEntry());
    ECFLOW_REGISTER(gen_parallelconstruct, OffspringGenerator, parallelConstructEntry());
}
