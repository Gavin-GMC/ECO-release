//------------------------Description------------------------
// This file is consist of all offspring generator model types that is 
// supported by the library
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------
#pragma once

namespace ECFC
{
	enum class OffspringGeneratorType
	{
		F_default,
		F_generation_nocheck,
		F_generation,
		F_construct_order,
		F_construct_parallel,
		F_end
	};
}