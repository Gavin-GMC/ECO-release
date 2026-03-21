//------------------------Description------------------------
// This file defines the learning strategy of individuals, which
// means how the individual learns from the goal and produces
// the offspring.
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
#include"individual.h"
#include"population.h"
#include"problem-handle.h"

namespace ECFC
{
	class LearningStrategy
	{
	public:
		LearningStrategy() {}

		virtual ~LearningStrategy()
		{

		}

		// 策略的初始化
		virtual void ini(ProblemHandle* problem_handle) {};

		virtual void setProblem(ProblemHandle* problem_handle) {}

		// 在每一代开始前的准备工作
		virtual void preparation_s(Population& population) {};

		// 在每个个体开始前的准备工作
		virtual void preparation_i(Individual* individual, Solution** learning_object, Individual* child){}

		// 在每一维开始前的准备工作
		virtual void preparation_d(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual* child) {}

		// 根据学习对象获得下一维的值
		virtual double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual* child)
		{
			// 报错
			sys_logger.error("Current learning strategy does not support dimensional-by-dimension construction!");
			// 退出
			exit(-1);
		}

		// 在每一维生成后的更新工作
		virtual void update_d(Individual* child, const int decision_d) {};

		// 在每个个体生成并评估后的更新工作
		virtual void update_i(Individual* child) {};

		// 在子代生成并选择后的更新工作
		virtual void update_s(Population& population, Population& offspring) {};

		virtual void getNewIndividual(Individual* child, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle)
		{
			preparation_i(individual, learning_object, child);
			for (int i = 0; i < child->getSolutionSize(); i++)
			{
				preparation_d(i, individual, learning_object, problem_handle, child);
				child->solution.result[i] = nextDecision(i, individual, learning_object, problem_handle, child);
				update_d(child, i);
			}
		}
	};
}