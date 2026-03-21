//------------------------Description------------------------
// This file is consisted of some basic, necessary, widely used
//  constants and control variables.
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
#define ECFC_MAX 2147483647
#define ECFC_MIN -2147483647
#define E_CONST 2.718281828459045
#define PI_CONST 3.1415926535624
#define MAXVARIABLE 100 // 可定义的变量数目

	const int PARANUM = 100; // 设置中允许的参数数目
	const int SUBPARA = 25;
	const int PKSIZE = 100;
	const int PFSIZE = 200;

	int EMPTYVALUE = -2139062144;
	double EQACCURACY = 1e-8;
}
