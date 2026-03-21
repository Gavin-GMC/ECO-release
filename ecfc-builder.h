//------------------------Description------------------------
// This file provides a optimizer builder of ECFC. It provides 
// a unified interface for users to define and build optimizer.
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
#include"setter-subpopulation.h"
#include"offspring-generator-factory.h"
#include"smanager-factory.h"
#include"bestarchive-factory.h"
#include"selector-factory.h"
#include"initializer-factory.h"
#include"optimizer.h"


namespace ECFC
{
	class OptimizerBuilder
	{
	private:
		ConfigureList _list;
		std::vector<SubpopulationSetter> _setter_list;
		std::string _tag;

		SolutionInitializer* Ibuild(InitializerType itype)
		{
			return InitializerFactory::newInitializer(itype);
		}

		OffspringGenerator* Obuild(OffspringGeneratorType otype, double* paras)
		{
			OffspringGenerator* back;

			back = OgeneratorFactory::produce(otype, paras);

			return back;
		}

		Population* Pbuild(int swarm_size, IndividualType itype, SolutionInitializer* initial)
		{
			Population* back = new Population(itype, swarm_size);

			for (int i = 0; i < swarm_size; i++)
			{
				(*back)[i].setInitializer(initial);
			}

			return back;
		}

		BestArchive* Arbuild(BestArchiveType atype)
		{
			return ArchiveFactory::produce(atype);
		}

		EnvirSelect* Ebuild(SelectorType stype, double* paras)
		{
			return SelectorFactory::produce(stype, paras);
		}

		Terminator* Tbuild(int* paras)
		{
			Terminator* back = new Terminator();

			// 缺省设置
			if (paras[0] < 0 &&	paras[1] < 0 &&	paras[2] < 0)
			{
				paras[0] = int(1e5);
			}

			if (paras[0] > 0)
			{
				back->setMaxFES(paras[0]);
			}
			if (paras[1] > 0)
			{
				back->setMaxConvergence(paras[1]);
			}
			if (paras[2] > 0)
			{
				back->setMaxTime(paras[2]);
			}
			return back;
		}

		Subswarm* Sbuild(ConfigureList::PopulationConfig* config, Logger* logger)
		{
			SolutionInitializer* ibuffer = Ibuild(config->ini_type);
			Subswarm* back = new Subswarm(config->tag,
				Pbuild(config->size, _list.individual_type, ibuffer), Pbuild(config->size, _list.individual_type, ibuffer),
				Tbuild(config->terminate_conditions), Arbuild(config->archive_type), 
				Ebuild(config->selector_type, config->selector_para),
				ibuffer, Obuild(config->lgenerator_type, config->l_para), logger);

			return back;
		}

		bool filenameCheck(std::string& file_name)
		{
			// 格式检查

			// 前置路径补全
			file_name = "_config/" + file_name + ".cfg";

			return true;
		}

		void defaultCompletion()
		{
			// 基于断言系统的最小化补全
		}

	public:
		OptimizerBuilder()
		{
			_tag = "";
		}

		~OptimizerBuilder()
		{

		}

		void setName(std::string name)
		{
			_list.name = name;
		}

		void setTag(std::string tag)
		{
			_tag = tag;
		}

		void setIndividual(IndividualType type)
		{
			_list.individual_type = type;
		}

		void setSwarmManager(SubswarmManagerType type)
		{
			_list.smanager_type = type;
		}

		void setSwarmConstruct(SubswarmConstructerType type)
		{
			_list.sbuilder_type = type;
			_list.s_para[0] = double(type);
		}

		void setSwarmTopology(SubswarmTopologyType type, int degree = 3)
		{
			_list.stopology_type = type;
			_list.s_para[1 + PARANUM] = double(type);
			_list.stopology_para[0] = degree;
		}

		SubpopulationSetter* addSubpopulation(std::string tag = "")
		{
			int id = _list.subpopulations.size();
			if (tag == "")
			{
				tag = std::to_string(id);
			}
			_list.subpopulations.push_back(ConfigureList::PopulationConfig());
			_list.subpopulations[id].tag = tag;

			_setter_list.push_back(SubpopulationSetter(&_list.subpopulations[id]));
			return &_setter_list[id];
		}

		SubpopulationSetter* getSubpopulation(std::string tag)
		{
			for (int i = 0; i < _setter_list.size(); i++)
			{
				if (_list.subpopulations[i].tag == tag)
				{
					return &_setter_list[i];
				}
			}
			return nullptr;
		}

		void setArchive(BestArchiveType type)
		{
			_list.archive_type = type;
		}

		void setTerminateMAXFES(int times)
		{
			if (times > 0)
			{
				_list.terminate_conditions[0] = times;
			}
		}

		void setTerminateMAXStop(int times)
		{
			if (times > 0)
			{
				_list.terminate_conditions[1] = times;
			}
		}

		void setTerminateMAXTime(int seconds)
		{
			if (seconds > 0)
			{
				_list.terminate_conditions[2] = seconds;
			}
		}

		void setLoggerFull(bool full_print)
		{
			_list.logger_para[0] = full_print;
		}

		void setLoggerProcess(bool process_print, bool full_print = false)
		{
			_list.logger_para[1] = process_print;
			_list.logger_para[2] = full_print;
		}

		void loadConfigure(std::string file_name)
		{
			clearConfigiure();

			if (filenameCheck(file_name))
				_list.load(file_name);
			else
				return;

			for (int i = 0; i < _list.subpopulations.size(); i++)
				_setter_list.push_back(&_list.subpopulations[i]);
		}

		void saveConfigure(bool overwirte = false)
		{
			std::string file_name = _list.name;

			if (filenameCheck(file_name))
			{
				if (!overwirte && file_exist(file_name)) 
				{
					sys_logger.error("Save failed! The configuration file already exists and overwriting is not allowed.");
					return;
				}
				else
				{
					_list.save(file_name);
				}
			}
				
		}

		void clearConfigiure()
		{
			_list.clear();
			_setter_list.clear();
		}

		void randomMultiPopulation()
		{

		}

		void addRandomPopulation()
		{

		}

		Optimizer* build()
		{
			if (_tag == "")
			{
				_tag = std::to_string(time(NULL));
			}

			Logger* logger = new Logger(_list.name, _tag, bool(_list.logger_para[0]), bool(_list.logger_para[1]), bool(_list.logger_para[2]));

			SwarmManager* smanager = SManagerFactory::produce(_list.smanager_type, _list.s_para);

			int subpopulation_number = _list.subpopulations.size();
			smanager->setSwarmNumber(subpopulation_number);
			for (int i = 0; i < subpopulation_number; i++)
			{
				Subswarm* sswarm = Sbuild(&_list.subpopulations[i], logger);
				smanager->setSwarm(sswarm, i);
			}

			BestArchive* g_archive = ArchiveFactory::produce(_list.archive_type);

			Swarm* swarms = new Swarm(smanager, g_archive);
			Terminator* terminator = Tbuild(_list.terminate_conditions);

			Optimizer* back = new Optimizer(swarms, terminator, logger, _tag);

			return back;
		}
	};
}