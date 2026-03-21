//------------------------Description------------------------
// This file is consist of all optimizer module types that 
// support user-defined in the library
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
	enum class ModuleType {
		T_individual,
		T_learnstrategy,
		T_learntopology,
		T_offspringgenerator,
		T_selector,
		T_subswarbuilder,
		T_subswarmtopology,
		T_subswarmmanager,
		T_bestarchive
	};
}
