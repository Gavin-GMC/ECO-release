//------------------------Description------------------------
// This file defines the learning strategy of de. 
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include"learning-strategy.h"
#include"ga-crossover.h"

namespace ECFC
{
	// Differential Evolution Algorithm for continues optimization
	class DEStrategy : public LearningStrategy
	{
	protected:
		Crossover* _mutation_func; // 基于交叉算子产生变异个体
		double _cross_rate;
		bool adaptive_cr;
		Individual* _best_solution;
		Individual* _worst_solution;

	public:
		DEStrategy(double cross_rate, Crossover* mutation)
			:LearningStrategy()
		{
			adaptive_cr = _cross_rate == EMPTYVALUE;
			_cross_rate = cross_rate;
			_mutation_func = mutation;
			_best_solution = nullptr;
			_worst_solution = nullptr;
		}

		~DEStrategy()
		{
			delete _mutation_func;
		}

		
		static void preAssert(AssertList& list, double* paras)
		{
			CrossoverType ctype = CrossoverType(paras[1]);
			AssertList* clist = CrossoverFactory::postAssert(ctype, paras + 2);
			for (int i = 0; i < clist->getSize(); i++)
			{
				list.add(&(*clist)[i]);
			}
			delete clist;
		}

		static void postAssert(AssertList& list, double* paras)
		{
			// none
		}

		void setProblem(ProblemHandle* problem_handle) 
		{
			_mutation_func->setProblem(problem_handle);
		}

		void preparation_s(Population& population)
		{
			_mutation_func->ini();

			if (adaptive_cr)
			{
				// 获取最好和最差个体
				_best_solution = &population[0];
				_worst_solution = &population[0];
				for (int i = 1; i < population.getSize(); i++)
				{
					if (population[i] < *_best_solution)
						_best_solution = &population[i];
					if (*_worst_solution < population[i])
						_worst_solution = &population[i];
				}
			}
		};

		virtual void getNewIndividual(Individual* child, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle)
		{
			_mutation_func->apply(&child->solution, &individual->solution, learning_object, problem_handle);

			if (adaptive_cr)
			{
				_cross_rate = 0.1 + 0.5 * (individual->solution.fitness[0] - _best_solution->solution.fitness[0])
					/ (_worst_solution->solution.fitness[0] - _best_solution->solution.fitness[0]);
			}

			for (int i = 0; i < child->getSolutionSize(); i++)
			{
				if (_cross_rate < rand01()) // 亲本个体以cr的概率与变异个体交叉
				{
					child->solution[i] = individual->solution[i];
				}
			}
		}
	};
}