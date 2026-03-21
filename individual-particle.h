//------------------------Description------------------------
// This file defines individual object with pbest and velocity parameter,
// which is used in particle swarm optimzer.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include"individual-pbest.h"


namespace ECFC
{
	class Particle : public PbestIndividual
	{
	public:
		double* velocity;

		Particle()
			:PbestIndividual()
		{
			pbest = Solution();
			velocity = nullptr;
		}

		~Particle()
		{
			delete[]velocity;
		}

		static void preAssert(AssertList& list, double* paras)
		{
			PbestIndividual::preAssert(list, paras);
		}

		static void postAssert(AssertList& list, double* paras)
		{
			PbestIndividual::postAssert(list, paras);
			list.add(new Assert(ModuleType::T_individual, "velocity", 1, MatchType::postAssert));
		}

		void setProblem(ProblemHandle* problem_handle)
		{
			PbestIndividual::setProblem(problem_handle);
			delete[] velocity;
			velocity = new double[solution.getSolutionSize() + 1];
		}

		void setProblem(const Individual& source)
		{
			PbestIndividual::setProblem(source);
            delete[] velocity;
			velocity = new double[solution.getSolutionSize() + 1];
		}

		void ini(bool ini_solution = true, bool evaluate = true, bool ini_speciality = false)
		{
			PbestIndividual::ini(ini_solution, evaluate, ini_speciality);
			// 速度初始化
			if (ini_speciality)
			{
				// 速度向量随机初始化
				for (int i = 0; i < solution.getSolutionSize(); i++)
				{
					// velocity[i] = _initializer_pointer->getRandomValue(i);
                    velocity[i] = 0;
				}
			}
		}

		void copy(const Individual& copy_source)
		{
			PbestIndividual::copy(copy_source);
			Individual* p_buffer = const_cast<Individual*>(&copy_source);
			Particle* pointer = static_cast<Particle*> (p_buffer);
			memcpy(velocity, pointer->velocity, sizeof(double) * solution.getSolutionSize());
		}

		void copy(const double* source_result, const double* source_fitness)
		{
			PbestIndividual::copy(source_result, source_fitness);
		}

		void shallowCopy(const Individual& copy_source)
		{
			PbestIndividual::shallowCopy(copy_source);
			Individual* p_buffer = const_cast<Individual*>(&copy_source);
			Particle* pointer = static_cast<Particle*> (p_buffer);

			delete[] velocity; // 防止内存泄露
			velocity = pointer->velocity;
		}

		void swap(Individual& copy_source)
		{
			PbestIndividual::swap(copy_source);
			Individual* p_buffer = const_cast<Individual*>(&copy_source);
			Particle* pointer = static_cast<Particle*> (p_buffer);
			std::swap(velocity, pointer->velocity);
		}
	};
}