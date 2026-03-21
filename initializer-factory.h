//------------------------Description------------------------
// This file defines the factory object of individual initializers.
// It can product different individual initializer based on types.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include"initializer-type.h"
#include"solution-initializer.h"

namespace ECFC
{
	class InitializerFactory
	{
	public:
		static SolutionInitializer* newInitializer(InitializerType type)
		{
			switch (type)
			{
			case ECFC::InitializerType::F_random:
				return new RandomInitializer();
			case ECFC::InitializerType::F_greedy:
				return new GreedyInitializer();
			case ECFC::InitializerType::F_no:
				return new NoInitializer();
			case ECFC::InitializerType::F_default:
			case ECFC::InitializerType::F_end:
			default:
				return nullptr;
			}
		}

		static size_t typeSize(InitializerType type)
		{
			switch (type)
			{
			case ECFC::InitializerType::F_random:
				return sizeof(RandomInitializer);
			case ECFC::InitializerType::F_greedy:
				return sizeof(GreedyInitializer);
			case ECFC::InitializerType::F_no:
				return sizeof(NoInitializer);
			case ECFC::InitializerType::F_default:
			case ECFC::InitializerType::F_end:
			default:
				return 0;
			}
		}

	};
}
