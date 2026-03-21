//------------------------Description------------------------
// This file defines the inspiration in ECFC. They provide 
// the optimizer with recommendations for the best possible 
// decision based on specific rules.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include<algorithm>
#include <random>
#include"calculator.h"
#include"sort-helper.h"
#include"feasible-region.h"

namespace ECFC
{
	// 启发器的设计思路：
	// 基于可行域和启发式产生启发决策
	// 其不负责可行域的维护，所以可行域作为参数传入可以更好地降低耦合度
	// 基于此，其参数传入应为当前可行域，决策量维度，计算所需的量
	// 启发器与量（非维度）进行绑定
	// 
	// 启发式的情况：
	// 动态 - 计算中涉及其它决策量，会根据候选解生成状态而发生改变
	// 静态 - 只与环境中的常量相关
	// 
	// 启发式计算式定义：
	// 第一维：当前决策维度
	// 第二维：决策量长度
	// 后续维：决策量内容，涉及的其他常量
	//


	// 决策启发器，与维度绑定
	class Inspiration
	{
	public:
		Inspiration()
		{

		}

		virtual ~Inspiration() {}

		// 获取可能的最优决策
		virtual double getPrioriDecision(FeasibleLine* feasible_region, int demension, double** inputs) = 0;

		// 获得所有可行决策的优先序
		virtual double* getPrioriOrder(FeasibleLine* feasible_region, int demension, double** inputs, int& order_size) = 0;

		// 获得启发式的计算值
		virtual double getHeuristic(FeasibleLine* feasible_region, int demension, double choice, double** inputs) = 0;

		virtual Inspiration* clone() = 0;
	};

	// 随机启发器，默认返回可行域中的随机值（缺省启发器）
	class RandomInspiration final : public Inspiration
	{
	public:
		RandomInspiration()
			:Inspiration()
		{

		}

		~RandomInspiration() {}

		double getPrioriDecision(FeasibleLine* feasible_region, int demension, double** inputs)
		{
			return feasible_region->getRandomDecision();
		}

		double* getPrioriOrder(FeasibleLine* feasible_region, int demension, double** inputs, int& order_size)
		{
			double* back = feasible_region->getFeasibleList(order_size);
			std::shuffle(back, back + order_size, std::random_device());
			return back;
		}

		double getHeuristic(FeasibleLine* feasible_region, int demension, double choice, double** inputs)
		{
			return rand01();
		}

		Inspiration* clone()
		{
			return new RandomInspiration();
		}
	};

	// 边界启发器，默认返回可行域的边界值
	class BoundaryInspiration final : public Inspiration
	{
	public:
		BoundaryInspiration()
			:Inspiration()
		{

		}

		~BoundaryInspiration() {}

		double getPrioriDecision(FeasibleLine* feasible_region, int demension, double** inputs)
		{
			return feasible_region->getBoundaryDecision(true);
		}

		double* getPrioriOrder(FeasibleLine* feasible_region, int demension, double** inputs, int& order_size)
		{
			double* back = feasible_region->getFeasibleList(order_size);
			return back;
		}

		double getHeuristic(FeasibleLine* feasible_region, int demension, double choice, double** inputs)
		{
			if (choice == feasible_region->getBoundaryDecision(true) || choice == feasible_region->getBoundaryDecision(false))
				return 1;
			return 0;
		}

		Inspiration* clone()
		{
			return new BoundaryInspiration();
		}
	};

	// 基于预定义的启发式信息，返回优先决策，针对启发式信息静态不变的情况，时间复杂度低
	class StableInspiration : public Inspiration
	{
	private:
		double* _saved_order;
		int _order_size[2];

		double& orderTable(int demension, int rank)
		{
			return _saved_order[demension * _order_size[0] + rank];
		}

		void ini(FeasibleLine* region_pointer, Calculator* calculator, double** inputs)
		{
			double decision_pair[2];// 0 is demension, 1 is decision 
			int paras = calculator->getParameterNumber();
			double** calculator_buffer = new double* [paras];
			double* feasible_list;
			int feasible_list_size;
			sortHelper<double, double>* sort_buffer = new sortHelper<double, double>[_order_size[1]];

			// 获取所有可行解
			feasible_list = region_pointer->getFeasibleList(feasible_list_size);
			// 构建输入量
			calculator_buffer[0] = decision_pair;
			calculator_buffer[1] = decision_pair + 1;
			memcpy(calculator_buffer + 2, inputs, (paras - 2) * sizeof(double*));

			for (int demensionId = 0; demensionId < _order_size[0]; demensionId++)
			{
				// 计算可行解启发式值
				decision_pair[0] = demensionId;
				for (int i = 0; i < feasible_list_size; i++)
				{
					sort_buffer[i].id = feasible_list[i];
					decision_pair[1] = feasible_list[i];
					calculator->run(calculator_buffer, &sort_buffer[i].value);
				}

				// 可行解按启发式值排序
				std::sort(sort_buffer, sort_buffer + feasible_list_size);

				// 存入记录容器
				for (int i = 0; i < feasible_list_size; i++)
				{
					orderTable(demensionId, i) = sort_buffer[i].value;
				}

				// 清理内存
				delete[] feasible_list;
			}
			delete[] sort_buffer;
		}

