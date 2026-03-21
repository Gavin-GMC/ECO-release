//------------------------Description------------------------
// This file defines environment selection object. It represents 
// the process by which offspring and parents form new populations 
// together. In this process, individuals who are not adapted to
//  their environment may be eliminated, thus promoting population
//  evolution and optimization.
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
#include"sort-helper.h"
#include"solution.h"
#include"population.h"

namespace ECFC
{
	// 后续移入cpp
	void replacer(Individual& offspring, Individual& parent)
	{
		parent.swap(offspring);
		parent.rebirth();
	}

	void betterReplacer(Individual& offspring, Individual& parent)
	{
		if (offspring < parent)
		{
			replacer(offspring, parent);
			parent.rebirth();
		}
	}

	class EnvirSelect
	{
	protected:
		void (*replace)(Individual& offspring, Individual& parent);
	public:
		EnvirSelect(bool better_replace)
		{
			if (better_replace)
				replace = betterReplacer;
			else
				replace = replacer;
		}

		virtual ~EnvirSelect()
		{

		}

		virtual void update_subswarm(Population& parent, Population& offspring) = 0;
	};
}
