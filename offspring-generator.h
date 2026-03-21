//------------------------Description------------------------
// This file defines the offspring generation framework for individuals.
// Parental individuals in the population construct offspring individuals
// based on this framework
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include"solution.h"
#include"population.h"
#include"learning-topology.h"
#include"learning-strategy.h"
#include"problem-handle.h"
#include"best-archive.h"
#include"terminator.h"
#include"solution-repairman.h"

// Attention!，当前生成过程中未考虑终止条件，即即使条件触发也会完成当前的子代生成

namespace ECFC
{
	class OffspringGenerator
	{
	protected:
		ProblemHandle* _problem_handle;
		InSwarmTopology* _inswarm_topology;
		LearningStrategy* _learning_strategy;
		Terminator* _terminator_pointer;
		SolutionRepaireman* _repairman;

		void evaluated_solution(Solution& solution, BestArchive* archive_pointer)
		{
			bool best_solution_updated;
			_problem_handle->solutionEvaluate(solution);
			best_solution_updated = archive_pointer->updateBest(solution);
			_terminator_pointer->update(best_solution_updated);
		}

	public:
		OffspringGenerator(InSwarmTopology* topology, LearningStrategy* strategy, SolutionRepaireman* repaire)
		{
			_inswarm_topology = topology;
			_learning_strategy = strategy;
			_repairman = repaire;
			_problem_handle = nullptr;
			_terminator_pointer = nullptr;
		}

		virtual ~OffspringGenerator()
		{
			delete _inswarm_topology;
			delete _learning_strategy;
			delete _repairman;
			delete _problem_handle;
		}

		static void preAssert(AssertList& list, double* paras)
		{
			// none
		}

		static void postAssert(AssertList& list, double* paras)
		{
			// none
		}

		virtual void setTerminator(Terminator* pointer)
		{
			_terminator_pointer = pointer;
		}

		virtual void ini()
		{
			_inswarm_topology->ini();
			_learning_strategy->ini(_problem_handle);
		}

		virtual void setProblem(ProblemHandle* problem_handle)
		{
			delete _problem_handle;
			_problem_handle = new ProblemHandle(*problem_handle);
			_learning_strategy->setProblem(problem_handle);
		}

		virtual void getOffspring(Population& offspring, Population** parent, BestArchive** archive, int subswarm_number = 1) = 0;

		virtual void update(Population& population, Population& offspring)
		{
			_learning_strategy->update_s(population, offspring);
		}

		void changeEnv(const double* values, int variableId)
		{
			_problem_handle->changeEnv(values, variableId);
		}
	};

	class GeneratorWithGenerationFramework final : public OffspringGenerator
	{
	public:
		GeneratorWithGenerationFramework(InSwarmTopology* topology, LearningStrategy* strategy, SolutionRepaireman* repaire)
			:OffspringGenerator(topology, strategy, repaire)
		{

		}

		~GeneratorWithGenerationFramework()
		{

		}

		static void preAssert(AssertList& list, double* paras)
		{
			// none
		}

		static void postAssert(AssertList& list, double* paras)
		{
			// none
		}

		void getOffspring(Population& offspring, Population** parent, BestArchive** archive, int subswarm_number = 1)
		{
			Individual* individual;
			Solution** learning_objects;
			bool best_solution_updated;
			LearningGraph* graph = _inswarm_topology->getTopology(parent, archive, subswarm_number);

			size_t graph_size = graph->getSize();
			if (offspring.getSize() < graph_size)
			{
				offspring.extend(int(graph_size));
			}

			_learning_strategy->preparation_s(*(parent[0]));

			// 生成准备
			int solution_size = offspring[0].getSolutionSize();
			Individual* child;

			for (int i = 0; i < graph_size; i++)
			{
				// 生成准备
				individual = graph->getStartPoint(i);
				learning_objects = graph->getEndPoint(i);
				child = &offspring[i];

				if (learning_objects[0] == nullptr && graph->getEndSize() != 0) // 不学习，直接进入子代
				{
					child->copy(*individual);
					continue;
				}

				// 生成准备
				_learning_strategy->preparation_i(individual, learning_objects, child); // 先进行个体准备是因为该过程中，部分策略可能改变内部的变量内存，导致链接错误
				_problem_handle->setResult(child->solution); // 将候选解与句柄进行链接
				_problem_handle->constrainReset();

				// 子代生成
				_learning_strategy->getNewIndividual(child, individual, learning_objects, _problem_handle);

				// 约束修复
				for (int d = 0; d < solution_size; d++)
				{
					// 将决策值转化为预设的精度
					child->solution.result[d] = _problem_handle->choiceDiscretized(d, child->solution.result[d]);
					// 约束修复
					if (!_problem_handle->constrainCheck(d, child->solution.result[d]))
					{
						child->solution.result[d] = _repairman->repair(child->solution, d, _problem_handle);
					}
					_problem_handle->constrainChange(d, child->solution.result[d]);			
				}
				// 子代评估
				_problem_handle->solutionEvaluate(child->solution);
				best_solution_updated = archive[0]->updateBest(child->solution);
				
				_learning_strategy->update_i(child);
				_terminator_pointer->update(best_solution_updated);
			}

			delete graph;
		}
	};

