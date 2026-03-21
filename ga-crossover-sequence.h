//------------------------Description------------------------
// This file defines the crossover operator of evolationary algorithms.
// It realizes the generation of new candidate solutions by exchanging
//  fragments of different individuals in a specific way.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include"ga-crossover.h"
#include<unordered_map>

namespace ECFC
{
	// 对于面向序列优化的算子，拓展兼顾到两种场景
	// 1. 同一解中存在相同的变量
	// 2. 两个解中存在对方不具有的变量
	// 因此变量的匹配也具备三种情况
	// 1. 1对1匹配
	// 2. 1对多匹配
	// 3. 1对0匹配
	// 综合这些情况，基于两个候选解长度一致，做出如下的判断与操作
	// 1对1匹配时符合序列算子的假设，正常操作
	// 1对多时，选择任一未进行匹配过的对象，对于多次出现的同一数值应分别对待
	// 1对0时，或1对多但所有匹配均已用尽，此时优先从未匹配对象中选择，其后从多匹配但未用尽对象中选择
	// 
	// 一个关键点在于匹配链的构造是积极的还是消极的，是需要构建一个完整表还是每次逐个检索的
	// 为了兼顾各种情形，保证可用性与鲁棒性，采用积极的形式
	// 
	// 由于当出现不满足序列约束（独一约束，且元素相同）情形时，面向序列的交叉算子本身就工作不佳
	// 因此不再过度追求其适用性以及行为的相似性，以如下顺序进行匹配
	// 1. 寻找学习对象中元素匹配，且未进行交换的基因位
	// 2. 对位元素（即回归传统的点交叉/均匀交叉）
	// 3. 随机未匹配元素
	
	// 构建基因位匹配关系（将s1中索引在map_list中的基因位与s2进行匹配，并将结果存储在match_list中进行返回）
	void buildMap(double* s1, double* s2, int solution_size, int* map_list, int* match_list, int list_size)
	{
		bool* used = new bool[solution_size];
		for (int i = 0; i < solution_size; i++)
			used[i] = false;

		// 寻找匹配
		bool found;
		int s1_index;
		for (int i = 0; i < list_size; i++)
		{
			// 查找对应位置
			s1_index = map_list[i];
			found = false;
			int d;
			for (d = 0; d < solution_size; d++)
			{
				if (used[d]) // 当前位置已被匹配
					continue;

				if (equal(s1[s1_index], s2[d]))
				{
					used[d] = true;
					match_list[i] = d;
					found = true;
					break;
				}
			}

			// 若无则优先对位
			d = s1_index;
			while (!found)
			{
				if (!used[d])
				{
					used[d] = true;
					match_list[i] = d;
					found = true;
					break;
				}
				d = (d + rand() % solution_size) % solution_size;
			}
		}

		delete[] used;
	}

	void buildMap(double* s1, double* s2, int solution_size, int begin_index, int end_index, int* match_list)
	{
		bool* used = new bool[solution_size];
		for (int i = 0; i < solution_size; i++)
			used[i] = false;

		int list_size = end_index - begin_index;

		// 寻找匹配
		bool found;
		int s1_index;
		for (int i = 0; i < list_size; i++)
		{
			// 查找对应位置
			s1_index = begin_index + i;
			found = false;
			int d;
			for (d = 0; d < solution_size; d++)
			{
				if (used[d]) // 当前位置已被匹配
					continue;

				if (equal(s1[s1_index], s2[d]))
				{
					used[d] = true;
					match_list[i] = d;
					found = true;
					break;
				}
			}

			// 若无则优先对位
			d = s1_index;
			while (!found)
			{
				if (!used[d])
				{
					used[d] = true;
					match_list[i] = d;
					found = true;
					break;
				}
				d = (d + rand() % solution_size) % solution_size;
			}
		}

		delete[] used;
	}
	
	// 部分匹配交叉
	class PartialMappedCrossover : public Crossover
	{
	private:
		int _buffer_size;
		int begin_index;
		int end_index;
		double* offspring1;
		double* offspring2;

