//------------------------Description------------------------
// This file defines the learning strategy of bpso. It designs 
// velocity update and position update based on binary coding.
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
#include"individual-particle.h"
#include"learning-strategy.h"
#include"basicfunc.h"

namespace ECFC
{
	// Binary Particle Swarm Optimization for binary optimization
	class BPSOStrategy : public LearningStrategy
	{
	private:
		double _c1, _c2;
		double _w, _r1, _r2;

		double _w_attenuation;

	public:
		BPSOStrategy(double c1 = 2, double c2 = 2, double w_ini = 0.9, double w_attenuation = EMPTYVALUE)
			:LearningStrategy()
		{
			_c1 = c1;
			_c2 = c2;
			_w = w_ini;
			_w_attenuation = w_attenuation;
		}

		~BPSOStrategy()
		{

		}

		static void preAssert(AssertList& list, double* paras)
		{
			list.add(new Assert(ModuleType::T_learntopology, "objects", 2, MatchType::notLessButNotice)); // 需要2个学习目标
			list.add(new Assert(ModuleType::T_individual, "velocity", 1, MatchType::equal)); // 需要速度向量
		}

		static void postAssert(AssertList& list, double* paras)
		{
			// none
		}

		void preparation_s(Population& population)
		{
			if (_w_attenuation == EMPTYVALUE)
			{
				_w = rand01_();
			}
			else
			{
				_w -= EMPTYVALUE;
			}
			_r1 = rand01_();
			_r2 = rand01_();
		}

		double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual* child)
		{
			Individual* p_buffer = const_cast<Individual*>(individual);
			Particle* pointer = static_cast<Particle*> (p_buffer);

			// 速度更新
			pointer->velocity[decision_d] = _w * pointer->velocity[decision_d] +
				_c1 * _r1 * ((*learning_object[0])[decision_d] - (*pointer)[decision_d]) +
				_c2 * _r2 * ((*learning_object[1])[decision_d] - (*pointer)[decision_d]);

			// 位置更新
			if (rand01() < sigmoid(pointer->velocity[decision_d]))
				return 1;
			else
				return 0;
		}
	};
}