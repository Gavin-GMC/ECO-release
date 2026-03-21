//------------------------Description------------------------
// This file defines the learning topology of de, including
// random selection.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include"ecfc-constant.h"
#include"learning-topology.h"

namespace ECFC
{
	class RandomLearning : public InSwarmTopology
	{
	private:
		int _number;
		int* indexs;

		// 检查是否存在重复的个体选择
		bool isRepeat(int index)
		{
			for (int i = 0; i < index; i++)
			{
				if (indexs[i] == indexs[index])
					return true;
			}
			return false;
		}

	public:
		RandomLearning(int number)
			:InSwarmTopology()
		{
			_number = number;
			indexs = new int[_number + 2];
		}

		~RandomLearning()
		{

		}

		static void preAssert(AssertList& list, double* paras)
		{
			// none
		}

		static void postAssert(AssertList& list, double* paras)
		{
			list.add(new Assert(ModuleType::T_learntopology, "objects", paras[0], MatchType::postAssert));
		}

		void ini()
		{

		}

		LearningGraph* getTopology(Population** subswarm, BestArchive** best_holder, const int swarm_number)
		{
			Population* cswarm = subswarm[0];
			int graph_size = cswarm->getSize();
			LearningGraph* back = new LearningGraph(graph_size, _number);

			int counter = 0;
			Individual* s1;
			Solution* s2;
			while (counter < graph_size)
			{
				// 选取学习亲本
				s1 = &(*cswarm)[counter];
				back->addStart(s1);
				indexs[0] = counter;

				// 选取学习对象（需要不重复）
				for (int i = 1; i <= _number; i++)
				{
					indexs[i] = rand() % graph_size;
					while (isRepeat(i))
					{
						indexs[i] = rand() % graph_size;
					}

					s2 = &cswarm[0][indexs[i]].solution;
					back->addEnd(s2);
				}

				counter++;
			}

			return back;
		}
	};
}