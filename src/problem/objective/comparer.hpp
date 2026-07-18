//------------------------Description------------------------
// This file defines the solution comparer in ECFlow. They support
// candidate solution comparison considering target priority.
//-------------------------Copyright-------------------------
// Copyright (c) 2024 ���������ƣ���ȷ�ϣ�, All Rights Reserved.
// You are free to use the ECFlow����ȷ�ϣ� for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference 
// "δȷ��"
//-----------------------------------------------------------

#pragma once
#include"ecflow-basicfunc.h"
#include"objective.h"

namespace ECFlow
{
	class Comparer
	{
	protected:
		int _object_level; // Ŀ�����ȼ�������
		int* _object_number; // ÿһ����Ŀ����Ŀ
		int* _priority; // �����ȼ������Ŀ������
		bool* _min_is_better; // Ŀ������С��/��󻯱�ʶ
	public:
		Comparer(Objective* objectives, const int objective_number)
		{
			_priority = new int[objective_number];
			_min_is_better = new bool[objective_number];
			for (int i = 0; i < objective_number; i++)
			{
				_priority[i] = 0;
				_min_is_better[i] = objectives[i].IsMin();
			}

			// Only objectives with priority >= 0 participate in comparison.
			// priority < 0 means "excluded" for ANY negative value (not just the
			// default sentinel -1). Exclusion is done by filtering up front, so it
			// no longer relies on sort order / the first-group-always-kept boundary.
			sortHelper<int, int>* sort_buffer = new sortHelper<int, int>[objective_number];
			int participating = 0;
			for (int i = 0; i < objective_number; i++)
				if (objectives[i].priority() >= 0)
					sort_buffer[participating++] = sortHelper<int, int>(i, -1 * objectives[i].priority());

			// Ascending by value(=-priority): the larger priority number decides first.
			sort(sort_buffer, sort_buffer + participating, true);

			_object_number = new int[objective_number];
			for (int i = 0; i < objective_number; i++) _object_number[i] = 0;
			_object_level = 0;
			if (participating > 0)
			{
				int pre_priority = sort_buffer[0].value;
				for (int i = 0; i < participating; i++)
				{
					_priority[i] = sort_buffer[i].id;
					if (sort_buffer[i].value != pre_priority)
					{
						_object_level++;
						pre_priority = sort_buffer[i].value;
					}
					_object_number[_object_level]++;
				}
				_object_level++;
			}
			delete[] sort_buffer;
		}

		virtual ~Comparer()
		{
			delete[] _object_number;
			delete[] _priority;
			delete[] _min_is_better;
		}

		// ��������Ŀ�����������ӹ�ϵ(f1�Ƿ����f2)
		virtual bool isBetter(const double f1[], const double f2[])
		{
			int object_id;
			int counter;

			// ��ֵ�ų�
			counter = 0;
			for (int i = 0; i < _object_level; i++)
			{
				for (int j = 0; j < _object_number[i]; j++)
				{
					object_id = _priority[counter++];

					if (is_empty(f1[object_id]))   // 修复:原 == EMPTYVALUE 对 NaN 恒假 → "未评估目标视为差"从未生效
						return false; // f1����δ����Ŀ�꣬�ǲ��
					if (is_empty(f2[object_id]))   // 同上
						return true; // f2����δ����Ŀ�꣬�ǲ��
				}
			}

			// �����ȼ��ıȽ�
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

		// ������ȼ�Ŀ���е�������
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

	// ��һ��Ŀ���µıȽ����ػ������Խ�Լ�Ƚ�ʱ��
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
