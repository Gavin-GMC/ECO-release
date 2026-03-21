//------------------------Description------------------------
// This file is consisted of some system funcions of ECFC.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include"basicfunc.h"
#include"logger.h"

namespace ECFC
{
	//system environment initialization
	//系统环境初始化
	void sys_environment_ini()
	{
		try
		{
			dir_create("_log");
			dir_create("_config");
			dir_create("_resl");
		}
		catch(std::exception e)
		{
			sys_logger.error("Failing create system dictionary!");
		}

	}
}