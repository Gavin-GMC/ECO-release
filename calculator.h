//------------------------Description------------------------
// This file defines the calculation process object in ECFC,
// which is used to encapsulate and provide a uniform interface
// for different forms of computing processes
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------
#pragma once
#include"ecfc-functor.h"

namespace ECFC
{
	// 计算量的计算式（支持计算树和函数指针两种形式）
	class Calculator
	{
	protected:
		int _parameter_number;

	public:
		Calculator(int parameter_number)
		{
			_parameter_number = parameter_number;
		}

		virtual ~Calculator()
		{

		}

		// 基于输入值计算，并将结果记录
		virtual void run(double** input, double* output) = 0;

		int getParameterNumber()
		{
			return _parameter_number;
		}

		virtual Calculator* copy() = 0;
	};

	// 基于函数指针的计算器
	class FuncCalculator : public Calculator
	{
	private:
		double (*_function)(double** input);

	public:
		FuncCalculator(double (*function)(double** input), int parameter_number)
			:Calculator(parameter_number)
		{
			_function = function;
		}

		~FuncCalculator()
		{

		}

		void run(double** input, double* output)
		{
			*output = _function(input);
		}

		Calculator* copy()
		{
			return new FuncCalculator(_function, _parameter_number);
		}
	};

	// 基于仿函数的计算器
	class FunctorCalculator : public Calculator
	{
	private:
		eccalcul_functor* _function;

	public:
		FunctorCalculator(eccalcul_functor* function, int parameter_number)
			:Calculator(parameter_number)
		{
			_function = function->copy();
		}

		~FunctorCalculator()
		{
			delete _function;
		}

		void run(double** input, double* output)
		{
			*output = (*_function)(input);
		}

		Calculator* copy()
		{
			return new FunctorCalculator(_function, _parameter_number);
		}
	};

	//基于计算树的计算器
	class TreeCalculator
	{
	private:
		// 计算树指针
	public:
		TreeCalculator()
		{
			// # To be added
		}

		~TreeCalculator()
		{
			// # To be added
		}

		void run(double** input, double* output)
		{
			// # To be added
		}

		Calculator* copy()
		{
			// # To be added
			return nullptr;
		}
	};
}