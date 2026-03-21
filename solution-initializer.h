//------------------------Description------------------------
// This file defines the solution initializer, which provide 
// different approach to generate the initial candidate solution.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include"problem-handle.h"
#include"distribution-factory.h"
#include"solution.h"

namespace ECFC
{
	// 解初始器的模板基类
	class SolutionInitializer
	{
	protected:
		ProblemHandle* _problem_handle;
		bool _evaluate;
	public:
		SolutionInitializer(bool evaluate)
		{
			_problem_handle = nullptr;
			_evaluate = evaluate;
		}

		virtual ~SolutionInitializer()
		{
			delete _problem_handle;
		}

		virtual void ini_solution(double* solution, const int size) = 0;

		void ini_solution(Solution& solution)
		{
			ini_solution(solution.result, solution.getSolutionSize());
			
			if (_evaluate)
				_problem_handle->solutionEvaluate(solution);
			else
				solution.fitness[0] = EMPTYVALUE; // 设置未评估标签
		}

		virtual void setProblem(ProblemHandle* problem_handle)
		{
			delete _problem_handle;
			_problem_handle = new ProblemHandle(*problem_handle);
		}

		double getRandomValue(int demension_id)
		{
			return _problem_handle->getRandomChoice(demension_id);
		}
	};

	// 随机初始化器
	class RandomInitializer final : public SolutionInitializer
	{
	public:
		RandomInitializer()
			: SolutionInitializer(true)
		{

		}

		~RandomInitializer() {}

		void ini_solution(double* solution, const int size)
		{
			_problem_handle->constrainReset();
			for (int i = 0; i < size; i++)
			{
				solution[i] = _problem_handle->getRandomChoiceInspace(i);
				_problem_handle->constrainChange(i, solution[i]);
			}
		}
	};

	// 贪心初始化器
	class GreedyInitializer final : public SolutionInitializer
	{
	public:
		GreedyInitializer()
			: SolutionInitializer(true)
		{

		}

		~GreedyInitializer() {};

		void ini_solution(double* solution, const int size)
		{

			_problem_handle->setResult(solution);
			_problem_handle->constrainReset();			

			for (int i = 0; i < size; i++)
			{
				solution[i] = _problem_handle->getPrioriChoice(i);
				_problem_handle->constrainChange(i, solution[i]);
			}
		}
	};

	// 带随机概率的贪心初始化器
	class R_GInitializer final : public SolutionInitializer
	{
	private:
		double _greedy_posibility;
	public:
		R_GInitializer(int posibility)
			:SolutionInitializer(true)
		{
			_greedy_posibility = posibility;
		}

		~R_GInitializer() {}

		void ini_solution(double* solution, const int size)
		{
			_problem_handle->constrainReset();
			for (int i = 0; i < size; i++)
			{
				if (rand01() < _greedy_posibility)
				{
					//solution[i] = _problem_handle->getPrioriChoice(i);
				}
				else
				{
					solution[i] = _problem_handle->getRandomChoiceInspace(i);
				}
				_problem_handle->constrainChange(i, solution[i]);
			}
		}
	};

	// 基于分布的初始化器
	class DistributionInitializer final : public SolutionInitializer
	{
	private:
		int _demensions;
		DistributionModel** _models;

		void _deleteModels()
		{
			for (int i = 0; i < _demensions; i++)
			{
				delete _models[i];
			}
			delete[] _models;
		}

	public:
		DistributionInitializer(DistributionType model_type, int problem_size)
			:SolutionInitializer(true)
		{
			_models = DistributionFactory::newModelArray(model_type, problem_size);
			_demensions = problem_size;
		}

		~DistributionInitializer()
		{
			_deleteModels();
		}

		void ini_solution(double* solution, const int size)
		{
			_problem_handle->constrainReset();
			for (int i = 0; i < size; i++)
			{
				solution[i] = _models[i]->getValue();
				if (_problem_handle->constrainCheck(i, solution[i]))
				{
					solution[i] = _problem_handle->getCloseChoice(i, solution[i]);
				}
				_problem_handle->constrainChange(i, solution[i]);
			}
		}
	};

	// 不初始化器
	class NoInitializer final : public SolutionInitializer
	{
	public:
		NoInitializer()
			: SolutionInitializer(false)
		{

		}

		~NoInitializer() {}

		void ini_solution(double* solution, const int size)
		{
			
		}
	};
}
