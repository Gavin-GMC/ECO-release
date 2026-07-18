//------------------------Description------------------------
// This file defines the calculation process object in ECFlow,
// which is used to encapsulate and provide a uniform interface
// for different forms of computing processes
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference 
// "未确定"
//-----------------------------------------------------------
#pragma once

#include<string>

namespace ECFlow
{
	// 前向声明
	class eccalcul_functor;
	class ExpressionTree;
	struct ElementNote;

	/**
	 * @brief 计算器抽象基类
	 * @details 定义计算模块的统一接口规范，支持三种计算形式（计算树、函数指针、仿函数），
	 *          包含参数数量、结果长度的基础属性，声明纯虚的计算执行、对象拷贝接口，
	 *          所有具体计算器子类需继承并实现该接口
	 * @note 该类为抽象类，无法直接实例化，需通过子类（FuncCalculator/FunctorCalculator/TreeCalculator）实现具体逻辑
	 */
	class Calculator
	{
	protected:
		int _parameter_number;  /**< 输入参数的数量：表示计算所需的输入参数个数 */
		int _result_size;       /**< 输出结果的长度：表示计算结果的元素个数（标量为1，数组为对应长度） */

	public:
		/**
		 * @brief 构造函数
		 * @details 初始化计算器的输入参数数量和输出结果长度
		 * @param parameter_number 输入参数的数量
		 * @param result_size 输出结果的长度
		 */
		Calculator(int parameter_number, int result_size)
		{
			_parameter_number = parameter_number;
			_result_size = result_size;
		}

		/**
		 * @brief 虚析构函数
		 * @details 声明为虚析构函数，确保子类析构时能正确调用自身的析构逻辑，避免内存泄漏
		 */
		virtual ~Calculator()
		{

		}

		/**
		 * @brief 计算执行纯虚接口
		 * @details 定义计算的核心接口，子类需实现具体的计算逻辑：基于输入参数完成计算，并将结果写入输出数组
		 * @param input 输入参数二维指针：input[i]表示第i个参数的起始地址（支持标量/数组参数）
		 * @param output 输出结果指针：指向预分配的内存区域，用于存储计算结果
		 * @note 纯虚函数，必须由子类实现；调用前需确保input和output指向有效内存，且output长度不小于_result_size
		 */
		virtual void run(double** input, double* output) = 0;

		/**
		 * @brief 获取输入参数数量
		 * @details 读取计算器初始化时设置的输入参数数量
		 * @return int - 输入参数的数量
		 */
		int getParameterNumber()
		{
			return _parameter_number;
		}

		/**
		 * @brief 获取输出结果长度
		 * @details 读取计算器初始化时设置的输出结果长度
		 * @return int - 输出结果的元素个数
		 */
		int getResultSize()
		{
			return _result_size;
		}

		/**
		 * @brief 对象拷贝纯虚接口
		 * @details 定义计算器对象的深拷贝接口，用于创建与当前对象状态一致的新实例
		 * @return Calculator* - 指向新创建的计算器子类实例的指针（需由调用方负责释放内存）
		 * @note 纯虚函数，子类需实现深拷贝逻辑，避免浅拷贝导致的资源访问异常
		 */
		virtual Calculator* copy() = 0;
	};

	/**
	 * @brief 基于函数指针的计算器子类
	 * @details 继承Calculator抽象基类，实现基于C风格函数指针的计算逻辑，适用于简单的标量计算场景
	 * @note 该类仅支持输出标量结果（_result_size固定为1），函数指针需符合`double (*)(double**)`签名
	 */
	class FuncCalculator : public Calculator
	{
	private:
		double (*_function)(double** input);

	public:
		FuncCalculator(double (*function)(double** input), int parameter_number, int result_size);

		~FuncCalculator();

		void run(double** input, double* output);

		Calculator* copy();
	};

	/**
	 * @brief 基于仿函数的计算器子类
	 * @details 继承Calculator抽象基类，实现基于自定义仿函数的计算逻辑，支持更灵活的状态化计算（仿函数可包含成员变量）
	 * @note 仿函数需实现`operator()(double** input)`重载，返回标量结果；析构时会自动释放仿函数对象
	 */
	class FunctorCalculator : public Calculator
	{
	private:
		eccalcul_functor* _function;

	public:
		FunctorCalculator(eccalcul_functor* function, int parameter_number, int result_size);

		~FunctorCalculator();

		void run(double** input, double* output);

		Calculator* copy();
	};

	/**
	 * @brief 基于计算树的计算器类
	 * @details 继承Calculator抽象基类，实现基于计算树（CalculationTree）的计算逻辑，支持复杂表达式（标量/数组/矩阵）的计算
	 */
	class TreeCalculator : public Calculator
	{
	private:
		ExpressionTree* _tree;

	public:
		TreeCalculator(ExpressionTree* tree, int parameter_number, int result_size);

		TreeCalculator(std::string expression, ElementNote* notes, int parameter_number, int result_size);

		~TreeCalculator();

		void run(double** input, double* output);

		Calculator* copy();
	};
}