	class GeneratorWithNoCheckGeneration final : public OffspringGenerator
	{
	public:
		GeneratorWithNoCheckGeneration(InSwarmTopology* topology, LearningStrategy* strategy, SolutionRepaireman* repaire)
			:OffspringGenerator(topology, strategy, repaire)
		{

		}

		~GeneratorWithNoCheckGeneration()
		{

		}

		static void preAssert(AssertList& list, double* paras)
		{
			// none
		}

		static void postAssert(AssertList& list, double* paras)
		{
			// none
		}

		void getOffspring(Population& offspring, Population** parent, BestArchive** archive, int subswarm_number = 1)
		{
			Individual* individual;
			Solution** learning_objects;
			bool best_solution_updated;
			LearningGraph* graph = _inswarm_topology->getTopology(parent, archive, subswarm_number);

			size_t graph_size = graph->getSize();
			if (offspring.getSize() < graph_size)
				offspring.extend(int(graph_size));

			_learning_strategy->preparation_s(*(parent[0]));

			for (int i = 0; i < graph_size; i++)
			{
				// 生成准备
				individual = graph->getStartPoint(i);
				learning_objects = graph->getEndPoint(i);
				Individual* child = &offspring[i];

				if (learning_objects[0] == nullptr && graph->getEndSize() != 0) // 不学习，直接进入子代
				{
					offspring[i].copy(*individual);
					continue;
				}

				// 生成准备
				_learning_strategy->preparation_i(individual, learning_objects, child); // 先进行个体准备是因为该过程中，部分策略可能改变内部的变量内存，导致链接错误
				_problem_handle->setResult(child->solution); // 将候选解与句柄进行链接

				// 子代生成
				_learning_strategy->getNewIndividual(child, individual, learning_objects, _problem_handle);

				// 子代评估
				_problem_handle->solutionEvaluate(child->solution);
				best_solution_updated = archive[0]->updateBest(offspring[i].solution);
				
				// 评估后相关组件的更新
				_learning_strategy->update_i(&offspring[i]);
				_terminator_pointer->update(best_solution_updated);
			}

			delete graph;
		}
	};

	class GeneratorWithOrderedConstruct final : public OffspringGenerator
	{
	public:
		GeneratorWithOrderedConstruct(InSwarmTopology* topology, LearningStrategy* strategy, SolutionRepaireman* repaire)
			:OffspringGenerator(topology, strategy, repaire)
		{

		}

		~GeneratorWithOrderedConstruct()
		{

		}

		static void preAssert(AssertList& list, double* paras)
		{
			list.add(new Assert(ModuleType::T_learnstrategy, "constrcutive", 1, MatchType::equal));
		}

		static void postAssert(AssertList& list, double* paras)
		{
			// none
		}

		void getOffspring(Population& offspring, Population** parent, BestArchive** archive, int subswarm_number = 1)
		{
			Individual* individual;
			Solution** learning_objects;
			bool best_solution_updated;
			LearningGraph* graph = _inswarm_topology->getTopology(parent, archive, subswarm_number);

			int graph_size = graph->getSize();
			if (offspring.getSize() < graph_size)
				offspring.extend(graph_size);

			_learning_strategy->preparation_s(*(parent[0]));

			for (int i = 0; i < graph_size; i++)
			{
				// 生成准备
				_problem_handle->constrainReset();
				individual = graph->getStartPoint(i);
				learning_objects = graph->getEndPoint(i);

				// 个体直接进入子代
				if (learning_objects[0] == nullptr && graph->getEndSize() != 0)
				{
					offspring[i].copy(*individual);
					continue;
				}

				// 生成准备
				Individual* child = &offspring[i];
				_learning_strategy->preparation_i(individual, learning_objects, child); // 先进行个体准备是因为该过程中，部分策略可能改变内部的变量内存，导致链接错误
				_problem_handle->setResult(child->solution); // 将候选解与句柄进行链接

				// 子代生成
				for (int d = 0; d < child->getSolutionSize(); d++)
				{
					_learning_strategy->preparation_d(d, individual, learning_objects, _problem_handle, child);

					child->solution.result[d] = _learning_strategy->nextDecision(d, individual, learning_objects, _problem_handle, child);
					
					// 将决策值转化为预设的精度
					child->solution.result[d] = _problem_handle->choiceDiscretized(d, child->solution.result[d]);
					// 约束修复
					if (!_problem_handle->constrainCheck(d, child->solution.result[d]))
					{
						child->solution.result[d] = _repairman->repair(child->solution, d, _problem_handle);
					}

					_problem_handle->constrainChange(d, child->solution.result[d]);
					_learning_strategy->update_d(child, d);
				}

				// 子代评估
				_problem_handle->solutionEvaluate(child->solution);
				best_solution_updated = archive[0]->updateBest(child->solution);

				// 评估后相关组件的更新
				_learning_strategy->update_i(child);
				_terminator_pointer->update(best_solution_updated);
			}

			delete graph;
		}
	};

