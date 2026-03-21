//------------------------Description------------------------
// This file is consist of all Crossover types that is 
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
	enum class CrossoverType
	{
		F_default,
		F_point,
		F_SBX,
		F_uniform,
		F_difference,
		F_no,
		F_partialmap,
		F_cycle,
		F_order,
		F_subtourExchange,
		F_positionBased,
		F_end
	};
}