		StableInspiration()
		{

		}

	public:
		StableInspiration(FeasibleLine* region_pointer, Calculator* calculator, int demension_number, double** inputs)
			:Inspiration()
		{
			_order_size[0] = demension_number;
			_order_size[1] = region_pointer->getLength();
			_saved_order = new double[_order_size[0] * _order_size[1]];
			ini(region_pointer, calculator, inputs);
		}

		StableInspiration(double* priori_matrix, int size, int demension_number)
			:Inspiration()
		{
			_order_size[0] = demension_number;
			_order_size[1] = size;
			_saved_order = new double[_order_size[0] * _order_size[1]];
			memcpy(_saved_order, priori_matrix, size * demension_number * sizeof(double));
		}

		~StableInspiration()
		{
			delete[] _saved_order;
		}

		double* getPrioriOrder(FeasibleLine* feasible_region, int demension, double** inputs, int& order_size)
		{
			order_size = feasible_region->getLength();
			double* back = new double[order_size];

			//将可行解按存储的顺序排列
			int counter = 0;
			for (int i = 0; i < _order_size[1]; i++)
			{
				if (feasible_region->isin(orderTable(demension, i)))
				{
					back[counter] = orderTable(demension, i);
					counter++;
				}
			}

			return back;
		}

		// 预设可行解不为空，应当在调用之前进行检查
		double getPrioriDecision(FeasibleLine* feasible_region, int demension, double** inputs)
		{
			for (int i = 0; i < _order_size[1]; i++)
			{
				if (feasible_region->isin(orderTable(demension, i)))
				{
					return orderTable(demension, i);
				}
			}

			return EMPTYVALUE;
		}

		double getHeuristic(FeasibleLine* feasible_region, int demension, double choice, double** inputs)
		{
			int index = demension * _order_size[0];
			for (int i = 0; i < _order_size[1]; i++)
			{
				if (_saved_order[index] == choice)
					return i;
				index++;
			}
			return _order_size[1] + 1;
		}

		Inspiration* clone()
		{
			StableInspiration* back = new StableInspiration();

			memcpy(back->_saved_order, _saved_order, sizeof(double) * _order_size[0] * _order_size[1]);
			memcpy(back->_order_size, _order_size, 2 * sizeof(double));

			return back;
		}
	};

	// 基于预定义的启发式信息，返回优先决策，针对启发式信息动态生成的情况，时间复杂度较高
	class NormalInspiration : public Inspiration
	{
	private:
		Calculator* _calculator;
		double _decision_pair[2];
		int _parameter_number;
		double** _calculate_buffer; // 前两维分别为决策维度和决策值

	public:
		NormalInspiration(Calculator* calculator)
			:Inspiration()
		{
			_calculator = calculator->copy();
			_parameter_number = calculator->getParameterNumber();
			if (_parameter_number < 2) // 报错
				return;
			_calculate_buffer = new double* [_parameter_number];
			_calculate_buffer[0] = _decision_pair;
			_calculate_buffer[1] = _decision_pair + 1;
		}

		~NormalInspiration()
		{
			delete _calculator;
			delete[] _calculate_buffer;
		}

		double* getPrioriOrder(FeasibleLine* feasible_region, int demension, double** inputs, int& order_size)
		{
			double* back = feasible_region->getFeasibleList(order_size);
			sortHelper<double, double>* sort_buffer = new sortHelper<double, double>[order_size];
			memcpy(_calculate_buffer + 2, inputs, sizeof(double*) * (_parameter_number - 2));

			//计算可行解启发式值
			_decision_pair[0] = demension;
			for (int i = 0; i < order_size; i++)
			{
				sort_buffer[i].id = back[i];
				_decision_pair[1] = back[i];
				_calculator->run(_calculate_buffer, &sort_buffer[i].value);
			}

			//可行解按启发式值排序
			std::sort(sort_buffer, sort_buffer + order_size, std::greater<sortHelper<double, double>>());

			//存入返回容器
			for (int i = 0; i < order_size; i++)
			{
				back[i] = sort_buffer[i].id;
			}

			delete[] sort_buffer;
			return back;
		}

		double getPrioriDecision(FeasibleLine* feasible_region, int demension, double** inputs)
		{
			int order_size;
			double* priori_order = getPrioriOrder(feasible_region, demension, inputs, order_size);

			double back = priori_order[0];
			delete[] priori_order;

			return back;
		}

		double getHeuristic(FeasibleLine* feasible_region, int demension, double choice, double** inputs)
		{
			double back;
			_decision_pair[0] = demension;
			_decision_pair[1] = choice;
			memcpy(_calculate_buffer + 2, inputs, sizeof(double*) * (_parameter_number - 2));
			_calculator->run(_calculate_buffer, &back);

			return back;
		}

		Inspiration* clone()
		{
			return new NormalInspiration(_calculator);
		}

		StableInspiration* toStable(FeasibleLine* region_pointer, int demension_number, double** inputs)
		{
			return new StableInspiration(region_pointer, _calculator, demension_number, inputs);
		}
	};
}