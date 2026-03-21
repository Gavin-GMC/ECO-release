//------------------------Description------------------------
// This file defines the learning topology of isolate, where
// individuals do not learn from any special individual, for example aco.
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
	class IsolateTopology : public InSwarmTopology
	{
	private:

	public:
		IsolateTopology()
			:InSwarmTopology()
		{

		}

		~IsolateTopology()
		{

		}

		static void preAssert(AssertList& list, double* paras)
		{
			// none
		}

		static void postAssert(AssertList& list, double* paras)
		{
			list.add(new Assert(ModuleType::T_learntopology, "object", 0, MatchType::postAssert));
		}

		void ini()
		{

		}

		LearningGraph* getTopology(Population** subswarm, BestArchive** best_holder, const int swarm_number)
		{
			Population* cswarm = subswarm[0];
			int graph_size = cswarm->getSize();
			LearningGraph* back = new LearningGraph(graph_size, 0);

			Individual* s1;
			for (int i = 0; i < graph_size; i++)
			{
				s1 = &cswarm[0][i];
				// 插入拓扑图
				back->addStart(s1);
			}

			return back;
		}
	};
}