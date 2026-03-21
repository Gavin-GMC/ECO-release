//------------------------Description------------------------
// This file defines setting helper of learning topology, which 
// provides more clearly setting interface.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include"configurelist.h"
#include"learning-topology-type.h"

namespace ECFC
{
	class LTopologySetter
	{
	private:
		ConfigureList::PopulationConfig* _list_pointer;
	public:
		LTopologySetter()
		{
			_list_pointer = nullptr;
		}

		LTopologySetter(ConfigureList::PopulationConfig* clist)
		{
			_list_pointer = clist;
		}

		~LTopologySetter()
		{

		}

		void isolate()
		{
			_list_pointer->ltopology_type = LearningTopologyType::F_isolate;
			_list_pointer->l_para[0] = double(LearningTopologyType::F_isolate);
		}

		void roulrtte(int object_number = 1)
		{
			_list_pointer->ltopology_type = LearningTopologyType::F_roulette;
			_list_pointer->l_para[0] = double(LearningTopologyType::F_roulette);
			_list_pointer->ltopology_para[0] = object_number;
		}

		void championship(int number = 2, int object_number = 1)
		{
			_list_pointer->ltopology_type = LearningTopologyType::F_championship;
			_list_pointer->l_para[0] = double(LearningTopologyType::F_championship);
			_list_pointer->ltopology_para[0] = number;
			_list_pointer->ltopology_para[1] = object_number;
		}

		void uniform(int object_number = 1)
		{
			_list_pointer->ltopology_type = LearningTopologyType::F_uniform;
			_list_pointer->l_para[0] = double(LearningTopologyType::F_uniform);
			_list_pointer->ltopology_para[0] = object_number;
		}

		void random(int object_number)
		{
			_list_pointer->ltopology_type = LearningTopologyType::F_randomlearn;
			_list_pointer->l_para[0] = double(LearningTopologyType::F_randomlearn);
			_list_pointer->ltopology_para[0] = object_number;
		}

		void PSO()
		{
			_list_pointer->ltopology_type = LearningTopologyType::F_pso;
			_list_pointer->l_para[0] = double(LearningTopologyType::F_pso);
		}

		void CSO()
		{
			_list_pointer->ltopology_type = LearningTopologyType::F_cso;
			_list_pointer->l_para[0] = double(LearningTopologyType::F_cso);
		}

		void LLSO(int level_number)
		{
			_list_pointer->ltopology_type = LearningTopologyType::F_llso;
			_list_pointer->l_para[0] = double(LearningTopologyType::F_llso);
			_list_pointer->ltopology_para[0] = level_number;
		}

		void SDLSO()
		{
			_list_pointer->ltopology_type = LearningTopologyType::F_sdlso;
			_list_pointer->l_para[0] = double(LearningTopologyType::F_sdlso);
		}
	};
}