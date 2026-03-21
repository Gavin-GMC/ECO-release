//------------------------Description------------------------
// This file is consist of all Mutation types that is 
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
	enum class MutationType
	{
		F_default,
		F_bit,
		F_bitflip,
		F_overturn,
		F_exchange,
		F_PM,
		F_gauss,
		F_no,
		F_insert,
		F_reorder,
		F_end
	};
}