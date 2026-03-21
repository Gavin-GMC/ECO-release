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
//#include"assert.h"
#include"distribution-model.h"
#include"distribution-type.h"

namespace ECFC
{
	class DistributionFactory
	{
	public:
		static DistributionModel** newModelArray(DistributionType type, int length = 1)
		{
			DistributionModel** back = new DistributionModel * [length];
			switch (type)
			{
			case ECFC::DistributionType::F_Gaussian:
				for (int i = 0; i < length; i++)
					back[i] = new DM_Gaussian();
				return back;
			default:
				return nullptr;
			}
		}

		static size_t typeSize(DistributionType type)
		{
			switch (type)
			{
			case ECFC::DistributionType::F_Gaussian:
				return sizeof(DM_Gaussian);
			default:
				return 0;
			}
		}

		// 分布模型无断言需求
	};
}
