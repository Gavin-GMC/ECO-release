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

namespace ECFC
{
	//不对种群进行更新（谨慎使用！适用于已经在子代产生中完成更新时的情况（需添加弱断言））
	class NoUpdater final : public EnvirSelect
	{
	public:
		NoUpdater()
			:EnvirSelect(false)
		{

		}

		~NoUpdater()
		{

		}

		static void preAssert(AssertList& list, double* paras)
		{
			list.add(new Assert(ModuleType::T_offspringgenerator, "updated", 1, MatchType::anyButNotice));
		}

		static void postAssert(AssertList& list, double* paras)
		{
			// none
		}

		void update_subswarm(Population& parent, Population& offspring)
		{
			return;
		}
	};

	//根据索引进行替换
	class IndexUpdater final : public EnvirSelect
	{
	public:
		IndexUpdater(bool better_replace)
			:EnvirSelect(better_replace)
		{

		}

		~IndexUpdater()
		{

		}

		static void preAssert(AssertList& list, double* paras)
		{
			// none
		}

		static void postAssert(AssertList& list, double* paras)
		{
			// none
		}

		void update_subswarm(Population& parent, Population& offspring)
		{
			//替换的范围为两个群体大小的最小值
			int population_size = parent.getSize();
			if (offspring.getSize() < population_size)
			{
				population_size = offspring.getSize();
			}

			//执行替换，swap指针以减少开销
			for (int i = 0; i < population_size; i++)
			{
				replace(offspring[i], parent[i]);
			}
		}
	};

	//从亲代和子代中选取最优的部分进入下一代（适用于个体比较器已定义了完备偏序关系的情况）
	class RankUpdater final : public EnvirSelect
	{
	public:
		RankUpdater()
			:EnvirSelect(true)
		{

		}

		~RankUpdater()
		{

		}

		static void preAssert(AssertList& list, double* paras)
		{
			// none
		}

		static void postAssert(AssertList& list, double* paras)
		{
			// none
		}

		void update_subswarm(Population& parent, Population& offspring)
		{
			//对两个群体分别排序
			parent.sort();
			offspring.sort();

			//采用先统计后合并的方式，比直接插入排序减少了大量的插入操作
			//统计新群体的构成比例
			int parent_number = 0;
			int offspring_number = 0;
			int counter = 0;
			while (counter < parent.getSize())
			{
				if (offspring[offspring_number] < parent[parent_number])
				{
					offspring_number++;
				}
				else
				{
					parent_number++;
				}
                counter++;
			}

			//合并新种群
			offspring_number--;
			parent_number--;
			counter--;
			while (parent_number != -1 && offspring_number != -1)
			{
				if (offspring[offspring_number] < parent[parent_number])
				{
					parent[counter].swap(offspring[offspring_number]);
					offspring_number--;
					counter--;
				}
				else
				{
					parent[counter].swap(parent[parent_number]);
					parent_number--;
					counter--;
				}
			}
			while (offspring_number != -1)
			{
				parent[counter].swap(offspring[offspring_number]);
				offspring_number--;
				counter--;
			}
		}
	};

	//使用子代替代与子代个体最相似的个体
	class CloseUpdater final : public EnvirSelect
	{
	public:
		CloseUpdater(bool better_replace)
			:EnvirSelect(better_replace)
		{

		}

		~CloseUpdater()
		{

		}

		static void preAssert(AssertList& list, double* paras)
		{
			// none
		}

		static void postAssert(AssertList& list, double* paras)
		{
			// none
		}

		void update_subswarm(Population& parent, Population& offspring)
		{
			//替换的范围为两个群体大小的最小值
			int population_size = offspring.getSize();

			//执行替换
			double distance_min;
			double distance_current;
			int distance_min_id;
			for (int offspring_id = 0; offspring_id < population_size; offspring_id++)
			{
				//初始化
				distance_min = eu_distance(offspring[offspring_id].solution.result, parent[0].solution.result);
				distance_min_id = 0;

				//查找最相似父代
				for (int parent_id = 1; parent_id < parent.getSize(); parent_id++)
				{
					distance_current = eu_distance(offspring[offspring_id].solution.result, parent[parent_id].solution.result);
					if (distance_current < distance_min)
					{
						distance_min = distance_current;
						distance_min_id = parent_id;
					}
				}

				//替换
				replace(offspring[offspring_id], parent[distance_min_id]);
			}
		}
	};
}
