//------------------------Description------------------------
// This file defines the learning topology of level-based pso, where
// individuals are divided into several levels, and the lower particles
//  are learning at two random high-level.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include"learning-topology.h"
#include"individual.h"

namespace ECFC
{
	class LLSOTopology : public InSwarmTopology
	{
	private:
		int _levels;
		int _order_size;
		int* _orders;

		void setSize(int size)
		{
			if (_order_size < size)
			{
				delete[] _orders;
				_order_size = size;
				_orders = new int[_order_size];
			}
		}

		void update(Population& swarm)
		{
			int swarm_size = swarm.getSize();
			setSize(swarm_size);
			for (int i = 0; i < swarm_size; i++)
			{
				_orders[i] = i;
			}

			// 插入排序(因为后代很可能也是序良好的)
			for (int i = 1; i < swarm_size; i++)
			{
				int j = i - 1;
				int key = _orders[i];
				while ((j >= 0) && (swarm[key] < swarm[_orders[j]]))
				{
					_orders[j + 1] = _orders[j];
					j--;
				}
				_orders[j + 1] = key;
			}
		}

	public:
		LLSOTopology(int level_number = 4)
			:InSwarmTopology()
		{
			_levels = level_number;
			_order_size = 0;
			_orders = nullptr;
		}

		~LLSOTopology()
		{
			delete[] _orders;
		}

		static void preAssert(AssertList& list, double* paras)
		{
			// none
		}

		static void postAssert(AssertList& list, double* paras)
		{
			list.add(new Assert(ModuleType::T_learntopology, "objects", 2, MatchType::postAssert));
		}

		void ini()
		{

		}

		LearningGraph* getTopology(Population** subswarm, BestArchive** best_holder, const int swarm_number)
		{
			Population* cswarm = subswarm[0];
			int graph_size = cswarm->getSize();
			LearningGraph* back = new LearningGraph(graph_size, 2);

			int sid1, sid2;
			Individual* s1;
			Solution* s2;
			int level_capacity = ceil(double(graph_size) / _levels);

			// 更新排序
			update(*cswarm);

			// 设置第一层级
			for (int cid = 0; cid < level_capacity; cid++)
			{
				s1 = &cswarm[0][_orders[cid]];
				back->addStart(s1);

				back->addEnd(nullptr);
				back->addEnd(nullptr);
			}

			// 设置后续层级
			int counter = level_capacity;
			for (int lid = 1; lid < _levels; lid++)
			{
				for (int cid = 0; cid < level_capacity && counter < graph_size; cid++) // 计数器避免因最后一层不全导致越界
				{
					// 获取学习对象编号
					sid1 = rand() % lid * level_capacity + rand() % level_capacity;
					sid2 = rand() % lid * level_capacity + rand() % level_capacity;

					// 保证sid1优于sid2
					if (sid2 < sid1)
						std::swap(sid1, sid2);

					// 插入拓扑图
					s1 = &cswarm[0][counter];
					back->addStart(s1);
					counter++;

					// 插入拓扑图
					s2 = &cswarm[0][_orders[sid1]].solution;
					back->addEnd(s2);
					s2 = &cswarm[0][_orders[sid2]].solution;
					back->addEnd(s2);
				}
			}

			return back;
		}
	};
}