	class GeneratorWithParallelConstruct final : public OffspringGenerator
	{
	protected:
		ProblemHandle** _problem_handles;
		int _handle_buffer_number;

		void extend_buffer(int size)
		{
			if (_handle_buffer_number >= size)
				return;

			ProblemHandle** _new_handles = new ProblemHandle*[size];
			memcpy(_new_handles, _problem_handles, sizeof(ProblemHandle*) * _handle_buffer_number);

			for (int i = _handle_buffer_number; i < size; i++)
			{
				_new_handles[i] = new ProblemHandle(*_problem_handle);
			}

			delete[] _problem_handles;
			_problem_handles = _new_handles;
			_handle_buffer_number = size;
		}

		void delete_buffer()
		{
			for (int i = 0; i < _handle_buffer_number; i++)
			{
				delete _problem_handles[i];
			}
			delete[] _problem_handles;
			_handle_buffer_number = 0;
		}

	public:
		GeneratorWithParallelConstruct(InSwarmTopology* topology, LearningStrategy* strategy, SolutionRepaireman* repaire)
			:OffspringGenerator(topology, strategy, repaire)
		{
			_problem_handles = nullptr;
			_handle_buffer_number = 0;
		}

		~GeneratorWithParallelConstruct()
		{
			delete_buffer();
		}

		static void preAssert(AssertList& list, double* paras)
		{
			list.add(new Assert(ModuleType::T_learnstrategy, "constrcutive", 1, MatchType::equal));
		}

		static void postAssert(AssertList& list, double* paras)
		{
			// none
		}

		void setProblem(ProblemHandle* problem_handle)
		{
			delete_buffer();
			OffspringGenerator::setProblem(problem_handle);
		}

		void getOffspring(Population& offspring, Population** parent, BestArchive** archive, int subswarm_number = 1)
		{
			Individual* individual;
			Solution** learning_objects;
			bool best_solution_updated;
			LearningGraph* graph = _inswarm_topology->getTopology(parent, archive, subswarm_number);

			int graph_size = graph->getSize();
			if (offspring.getSize() < graph_size)
				offspring.extend(graph_size);

			_learning_strategy->preparation_s(*(parent[0]));

			// 生成准备
			extend_buffer(graph_size);
			for (int i = 0; i < graph_size; i++)
			{
				_problem_handles[i]->constrainReset();
			}
			for (int i = 0; i < graph_size; i++)
			{
				individual = graph->getStartPoint(i);
				learning_objects = graph->getEndPoint(i);
				_learning_strategy->preparation_i(individual, learning_objects, &offspring[i]);
				_problem_handles[i]->setResult(offspring[i].solution);
			}

			// 子代生成
			Individual* child;
			ProblemHandle* corr_handle;
			int solution_size = offspring[0].getSolutionSize();
			for (int d = 0; d < solution_size; d++)
			{
				for (int i = 0; i < graph_size; i++)
				{
					// 生成准备
					individual = graph->getStartPoint(i);
					learning_objects = graph->getEndPoint(i);

					// 个体直接进入子代
					if (learning_objects[0] == nullptr && graph->getEndSize() != 0)
					{
						continue;
					}

					// 生成准备
					child = &offspring[i];
					corr_handle = _problem_handles[i];
					_learning_strategy->preparation_d(d, individual, learning_objects, corr_handle, child);

					// 更新下一维
					child->solution.result[d] = _learning_strategy->nextDecision(d, individual, learning_objects, corr_handle, child);
					
					// 将决策值转化为预设的精度
					child->solution.result[d] = corr_handle->choiceDiscretized(d, child->solution.result[d]);
					// 约束修复
					if (!corr_handle->constrainCheck(d, child->solution.result[d]))
					{
						child->solution.result[d] = _repairman->repair(child->solution, d, corr_handle);
					}

					// 更新约束状态和学习策略
					corr_handle->constrainChange(d, child->solution.result[d]);
					_learning_strategy->update_d(child, d);
				}
			}

			// 子代评估
			for (int i = 0; i < graph_size; i++)
			{
				individual = graph->getStartPoint(i);
				learning_objects = graph->getEndPoint(i);
				if (learning_objects[0] == nullptr)
					offspring[i].copy(*individual);
				else
				{
					_problem_handle->solutionEvaluate(offspring[i].solution);
					best_solution_updated = archive[0]->updateBest(offspring[i].solution);
					_terminator_pointer->update(best_solution_updated);
				}

				_learning_strategy->update_i(&offspring[i]);
			}

			delete graph;
		}
	};
}