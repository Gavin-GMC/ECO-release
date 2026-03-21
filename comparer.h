//------------------------Description------------------------
// This file defines the solution comparer in ECFC. They support
// candidate solution comparison considering target priority.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include"basicfunc.h"
#include"sort-helper.h"
#include"objective.h"

namespace ECFC
{
	class Comparer
	{
	protected:
		int _object_level; // 目标优先级的种数
		int* _object_number; // 每一级的目标数目
		int* _priority; // 按优先级排序的目标索引
		bool* _min_is_better; // 目标是最小化/最大化标识
	public:
		Comparer(Objective* objectives, const int objective_number)
		{
			_priority = new int[objective_number];
			_min_is_better = new bool[objective_number];
			sortHelper<int, int>* sort_buffer = new sortHelper<int, int>[objective_number];
			for (int i = 0; i < objective_number; i++)
			{
				sort_buffer[i] = sortHelper<int, int>(i, -1 * objectives[i].priority());
				_min_is_better[i] = objectives[i].IsMin();
			}
			sort(sort_buffer, sort_buffer + objective_number, false);

			_object_level = 0;
			_object_number = new int[objective_number];
			int pre_priority = sort_buffer[0].value;
			_object_number[0] = 0;
			for (int i = 0; i < objective_number; i++)
			{
				_priority[i] = sort_buffer[i].id;
				if (sort_buffer[i].value != pre_priority)
				{
					//优先级小于0的目标不参与比较
					if (sort_buffer[i].value > 0)
					{
						break;
					}
					_object_level++;
					_object_number[_object_level] = 0;
					_priority[i] = sort_buffer[i].id;
					pre_priority = sort_buffer[i].value;
				}
				_object_number[_object_level]++;
			}

			_object_level++;
			delete[] sort_buffer;
		}

		virtual ~Comparer()
		{
			delete[] _object_number;
			delete[] _priority;
			delete[] _min_is_better;
		}

		// 返回两个目标向量的优劣关系(f1是否好于f2)
		virtual bool isBetter(const double f1[], const double f2[])
		{
			int object_id;
			int counter;
			
			// 空值排除
			counter = 0;
			for (int i = 0; i < _object_level; i++)
			{
				for (int j = 0; j < _object_number[i]; j++)
				{
					object_id = _priority[counter++];

					if (f1[object_id] == EMPTYVALUE)
						return false; // f1存在未评估目标，是差的
					if (f2[object_id] == EMPTYVALUE)
						return true; // f2存在未评估目标，是差的
				}
			}

			// 依优先级的比较
			bool is_better;
			bool is_worse;
			counter = 0;
			bool outperformed;
			for (int i = 0; i < _object_level; i++)
			{
				is_better = true;
				is_worse = true;
				for (int j = 0; j < _object_number[i]; j++)
				{
					object_id = _priority[counter++];
					if (f1[object_id] == f2[object_id])
						continue;
					outperformed = f1[object_id] > f2[object_id];
					outperformed = outperformed ^ _min_is_better[object_id];

					if (outperformed)
						is_worse = false;
					else
						is_better = false;
				}

				if (is_better && !is_worse)
					return true;
				if (!is_better && is_worse)
					return false;
			}
			return false;
		}

		// 最高优先级目标中的最大差异
		virtual double diff_max_priori(const double f1[], const double f2[])
		{
			int o_id = _priority[0];
			double result = abs(f1[o_id] - f2[o_id]);

			double buffer;
			for (int i = 1; i < _object_number[0]; i++)
			{
				o_id = _priority[i];
				buffer = abs(f1[o_id] - f2[o_id]);
				if (buffer > result)
					result = buffer;
			}

			return result;
		}
	};

	// 单一主目标下的比较器特化，用以节约比较时间
	class SingleObjectComparer final : public Comparer
	{
	private:
		int _key_id;
		bool _key_is_min;

	public:
		SingleObjectComparer(Objective* objectives, const int objective_number)
			:Comparer(objectives, objective_number)
		{
			_key_id = _priority[0];
			_key_is_min = _min_is_better[_key_id];
		}

		~SingleObjectComparer()
		{

		}

		bool isBetter(const double f1[], const double f2[])
		{
			bool smaller = f1[_key_id] < f2[_key_id];
			return smaller ^ _key_is_min;
		}

		double diff_max_priori(const double f1[], const double f2[])
		{
			return f1[0] - f2[0];
		}
	};
}
