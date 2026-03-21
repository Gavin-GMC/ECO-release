//------------------------Description------------------------
// This file defines some optional environment selection components,
// which are basic and commonly used.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include"environment-select.h"
#include"multiobjective.h"
#include"sort-helper.h"

namespace ECFC
{
	// 基于pareto支配关系和个体拥挤度构建的偏序关系，从亲代和子代中选取最优的部分进入下一代
	class RankCrowdingUpdater final : public EnvirSelect
	{
	private:
		int _buffer_size;
		Population _buffer;
		double* _indicators;
		sortHelper<int, double>* _sort_buffer;
		
	public:
		RankCrowdingUpdater()
			:EnvirSelect(true)
		{
			_buffer_size = 0;
		}

		~RankCrowdingUpdater()
		{
			delete[] _indicators;
		}

		static void preAssert(AssertList& list)
		{

		}

		static void postAssert(AssertList& list)
		{

		}

		void update_subswarm(Population& parent, Population& offspring)
		{
			// 准备
			int parent_size = parent.getSize();
			int offspring_size = offspring.getSize();
			int total_size = parent_size + offspring_size;
			if (total_size != _buffer_size)
			{
				_buffer_size = total_size;
				_buffer.extend(total_size);
				delete[] _indicators;
				_indicators = new double[_buffer_size + 1];
				delete[] _sort_buffer;
				_sort_buffer = new sortHelper<int, double>[_buffer_size + 1];
			}
			
			// 获得当前完整种群的浅拷贝
			for (int i = 0; i < parent_size; i++)
			{
				_buffer[i].shallowCopy(parent[i]);
			}
			for (int i = 0; i < offspring_size; i++)
			{
				_buffer[i + parent_size].shallowCopy(parent[i]);
			}

			// 快速非支配排序
			MO::fastNonDominatedSort(_buffer, total_size, _indicators);
			for (int i = 0; i < total_size; i++)
			{
				_sort_buffer[i].id = i;
				_sort_buffer[i].value = _indicators[i];
			}
			std::sort(_sort_buffer, _sort_buffer + total_size);

			// 清除连接，避免buffer析构时对链接目标空间的错误析构
			// for (int i = 0; i < parent.getSize(); i++)
			// {
			// 	_buffer[i].shallowClear();
			// }

			// 对跨越区域进行拥挤度排序
			int pf = _sort_buffer[parent_size - 1].value;
			// 获得跨越区域的起始个体
			int begin = parent_size;
			while (_sort_buffer[begin].value == pf && begin > -1)
			{
				begin--;
			}
			begin++;			
			// 获得跨越区域的终止个体
			int end = parent_size;
			while (_sort_buffer[end].value == pf && end < total_size)
			{
				end++;
			}
			end--;

			if (end != parent_size - 1) // 存在跨越区域
			{
				int region_size = end - begin + 1;
				int indi_id;
				for (int i = 0; i < region_size; i++)
				{
					indi_id = _sort_buffer[begin + i].id;
					if (indi_id > parent_size)
					{
						_buffer[i].shallowCopy(offspring[indi_id - parent_size]);
					}
					else
					{
						_buffer[i].shallowCopy(parent[indi_id]);
					}
				}

				// 拥挤度计算
				MO::normalizeCrowdDistance(_buffer, region_size, _indicators);
				for (int i = 0; i < region_size; i++)
				{
					_sort_buffer[begin + i].value = _indicators[i];
				}
				std::sort(_sort_buffer + begin, _sort_buffer + end + 1);
			}

			// 构建新种群
			int better_id;
			int worse_id;
			int worse_index = parent_size - 1;
			for (int i = 0; i < parent_size; i++)
			{
				better_id = _sort_buffer[i].id;
				if (better_id < parent_size)
				{
					continue;
				}
				else
				{
					while (true)
					{
						worse_index++;
						if (_sort_buffer[worse_index].id < parent_size)
						{
							worse_id = _sort_buffer[worse_index].id;
							break;
						}
					}
					parent[worse_id].swap(offspring[better_id - parent_size]);
				}
			}
		}
	};
}
