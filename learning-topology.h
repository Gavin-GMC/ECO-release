//------------------------Description------------------------
// This file defines the learning topology of individuals, which
// represents the learning objects selection approaches of individuals.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include"population.h"
#include"best-archive.h"
#include"learning-graph.h"

namespace ECFC
{
	class InSwarmTopology
	{
	public:
		InSwarmTopology() {}

		virtual ~InSwarmTopology()
		{

		}

		virtual void ini() = 0;

		virtual LearningGraph* getTopology(Population** subswarm, BestArchive** best_holder, const int swarm_number) = 0;
	};
}
