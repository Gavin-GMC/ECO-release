//------------------------Description------------------------
// This file defines the population consisted by individuals.
// It provides an object that manages the set of candidate
// solutions.
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
#include"individual-factory.h"
#include<algorithm>

namespace ECFC
{
	class Population
	{
	private:
		int _individual_buffer_size;
		int _population_size;
		Individual** _individuals;
		IndividualType _individual_type;

	public:
		Population(IndividualType type = IndividualType::F_default, int size = 1)
		{
			_individual_buffer_size = size;
			_population_size = size;
			_individual_type = type;

			_individuals = IndividualFactory::newIndividualArray(_individual_type, size);
		}

		~Population()
		{
			for (int i = 0; i < _individual_buffer_size; i++)
				delete _individuals[i];
			
			delete[] _individuals;
		}

		void ini()
		{
			for (int i = 0; i < _individual_buffer_size; i++)
			{
				_individuals[i]->ini(true, true, true);
			}
		}

		int getSize()
		{
			return _population_size;
		}

		Individual** getIndividuals()
		{
			return _individuals;
		}

		// 可能有堆损坏的异常！！！！！！！！
		void extend(int target_size)
		{
			if (target_size <= _individual_buffer_size)
			{
				_population_size = target_size;
			}
			else
			{
				int array_size = _population_size;//带有余量，避免频繁的拓展操作

				//测试新种群的长度
				while (array_size < target_size)
				{
					array_size += _population_size;
				}

				// 申请新的空间
				Individual** individuals_buffer = _individuals;
				_individuals = IndividualFactory::newIndividualArray(_individual_type, array_size);

				//将原种群复制入新种群
				for (int i = 0; i < _individual_buffer_size; i++)
				{
					// 通过交换避免深拷贝和析构错误
					std::swap(_individuals[i], individuals_buffer[i]);
				}

				for (int i = 0; i < _individual_buffer_size; i++)
				{
					delete individuals_buffer[i];
				}
				delete[] individuals_buffer;

				//设置问题给新种群剩余个体
				for (int i = _individual_buffer_size; i < array_size; i++)
				{
					_individuals[i]->setProblem(**_individuals);
				}

				//调整size数据与实际一致
				_individual_buffer_size = array_size;
				_population_size = target_size;
			}
		}

		void append(Individual* individuals, int size = 1)
		{
			//如果内存长度不够就进行拓展
			if (_population_size + size > _individual_buffer_size)
			{
				extend(_population_size + size);
				_population_size -= size;
			}
			//将新个体逐个拷贝（全部采用深拷贝，因为不知插入个体未来的状态，后续可以考虑新增参数来控制这一点）
			for (int i = 0; i < size; i++)
			{
				_individuals[_population_size++]->copy(individuals[i]);
			}
		}

		void remove(Individual* remove_individuals, int size = 1)
		{
			for (int i = 0; i < size; i++)
			{
				for (int j = 0; j < _population_size; j++)
				{
					//发现要移除的个体
					if (remove_individuals[i] == *(_individuals[j]))
					{
						//将个体调至队尾并缩减数组长度记录
						std::swap(_individuals[j], _individuals[--_population_size]);
					}
				}
			}
		}

		void remove(int* remove_individuals_index, int size = 1)
		{
			//对索引进行排序
			std::sort(remove_individuals_index, remove_individuals_index + size, std::greater<int>());
			//从大至小，将个体与队尾交换
			int last_index = -1;
			for (int i = 0; i < size; i++)
			{
				//异常情况的处理，排除非法索引以及重复的删除
				if (remove_individuals_index[i] < 0)
					break;
				if (remove_individuals_index[i] >= _population_size || remove_individuals_index[i] == last_index)
					continue;

				//将个体调至队尾并缩减数组长度记录
				_population_size--;
				std::swap(_individuals[remove_individuals_index[i]], _individuals[_population_size]);
				//记录更改
				last_index = remove_individuals_index[i];
			}
		}

		void sort()
		{
            int n = _population_size;
            bool swapped;
            for (int i = 0; i < n - 1; ++i) {
                swapped = false;
                for (int j = 0; j < n - 1 - i; ++j) {
                    // 如果当前元素大于下一个元素，则交换它们
                    if (*_individuals[j + 1] < *_individuals[j]) {
                        _individuals[j]->swap(*_individuals[j + 1]);
                        swapped = true;
                    }
                }
                // 如果在这一轮中没有发生交换，则说明数组已经有序，可以提前结束排序
                if (!swapped) {
                    break;
                }
            }
		}

		void setProblem(ProblemHandle* problem_handle)
		{
			for (int i = 0; i < _individual_buffer_size; i++)
			{
				_individuals[i]->setProblem(problem_handle);
			}
		}

		Individual& operator[](int i)
		{
			return *(_individuals[i]);
		}

	};
}