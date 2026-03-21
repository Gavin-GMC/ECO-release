//------------------------Description------------------------
// This file defines the learning topology of Stochastic Dominant
// Learning Swarm Optimizer, where learning objects are randomly 
// selected from the population and only learn if the object is
// superior to itself.
//-------------------------Reference-------------------------
// [1] Q. Yang, W. -N. Chen, T. Gu, H. Jin, W. Mao and J. Zhang, "An Adaptive Stochastic Dominant Learning Swarm Optimizer for High-Dimensional Optimization," 
// in IEEE Transactions on Cybernetics, vol. 52, no. 3, pp. 1960-1976, March 2022, doi: 10.1109/TCYB.2020.3034427. 
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
	// Stochastic Dominant Learning Swarm Optimizer
	class SDLTopology : public InSwarmTopology
	{
	public:
		SDLTopology()
			:InSwarmTopology()
		{
			
		}

		~SDLTopology()
		{

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

			for (int sid = 0; sid < graph_size; sid++)
			{
				// 插入起始点
				back->addStart(&cswarm[0][sid]);

				// 随机选择两个个体
				sid1 = rand() % graph_size;
				sid2 = rand() % graph_size;

				// 根据优劣对两个对象进行排序
				if (cswarm[0][sid2] < cswarm[0][sid1])
				{
					std::swap(sid1, sid2);
				}
				// 判断与当前个体的优劣关系
				if (cswarm[0][sid] < cswarm[0][sid2])
				{
					// 放弃学习
					back->addEnd(nullptr);
				}
				else
				{
					// 插入拓扑图
					back->addEnd(&cswarm[0][sid1].solution);
					back->addEnd(&cswarm[0][sid2].solution);
				}
			}

			return back;
		}
	};
}