		void buildPartialMap(double* s1, double* s2, int part_size, std::vector<std::pair<double, double>>& partial_map)
		{
			partial_map.clear();

			// 边存入
			for (int i = 0; i < part_size; i++)
			{
				partial_map.push_back({ s1[i],s2[i] });
			}

			// 合并
			for (int i = 0; i < partial_map.size(); i++)
			{
				for (int j = i + 1; j < partial_map.size(); j++)
				{
					if (equal(partial_map[i].second, partial_map[j].first)) // 向后合并
					{
						partial_map[i].second = partial_map[j].second;
						partial_map.erase(partial_map.begin() + j);
						
						j = i + 1; // 重新检查
					}
					else if (equal(partial_map[i].first, partial_map[j].second)) // 向前合并
					{
						partial_map[i].first = partial_map[j].first;
						partial_map.erase(partial_map.begin() + j);

						j = i + 1; // 重新检查
					}
				}
			}
		}

	public:
		PartialMappedCrossover(double cross_rate, bool coupled = true)
			:Crossover(cross_rate, coupled)
		{
			_buffer_size = 0;
			offspring1 = nullptr;
			offspring2 = nullptr;

			begin_index = -1;
			end_index = -1;
		}

		~PartialMappedCrossover()
		{
			delete[] offspring1;
			delete[] offspring2;
		}

