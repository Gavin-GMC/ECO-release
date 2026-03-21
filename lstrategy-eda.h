//------------------------Description------------------------
// This file defines the learning strategy of eda. It calculates 
// the distribution of individuals in the current population and 
// generates offspring individuals based on this probabilistic model.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include"ecfc-constant.h"
#include"solution.h"
#include"learning-strategy.h"
#include"distribution-factory.h"

namespace ECFC
{
	// 分布估计算法
	class EDAStrategy : public LearningStrategy
	{
	private:
		DistributionType _dm_type;
		DistributionModel** _models; // 每个维度一个分布模型
		int _demensions;
		int _good_number;
		double _good_rate;

		void _deleteModels()
		{
			for (int i = 0; i < _demensions; i++)
			{
				delete _models[i];
			}
			delete[] _models;
		}

	public:
		EDAStrategy(DistributionType model = DistributionType::F_Gaussian, int good_number = EMPTYVALUE, double good_rate = 0.5)
			:LearningStrategy()
		{
			_dm_type = model;
			_good_number = good_number;
			_good_rate = good_rate;

			_models = nullptr;
			_demensions = 0;
		}

		~EDAStrategy()
		{
			_deleteModels();
		}

		static void preAssert(AssertList& list, double* paras)
		{
			list.add(new Assert(ModuleType::T_learntopology, "objects", 0, MatchType::notLessButNotice)); // 需要0个学习目标
		}

		static void postAssert(AssertList& list, double* paras)
		{
			// none
		}

		void setProblem(ProblemHandle* problem_handle)
		{
			if (problem_handle->getProblemSize() != _demensions)
			{
				delete[] _models;
				_demensions = problem_handle->getProblemSize();
				_models = DistributionFactory::newModelArray(_dm_type, _demensions);
			}
		}

		void preparation_s(Population& population)
		{
			population.sort();

			for (int i = 0; i < _demensions; i++)
				_models[i]->clear();

			int good_number = _good_number;
			if (_good_number == EMPTYVALUE)
				good_number = _good_rate * population.getSize();

			for (int i = 0; i < _demensions; i++)
			{
				for (int j = 0; j < good_number; j++)
				{
					_models[i]->addSample(population[j].solution.result[i]);
				}
				_models[i]->build();
			}
		}

		double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual* child)
		{
			return _models[decision_d]->getValue();
		}
	};
}