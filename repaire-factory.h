//------------------------Description------------------------
// This file is the factory class of DistributionModel classes,
// which is used to build objects from types
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include"repaire-type.h"
#include"solution-repairman.h"

namespace ECFC
{
	class RepaireFactory
	{
	public:
		static SolutionRepaireman* newRepair(RepairType type)
		{
			switch (type)
			{
			case ECFC::RepairType::F_boundary:
				return new BoundaryRepair();
			case ECFC::RepairType::F_greedy:
				return new GreedyRepair();
			case ECFC::RepairType::F_no:
				return new NoRepair();
			case ECFC::RepairType::F_default:
			case ECFC::RepairType::F_random:
				return new RandomValueRepair();
			case ECFC::RepairType::F_end:
			default:
				return nullptr;
			}
		}

		static size_t typeSize(RepairType type)
		{
			switch (type)
			{
			case ECFC::RepairType::F_boundary:
				return sizeof(BoundaryRepair);
			case ECFC::RepairType::F_greedy:
				return 0;
				//return sizeof(GreedyRepair);
			case ECFC::RepairType::F_no:
				return sizeof(NoRepair);
			case ECFC::RepairType::F_random:
				return sizeof(RandomValueRepair);
			case ECFC::RepairType::F_end:
			case ECFC::RepairType::F_default:
			default:
				return 0;
			}
		}
	};
}
