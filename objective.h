//------------------------Description------------------------
// This file defines the object in ECFC. They support fitness 
// evaluate of candidate solutions.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include<string>
#include"calculator.h"

namespace ECFC
{
	class Objective
	{
	private:
		std::string _name;
		bool _min_is_better;
		int _priority;
		Calculator* _calculator;

	public:
		Objective()
		{
			_name = "none";
			_priority = -1;
			_min_is_better = true;
			_calculator = nullptr;
		}

		Objective(std::string name, int priority, bool min_is_better)
		{
			_name = name;
			_priority = priority;
			_min_is_better = min_is_better;
			_calculator = nullptr;
		}

		~Objective()
		{
			delete _calculator;
		}

		bool IsMin() const
		{
			return _min_is_better;
		}

		std::string getName()
		{
			return _name;
		}

		void setPriority(int priority)
		{
			_priority = priority;
		}

		int priority() const
		{
			return _priority;
		}

		void setCalculator(Calculator* c_pointer)
		{
			_calculator = c_pointer;
		}

		double getFitness(double** solution)
		{
			double result;
			_calculator->run(solution, &result);
			return result;
		}

		void copy(Objective* source)
		{
			_name = source->_name;
			_min_is_better = source->_min_is_better;
			_priority = source->_priority;
			_calculator = source->_calculator->copy();
		}

		bool operator<(const Objective& a)const
		{
			return _priority > a._priority;
		}
	};
}