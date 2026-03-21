//------------------------Description------------------------
// This file defines the learning topology of pso, where
// individuals learn from gbest and its pbest.
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
#include"individual-pbest.h"

namespace ECFC
{
	class PGBestTopology : public InSwarmTopology
	{
	private:

	public:
		PGBestTopology()
			:InSwarmTopology()
		{

		}

		~PGBestTopology()
		{

		}

		
		static void preAssert(AssertList& list, double* paras)
		{
			list.add(new Assert(ModuleType::T_individual, "pbest", 1, MatchType::notLess));
		}

		static void postAssert(AssertList& list, double* paras)
		{
			list.add(new Assert(ModuleType::T_learntopology, "objects", 2, MatchType::postAssert));
		}

		void ini()
		{

		}

		LearningGraph* getTopology(Population** subswarm, BestArchive** best_harchive, const int swarm_number)
		{
			Population* cswarm = subswarm[0];
			BestArchive* cholder = best_harchive[0];
			int graph_size = cswarm->getSize();
			LearningGraph* back = new LearningGraph(graph_size, 2);
			Individual* p_buffer;
			PbestIndividual* pointer;

			for (int i = 0; i < graph_size; i++)
			{
				p_buffer = const_cast<Individual*>(&cswarm[0][i]);
				pointer = static_cast<PbestIndividual*> (p_buffer);
				back->addStart(p_buffer);
				back->addEnd(cholder->getBest());
				back->addEnd(&pointer->pbest);
			}
			return back;
		}
	};
}