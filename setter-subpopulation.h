//------------------------Description------------------------
// This file defines setting helper of mutation, which 
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
#include"ltopology-setter.h"
#include"lstrategy-setter.h"

namespace ECFC
{
	class SubpopulationSetter
	{
	private:
		ConfigureList::PopulationConfig* _list_pointer;

	public:
		LTopologySetter ltopology;
		LStrategySetter lstrategy;

		SubpopulationSetter(ConfigureList::PopulationConfig* clist)
		{
			_list_pointer = clist;
			ltopology = LTopologySetter(_list_pointer);
			lstrategy = LStrategySetter(_list_pointer);
		}

		~SubpopulationSetter()
		{

		}

		void setTag(std::string tag)
		{
			_list_pointer->tag = tag;
		}

		void setSolutionIni(InitializerType type)
		{
			_list_pointer->ini_type = type;
		}

		void setSwarmSize(int size)
		{
			_list_pointer->size = size;
		}

		void setSelector(SelectorType type, bool better_replace = false)
		{
			_list_pointer->selector_type = type;
			_list_pointer->selector_para[0] = better_replace;
		}

		void setArchive(BestArchiveType type)
		{
			_list_pointer->archive_type = type;
		}

		void setLTopology(LearningTopologyType type, double* para)
		{
			_list_pointer->ltopology_type = type;
			_list_pointer->l_para[0] = double(type);
			memcpy(_list_pointer->lstrategy_para, para, sizeof(double) * PARANUM);
		}

		void setLStrategy(LearningStrategyType type, double* para)
		{
			_list_pointer->lstrategy_type = type;
			_list_pointer->l_para[PARANUM + 1] = double(type);
			memcpy(_list_pointer->lstrategy_para, para, sizeof(double) * PARANUM);
		}

		void setLFramework(OffspringGeneratorType type)
		{
			_list_pointer->lgenerator_type = type;
		}

		void setTerminateMAXFES(int times)
		{
			if (times > 0)
			{
				_list_pointer->terminate_conditions[0] = times;
			}
		}

		void setTerminateMAXStop(int times)
		{
			if (times > 0)
			{
				_list_pointer->terminate_conditions[1] = times;
			}
		}

		void setTerminateMAXTime(int seconds)
		{
			if (seconds > 0)
			{
				_list_pointer->terminate_conditions[2] = seconds;
			}
		}

		void setRepairMethod(RepairType type)
		{
			_list_pointer->l_para[2 * PARANUM + 2] = double(type);
			_list_pointer->repair_type = type;
		}
	};
}