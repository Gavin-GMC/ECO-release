//------------------------Description------------------------
// This file defines the support Learning topology types by ECFC.
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
	enum class LearningTopologyType
	{
		F_default,
		F_isolate,
		F_roulette,
		F_championship,
		F_uniform,
		F_randomlearn,
		F_pso,
		F_cso,
		F_llso,
		F_sdlso,
		F_end
	};
}
