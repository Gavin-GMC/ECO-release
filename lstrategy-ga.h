//------------------------Description------------------------
// This file defines the learning strategy of ga. It realizes
// the generation of child individuals through crossover and 
// mutation operators.
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
#include"crossover-factory.h"
#include"mutation-factory.h"

namespace ECFC
{
	// 候选解生成情景：
	// 生成式：直接产生整个解，
	// 顺序构造式：逐维产生整个解，需要记录是否进行交叉变异，交叉变异参数等信息
	// 并行构造式：每个解依次生成一个维度，需要记录整个种群的上述信息
	// 
	// 生成式和构造式结果的差异：
	// 交叉过程中不存在约束检查和应用的环节，而变异中存在
	// 考虑构造式会极大增加框架和算子的设计难度
	// 同时，部分算子并不支持按位运算
	// 
	// 结论：只支持生成式，不提供构造式按位生成的方法，可以考虑后续的添加？
	// 


	// 包含交叉和变异的遗传算法学习策略框架
	class GAStrategy : public LearningStrategy
	{
	private:
		Mutation* _mutation_func;
		Crossover* _crossover_func;
	public:
		GAStrategy(Mutation* mutation_func, Crossover* crossover_func)
			:LearningStrategy()
		{
			_mutation_func = mutation_func;
			_crossover_func = crossover_func;
		}

		~GAStrategy()
		{
			delete _mutation_func;
			delete _crossover_func;
		}

		static void preAssert(AssertList& list, double* paras)
		{
			CrossoverType ctype = CrossoverType(paras[0]);
			AssertList* clist = CrossoverFactory::preAssert(ctype, paras + 1);
			for (int i = 0; i < clist->getSize(); i++)
			{
				list.add(&(*clist)[i]);
			}
			delete clist;

			MutationType mtype = MutationType(paras[SUBPARA]);
			AssertList* mlist = MutationFactory::preAssert(mtype, paras + 1 + SUBPARA);
			for (int i = 0; i < mlist->getSize(); i++)
			{
				list.add(&(*mlist)[i]);
			}
			delete mlist;
		}

		static void postAssert(AssertList& list, double* paras)
		{
			CrossoverType ctype = CrossoverType(paras[0]);
			AssertList* clist = CrossoverFactory::postAssert(ctype, paras + 1);
			for (int i = 0; i < clist->getSize(); i++)
			{
				list.add(&(*clist)[i]);
			}
			delete clist;

			MutationType mtype = MutationType(paras[SUBPARA]);
			AssertList* mlist = MutationFactory::postAssert(mtype, paras + 1 + SUBPARA);
			for (int i = 0; i < mlist->getSize(); i++)
			{
				list.add(&(*mlist)[i]);
			}
			delete mlist;
		}

		void setProblem(ProblemHandle* problem_handle)
		{
			LearningStrategy::setProblem(problem_handle);
			_crossover_func->setProblem(problem_handle);
			_mutation_func->setProblem(problem_handle);
		}

		void preparation_s(Population& population)
		{
			_crossover_func->ini();
		};

		void getNewIndividual(Individual* child, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle)
		{
			problem_handle->constrainReset();
			_crossover_func->apply(&child->solution, &individual->solution, learning_object, problem_handle);
			_mutation_func->apply(&child->solution, problem_handle);
		}
	};
}