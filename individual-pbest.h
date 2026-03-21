//------------------------Description------------------------
// This file defines individual objects with pbest parameter.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include"individual.h"

namespace ECFC
{
	class PbestIndividual : public Individual
	{		
	public:
		Solution pbest;

		PbestIndividual()
			:Individual()
		{
			pbest = Solution();
		}

		~PbestIndividual()
		{

		}

		static void preAssert(AssertList& list, double* paras)
		{
			Individual::preAssert(list, paras);
		}

		static void postAssert(AssertList& list, double* paras)
		{
			Individual::postAssert(list, paras);
			list.add(new Assert(ModuleType::T_individual, "pbest", 1, MatchType::postAssert));
		}

		void setProblem(ProblemHandle* problem_handle)
		{
			Individual::setProblem(problem_handle);
			pbest.setSize(solution);
		}

		void setProblem(const Individual& source)
		{
			Individual::setProblem(source);
			pbest.setSize(solution);
		}

		void ini(bool ini_solution = true, bool evaluate = true, bool ini_speciality = false)
		{
			Individual::ini(ini_solution, evaluate, ini_speciality);
			if (ini_speciality)
			{
				// pbest 初始化
				pbest.copy(solution);
			}
		}

		void copy(const Individual& copy_source)
		{
			Individual::copy(copy_source);
			Individual* p_buffer = const_cast<Individual*>(&copy_source);
			PbestIndividual* pointer = static_cast<PbestIndividual*> (p_buffer);
			pbest.copy(pointer->pbest);
		}

		void copy(const double* source_result, const double* source_fitness)
		{
			Individual::copy(source_result, source_fitness);
		}

		void shallowCopy(const Individual& copy_source)
		{
			Individual::shallowCopy(copy_source);
			Individual* p_buffer = const_cast<Individual*>(&copy_source);
			PbestIndividual* pointer = static_cast<PbestIndividual*> (p_buffer);
			pbest.shallowCopy(pointer->pbest);
		}

		void swap(Individual& copy_source)
		{
			Individual::swap(copy_source);
			PbestIndividual* pointer = static_cast<PbestIndividual*> (&copy_source);
			pbest.swap(pointer->pbest);
		}

		void pbestUpdate()
		{
			if (_comparer_pointer->isBetter(solution.fitness, pbest.fitness))
			{
				pbest.copy(solution);
			}
		}
	};
}