		static void preAssert(AssertList& list, double* paras)
		{
			Crossover::preAssert(list, paras);
			list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::notLessButNotice)); // 需要1个学习目标
			if (paras[1])
				list.add(new Assert(ModuleType::T_learntopology, "coupled", 1, MatchType::anyButNotice)); // 两个个体间相互学习
		}

		static void postAssert(AssertList& list, double* paras)
		{
			Crossover::postAssert(list, paras);
		}

		void preparation(Solution* s)
		{
			Crossover::preparation(s);

			if (new_pair && is_crossover)// 新的亲本对以及需要交叉
			{
				if (_buffer_size != s->getSolutionSize()) // 更新缓存大小
				{
					delete[] offspring1;
					delete[] offspring2;
					_buffer_size = s->getSolutionSize();
					offspring1 = new double[_buffer_size];
					offspring2 = new double[_buffer_size];
				}

				begin_index = rand() % s->getSolutionSize();
				end_index = rand() % s->getSolutionSize();
				if (end_index < begin_index)
					std::swap(begin_index, end_index);
				end_index++;
			}
		}

		void apply(Solution* child, Solution* s1, Solution** s2, ProblemHandle* handle)
		{
			preparation(child);

			if (is_crossover)
			{
				if (new_pair)
				{
					int solution_size = child->getSolutionSize();

					memcpy(offspring1, s1->result, child->getSolutionSize() * sizeof(double));
					memcpy(offspring2, s2[0]->result, child->getSolutionSize() * sizeof(double));

					// 交换基因段
					for (int i = begin_index; i < end_index; i++)
					{
						std::swap(offspring1[i], offspring2[i]);
					}

					// 获取基因段映射环
					int part_size = end_index - begin_index;
					std::vector<std::pair<double, double>> partial_map;
					buildPartialMap(offspring1 + begin_index, offspring2 + begin_index, part_size, partial_map);
					
					// 执行映射
					bool used;
					// 对o1映射
					for (int m = 0; m < partial_map.size(); m++)
					{
						used = false; // 每个映射每个方向只使用一次
						for (int i = 0; i < begin_index; i++)
						{
							if (equal(offspring1[i], partial_map[m].first))
							{
								offspring1[i] = partial_map[m].second;
								used = true;
								break;
							}
						}

						if (used)
							continue;

						for (int i = end_index; i < solution_size; i++)
						{
							if (equal(offspring1[i], partial_map[m].first))
							{
								offspring1[i] = partial_map[m].second;
								break;
							}
						}
					}
					// 对o2映射
					for (int m = 0; m < partial_map.size(); m++)
					{
						used = false; // 每个映射每个方向只使用一次
						for (int i = 0; i < begin_index; i++)
						{
							if (equal(offspring2[i], partial_map[m].second))
							{
								offspring2[i] = partial_map[m].first;
								used = true;
								break;
							}
						}

						if (used)
							continue;

						for (int i = end_index; i < solution_size; i++)
						{
							if (equal(offspring2[i], partial_map[m].second))
							{
								offspring2[i] = partial_map[m].first;
								break;
							}
						}
					}

					// 拷贝子代
					memcpy(child->result, offspring1, child->getSolutionSize() * sizeof(double));
				}
				else
				{
					memcpy(child->result, offspring2, child->getSolutionSize() * sizeof(double));
				}
			}
			else
			{
				memcpy(child->result, s1->result, child->getSolutionSize() * sizeof(double));
			}

			ending();
		}
	};

	// 循环交叉
	class CycleCrossover : public Crossover
	{
	private:
		int _buffer_size;
		bool* used;
		int begin_index;
		double* offspring1;
		double* offspring2;

	public:
		CycleCrossover(double cross_rate, bool coupled = true)
			:Crossover(cross_rate, coupled)
		{
			_buffer_size = 0;
			used = nullptr;
			offspring1 = nullptr;
			offspring2 = nullptr;

			begin_index = -1;
		}

		~CycleCrossover()
		{
			delete[] used;
			delete[] offspring1;
			delete[] offspring2;
		}

		static void preAssert(AssertList& list, double* paras)
		{
			Crossover::preAssert(list, paras);
			list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::notLessButNotice)); // 需要1个学习目标
			if (paras[1])
				list.add(new Assert(ModuleType::T_learntopology, "coupled", 1, MatchType::anyButNotice)); // 两个个体间相互学习
		}

		static void postAssert(AssertList& list, double* paras)
		{
			Crossover::postAssert(list, paras);
		}

		void preparation(Solution* s)
		{
			Crossover::preparation(s);

			if (new_pair && is_crossover)// 新的亲本对以及需要交叉
			{
				if (_buffer_size != s->getSolutionSize()) // 更新缓存大小
				{
					delete[] used;
					delete[] offspring1;
					delete[] offspring2;
					_buffer_size = s->getSolutionSize();
					used = new bool[_buffer_size];
					offspring1 = new double[_buffer_size];
					offspring2 = new double[_buffer_size];
				}

				begin_index = rand() % s->getSolutionSize();
			}
		}

		void apply(Solution* child, Solution* s1, Solution** s2, ProblemHandle* handle)
		{
			preparation(child);

			if (is_crossover)
			{
				if (new_pair)
				{
					memcpy(offspring1, s1->result, child->getSolutionSize() * sizeof(double));
					memcpy(offspring2, s2[0]->result, child->getSolutionSize() * sizeof(double));
					for (int i = 0; i < s1->getSolutionSize(); i++)
						used[i] = false;

					int current_index = begin_index;
					bool found;
					double corr;
					while (true)
					{
						used[current_index] = true;
						std::swap(offspring1[current_index], offspring2[current_index]);

						// 寻找对位
						found = false;
						corr = s2[0]->result[current_index];
						for (int i = 0; i < s1->getSolutionSize(); i++)
						{
							if (used[i]) // 构成（子）环
								continue;

							if (equal(corr, s1->result[i]))
							{
								found = true;
								current_index = i;
								break;
							}
						}

						if (!found) // 达到端点
							break;
					}

					memcpy(child->result, offspring1, child->getSolutionSize() * sizeof(double));
				}
				else
				{
					memcpy(child->result, offspring2, child->getSolutionSize() * sizeof(double));
				}

			}
			else
			{
				memcpy(child->result, s1->result, child->getSolutionSize() * sizeof(double));
			}

			ending();
		}
	};

	// 顺序交叉
	class OrderCrossover : public Crossover
	{
	private:
		int _buffer_size;
		int* matched;
		int begin_index;
		int end_index;
		double* offspring1;
		double* offspring2;

	public:
		OrderCrossover(double cross_rate, bool coupled = true)
			:Crossover(cross_rate, coupled)
		{
			_buffer_size = 0;
			matched = nullptr;
			offspring1 = nullptr;
			offspring2 = nullptr;

			begin_index = -1;
			end_index = -1;
		}

		~OrderCrossover()
		{
			delete[] matched;
			delete[] offspring1;
			delete[] offspring2;
		}

		static void preAssert(AssertList& list, double* paras)
		{
			Crossover::preAssert(list, paras);
			list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::notLessButNotice)); // 需要1个学习目标
			if (paras[1])
				list.add(new Assert(ModuleType::T_learntopology, "coupled", 1, MatchType::anyButNotice)); // 两个个体间相互学习
		}

		static void postAssert(AssertList& list, double* paras)
		{
			Crossover::postAssert(list, paras);
		}

		void preparation(Solution* s)
		{
			Crossover::preparation(s);

			if (new_pair && is_crossover)// 新的亲本对以及需要交叉
			{
				if (_buffer_size != s->getSolutionSize()) // 更新缓存大小
				{
					delete[] matched;
					delete[] offspring1;
					delete[] offspring2;
					_buffer_size = s->getSolutionSize();
					matched = new int[_buffer_size];
					offspring1 = new double[_buffer_size];
					offspring2 = new double[_buffer_size];
				}

				int begin_index = rand() % s->getSolutionSize();
				int end_index = rand() % s->getSolutionSize();
				if (end_index < begin_index)
					std::swap(begin_index, end_index);
				end_index++;
			}
		}

		void apply(Solution* child, Solution* s1, Solution** s2, ProblemHandle* handle)
		{
			preparation(child);

			if (is_crossover)
			{
				if (new_pair)
				{
					int solution_size = child->getSolutionSize();

					memcpy(offspring1, s1->result, child->getSolutionSize() * sizeof(double));
					memcpy(offspring2, s2[0]->result, child->getSolutionSize() * sizeof(double));
					
					buildMap(s1->result, s2[0]->result, solution_size, begin_index, end_index, matched);

					int list_size = end_index - begin_index;
					std::sort(matched, matched + list_size);

					// 交叉
					int o1_index = 0;
					int o2_index = 0;
					int match_index = 0;
					while (true)
					{
						if (o1_index == begin_index)
							o1_index = end_index;
						if (o1_index >= solution_size) // 结束判断放在这里防止end_index是解的结尾
							break;

						while (match_index < list_size && o2_index == matched[match_index])
						{
							o2_index++;
							match_index++;
						}

						std::swap(offspring1[o1_index], offspring2[o2_index]);
						o1_index++;
						o2_index++;
					}

					memcpy(child->result, offspring1, child->getSolutionSize() * sizeof(double));
				}
				else
				{
					memcpy(child->result, offspring2, child->getSolutionSize() * sizeof(double));
				}
			}
			else
			{
				memcpy(child->result, s1->result, child->getSolutionSize() * sizeof(double));
			}

			ending();
		}
	};

	// 子路径交叉交叉
	class SubtourExchangeCrossover : public Crossover
	{
	private:
		int _buffer_size;
		int* matched;
		int begin_index;
		int end_index;
		double* offspring1;
		double* offspring2;

	public:
		SubtourExchangeCrossover(double cross_rate, bool coupled = true)
			:Crossover(cross_rate, coupled)
		{
			_buffer_size = 0;
			matched = nullptr;
			offspring1 = nullptr;
			offspring2 = nullptr;

			begin_index = -1;
			end_index = -1;
		}

		~SubtourExchangeCrossover()
		{
			delete[] matched;
			delete[] offspring1;
			delete[] offspring2;
		}

		static void preAssert(AssertList& list, double* paras)
		{
			Crossover::preAssert(list, paras);
			list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::notLessButNotice)); // 需要1个学习目标
			if (paras[1])
				list.add(new Assert(ModuleType::T_learntopology, "coupled", 1, MatchType::anyButNotice)); // 两个个体间相互学习
		}

		static void postAssert(AssertList& list, double* paras)
		{
			Crossover::postAssert(list, paras);
		}

		void preparation(Solution* s)
		{
			Crossover::preparation(s);

			if (new_pair && is_crossover)// 新的亲本对以及需要交叉
			{
				if (_buffer_size != s->getSolutionSize()) // 更新缓存大小
				{
					delete[] offspring1;
					delete[] offspring2;
					delete[] matched;
					_buffer_size = s->getSolutionSize();
					matched = new int[_buffer_size];
					offspring1 = new double[_buffer_size];
					offspring2 = new double[_buffer_size];
				}

				begin_index = rand() % s->getSolutionSize();
				end_index = rand() % s->getSolutionSize();
				if (end_index < begin_index)
					std::swap(begin_index, end_index);
				end_index++;
			}
		}

		void apply(Solution* child, Solution* s1, Solution** s2, ProblemHandle* handle)
		{
			preparation(child);

			if (is_crossover)
			{
				if (new_pair)
				{
					int solution_size = child->getSolutionSize();

					memcpy(offspring1, s1->result, child->getSolutionSize() * sizeof(double));
					memcpy(offspring2, s2[0]->result, child->getSolutionSize() * sizeof(double));
					
					buildMap(s1->result, s2[0]->result, solution_size, begin_index, end_index, matched);

					int list_size = end_index - begin_index;
					std::sort(matched, matched + list_size);

					for (int i = 0; i < list_size; i++)
					{
						std::swap(offspring1[i + begin_index], offspring2[matched[i]]);
					}

					memcpy(child->result, offspring1, child->getSolutionSize() * sizeof(double));
				}
				else
				{
					memcpy(child->result, offspring2, child->getSolutionSize() * sizeof(double));
				}
			}
			else
			{
				memcpy(child->result, s1->result, child->getSolutionSize() * sizeof(double));
			}

			ending();
		}
	};

	// 基于位置的交叉
	class PositionBasedCrossover : public Crossover
	{
	private:
		int _buffer_size;

		int* genes;
		int* matched;
		int list_size;

		double* offspring1;
		double* offspring2;
		double _proportion;

	public:
		PositionBasedCrossover(double cross_rate, double select_gene_proportion, bool coupled = true)
			:Crossover(cross_rate, coupled)
		{
			_buffer_size = 0;
			_proportion = select_gene_proportion;

			genes = nullptr;
			matched = nullptr;
			list_size = 0;

			offspring1 = nullptr;
			offspring2 = nullptr;
		}

		~PositionBasedCrossover()
		{
			delete[] genes;
			delete[] matched;
			delete[] offspring1;
			delete[] offspring2;
		}

		static void preAssert(AssertList& list, double* paras)
		{
			Crossover::preAssert(list, paras);
			list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::notLessButNotice)); // 需要1个学习目标
			if (paras[1])
				list.add(new Assert(ModuleType::T_learntopology, "coupled", 1, MatchType::anyButNotice)); // 两个个体间相互学习
		}

		static void postAssert(AssertList& list, double* paras)
		{
			Crossover::postAssert(list, paras);
		}

		void preparation(Solution* s)
		{
			Crossover::preparation(s);

			if (new_pair && is_crossover)// 新的亲本对以及需要交叉
			{
				if (_buffer_size != s->getSolutionSize()) // 更新缓存大小
				{
					delete[] genes;
					delete[] matched;
					delete[] offspring1;
					delete[] offspring2;
					_buffer_size = s->getSolutionSize();
					genes = new int[_buffer_size];
					matched = new int[_buffer_size];
					offspring1 = new double[_buffer_size];
					offspring2 = new double[_buffer_size];
				}

				list_size = 0;
				for (int i = 0; i < s->getSolutionSize(); i++)
				{
					if (rand01() < _proportion)
					{
						genes[list_size] = i;
						list_size++;
					}
				}
			}
		}

		void apply(Solution* child, Solution* s1, Solution** s2, ProblemHandle* handle)
		{
			preparation(child);

			if (is_crossover)
			{
				if (new_pair)
				{
					int solution_size = child->getSolutionSize();

					memcpy(offspring1, s1->result, solution_size * sizeof(double));
					memcpy(offspring2, s2[0]->result, solution_size * sizeof(double));
					
					buildMap(s1->result, s2[0]->result, solution_size, genes, matched, list_size);
					
					// 选择的基因从学习对象选取，其余直接进入子代
					// (这其实是order-based的做法，但这两个算子并没有本质不同，且order-based与orderX命名相似，所以使用position-based的命名)
					std::sort(matched, matched + list_size);
					int gene_index = 0;
					for (int i = 0; i < solution_size; i++)
					{
						if (i == genes[gene_index]) // 被选择的基因位
						{
							std::swap(offspring1[i], offspring2[matched[gene_index]]);
							gene_index++;
							if (gene_index == list_size)
								break;
						}
					}

					memcpy(child->result, offspring1, solution_size * sizeof(double));
				}
				else
				{
					memcpy(child->result, offspring2, child->getSolutionSize() * sizeof(double));
				}

			}
			else
			{
				memcpy(child->result, s1->result, child->getSolutionSize() * sizeof(double));
			}

			ending();
		}
	};
}