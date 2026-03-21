//------------------------Description------------------------
// This file defines the learning strategy of ga. It realizes
// the generation of child individuals through velocity update
// and position update.
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

namespace ECFC
{
	// Classic Particle Swarm Optimizer
	class PSOStrategy : public LearningStrategy
	{
	private:
		int _object_number;
		double* _c;
		double _w_ini;
		double _w;

		double _w_attenuation;

	public:
		PSOStrategy(int object_number = 2, double c = 2, double w_ini = 0.9, double w_attenuation = EMPTYVALUE)
			:LearningStrategy()
		{
			_object_number = object_number;
			_c = new double[object_number];
			for (int i = 0; i < object_number; i++)
				_c[i] = c;

			_w_ini = w_ini;
			_w_attenuation = w_attenuation;
			_w = 0;
		}

		PSOStrategy(int object_number, double* c, double w_ini = 0.9, double w_attenuation = EMPTYVALUE)
			:LearningStrategy()
		{
			_object_number = object_number;
			_c = new double[object_number];
			memcpy(_c, c, sizeof(double) * object_number);

			_w_ini = w_ini;
			_w_attenuation = w_attenuation;
			_w = 0;
		}

		~PSOStrategy()
		{
			delete[] _c;
		}

		static void preAssert(AssertList& list, double* paras)
		{
			list.add(new Assert(ModuleType::T_learntopology, "objects", int(paras[0]), MatchType::notLessButNotice)); // 需要2个学习目标
			list.add(new Assert(ModuleType::T_individual, "velocity", 1, MatchType::equal)); // 需要速度向量
		}

		static void postAssert(AssertList& list, double* paras)
		{
			// none
		}

		void ini(ProblemHandle* problem_handle)
		{
			_w = _w_ini;
		}

		void preparation_s(Population& population)
		{
			if (_w_attenuation != EMPTYVALUE)
			{
                _w -= _w_attenuation;
			}

			// for(int i=0;i<_object_number;i++)
				// _r[i] = rand01_();
		}

		void preparation_i(Individual* individual, Solution** learning_object, Individual* child)
		{
			Particle* p_pointer = static_cast<Particle*> (individual);
			Particle* c_pointer = static_cast<Particle*> (child);

			// 速度更新
			double v;
			int problem_size = individual->getSolutionSize();
			for (int d = 0; d < problem_size; d++)
			{
                if(_w_attenuation == EMPTYVALUE)
                    v = rand01_() * p_pointer->velocity[d];
				else
                    v = _w * p_pointer->velocity[d];
                
				for (int i = 0; i < _object_number; i++)
				{
                    double a = ((*learning_object[i])[d] - (*p_pointer)[d]);
					// v += _c[i] * _r[i] * ((*learning_object[i])[d] - (*p_pointer)[d]);
                    v += _c[i] * rand01_() * ((*learning_object[i])[d] - (*p_pointer)[d]);
				}
				c_pointer->velocity[d] = v;
			}
		}

		double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual* child)
		{
			Particle* c_pointer = static_cast<Particle*> (child);

			// 位置更新
			return (*individual)[decision_d] + c_pointer->velocity[decision_d];
		}
	};
}