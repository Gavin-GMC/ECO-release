//------------------------Description------------------------
// 
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2022 所有人名称（待确认）, All Rights Reserved.
// You are free to use the myEC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "myEC" and reference "未确定"
//-----------------------------------------------------------

#pragma once
#include"subswarm.h"
#include"subswarm-neighbor.h"

namespace ECFC
{
	/// <summary>
	/// 拓扑模型
	/// </summary>
	class TopologyModel
	{
	public:
		NeighborSwarm* neighborhoods;

		TopologyModel()
		{
			neighborhoods = nullptr;
		}

		virtual ~TopologyModel()
		{
			delete[] neighborhoods;
		}

		virtual void build(Subswarm** subswarms, int& swarm_number) = 0;

		virtual void ini(Subswarm** subswarms, int& swarm_number) = 0;

		virtual void update(Subswarm** subswarms, int& swarm_number) {}
	};
}