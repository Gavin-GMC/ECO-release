//------------------------Description------------------------
// This file is consist of class "AssertMtacher", which define the basic
// data format of assert system, and class "AssertList", which 
// provide a container for the assertion object
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include"assert.h"
#include"configurelist.h"
#include"setter-subpopulation.h"
#include"offspring-generator-factory.h"
#include"smanager-factory.h"
#include"bestarchive-factory.h"
#include"selector-factory.h"
#include"initializer-factory.h"

namespace ECFC
{
	class AssertMatcher
	{
	private:
		static AssertList* getPreAssertList(ModuleType mtype, int ctype, double* paras)
		{
			switch (mtype)
			{
			case ECFC::ModuleType::T_individual:
				return IndividualFactory::preAssert(IndividualType(ctype), paras);
			case ECFC::ModuleType::T_learnstrategy:
				return LStrategyFactory::preAssert(LearningStrategyType(ctype), paras);
			case ECFC::ModuleType::T_learntopology:
				return LTopologyFactory::preAssert(LearningTopologyType(ctype), paras);
			case ECFC::ModuleType::T_offspringgenerator:
				return OgeneratorFactory::preAssert(OffspringGeneratorType(ctype), paras);
			case ECFC::ModuleType::T_selector:
				return SelectorFactory::preAssert(SelectorType(ctype), paras);
			case ECFC::ModuleType::T_subswarbuilder:
				return SbuilderFactory::preAssert(SubswarmConstructerType(ctype), paras);
			case ECFC::ModuleType::T_subswarmtopology:
				return STopologyFactory::preAssert(SubswarmTopologyType(ctype), paras);
			case ECFC::ModuleType::T_subswarmmanager:
				return SManagerFactory::preAssert(SubswarmManagerType(ctype), paras);
			case ECFC::ModuleType::T_bestarchive:
			default:
				return nullptr;
			}
		}

		static AssertList* getPostAssertList(ModuleType mtype, int ctype, double* paras)
		{
			switch (mtype)
			{
			case ECFC::ModuleType::T_individual:
				return IndividualFactory::postAssert(IndividualType(ctype), paras);
			case ECFC::ModuleType::T_learnstrategy:
				return LStrategyFactory::postAssert(LearningStrategyType(ctype), paras);
			case ECFC::ModuleType::T_learntopology:
				return LTopologyFactory::postAssert(LearningTopologyType(ctype), paras);
			case ECFC::ModuleType::T_offspringgenerator:
				return OgeneratorFactory::postAssert(OffspringGeneratorType(ctype), paras);
			case ECFC::ModuleType::T_selector:
				return SelectorFactory::postAssert(SelectorType(ctype), paras);
			case ECFC::ModuleType::T_subswarbuilder:
				return SbuilderFactory::postAssert(SubswarmConstructerType(ctype), paras);
			case ECFC::ModuleType::T_subswarmtopology:
				return STopologyFactory::postAssert(SubswarmTopologyType(ctype), paras);
			case ECFC::ModuleType::T_subswarmmanager:
				return SManagerFactory::postAssert(SubswarmManagerType(ctype), paras);
			case ECFC::ModuleType::T_bestarchive:
			default:
				return nullptr;
			}
		}

		static std::string getModuleName(ModuleType mtype)
		{
			switch (mtype)
			{
			case ECFC::ModuleType::T_individual:
				return "Individual";
			case ECFC::ModuleType::T_learnstrategy:
				return "Learning Strategy";
			case ECFC::ModuleType::T_learntopology:
				return "Learning Topology";
			case ECFC::ModuleType::T_offspringgenerator:
				return "Offspring generator";
			case ECFC::ModuleType::T_selector:
				return "Environment Selector";
			case ECFC::ModuleType::T_subswarbuilder:
				return "Subpopulation Constructor";
			case ECFC::ModuleType::T_subswarmtopology:
				return "Communication Model";
			case ECFC::ModuleType::T_subswarmmanager:
				return "Subpopulation Manager";
			case ECFC::ModuleType::T_bestarchive:
				return "Best Archive";
			default:
				return "";
			}
		}

		static void extractConfig_P(ConfigureList& list, ModuleType mtype, int& ctype, double* &paras)
		{
			switch (mtype)
			{
			case ECFC::ModuleType::T_individual:
				ctype = int(list.individual_type);
				paras = nullptr;
				return;
			case ECFC::ModuleType::T_subswarbuilder:
				ctype = int(list.sbuilder_type);
				paras = list.sbuilder_para;
				return;
			case ECFC::ModuleType::T_subswarmtopology:
				ctype = int(list.stopology_type);
				paras = list.stopology_para;
				return;
			case ECFC::ModuleType::T_subswarmmanager:
				ctype = int(list.smanager_type);
				paras = list.s_para;
				return;
			case ECFC::ModuleType::T_bestarchive:
				ctype = int(list.archive_type);
				paras = nullptr;
				return;
			case ECFC::ModuleType::T_learnstrategy:
			case ECFC::ModuleType::T_learntopology:
			case ECFC::ModuleType::T_offspringgenerator:
			case ECFC::ModuleType::T_selector:
			default:
				return;
			}
		}

		static void extractConfig_SP(ConfigureList::PopulationConfig& list, ModuleType mtype, int& ctype, double*& paras)
		{
			switch (mtype)
			{
			case ECFC::ModuleType::T_learnstrategy:
				ctype = int(list.lstrategy_type);
				paras = list.lstrategy_para;
				return;
			case ECFC::ModuleType::T_learntopology:
				ctype = int(list.ltopology_type);
				paras = list.ltopology_para;
				return;
			case ECFC::ModuleType::T_offspringgenerator:
				ctype = int(list.lgenerator_type);
				paras = list.l_para;
				return;
			case ECFC::ModuleType::T_selector:
				ctype = int(list.selector_type);
				paras = list.selector_para;
				return;
			case ECFC::ModuleType::T_bestarchive:
				ctype = int(list.archive_type);
				paras = nullptr;
				return;
			case ECFC::ModuleType::T_individual:
			case ECFC::ModuleType::T_subswarbuilder:
			case ECFC::ModuleType::T_subswarmtopology:
			case ECFC::ModuleType::T_subswarmmanager:
			default:
				return;
			}
		}

		static Assert::MatchResult assertMatch(Assert& preassert, AssertList& postassert, int& matched_id)
		{
			Assert::MatchResult back = Assert::MatchResult::unsatisfied;
			Assert::MatchResult buffer;
			matched_id = -1;

			int list_size = postassert.getSize();
			for (int i = 0; i < list_size; i++) // 逐个尝试匹配
			{
				buffer = preassert.match(postassert[i]);
				if (buffer > back) // 依照结果优先级进行覆写
				{
					back = buffer;
					matched_id = i;
				}
			}
			return back;
		}

		static bool preassertMatch_P(AssertList* preassert, ConfigureList& list)
		{
			int list_size = preassert->getSize();
			ModuleType mtype;
			int ctype;
			double* paras;
			AssertList* postassert;
			Assert::MatchResult match_result;
			int match_id;
			for (int i = 0; i < list_size; i++)
			{
				mtype = (*preassert)[i].getModuleType();
				extractConfig_P(list, mtype, ctype, paras);
				if (ctype != 0) // 不是默认设置
				{
					postassert = getPostAssertList(mtype, ctype, paras);
					match_result = assertMatch((*preassert)[i], *postassert, match_id);

					bool process_result = resultProcess(mtype, match_result, (*preassert)[i], "population strategy");
					delete postassert;

					if (!process_result)
					{
						return false;
					}
				}
			}

			return true;
		}

		static bool preassertMatch_SP(AssertList* preassert, ConfigureList::PopulationConfig& list)
		{
			int list_size = preassert->getSize();
			ModuleType mtype;
			int ctype;
			double* paras;
			AssertList* postassert;
			Assert::MatchResult match_result;
			int match_id;
			for (int i = 0; i < list_size; i++)
			{
				mtype = (*preassert)[i].getModuleType();
				extractConfig_SP(list, mtype, ctype, paras);
				if (ctype != 0) // 不是默认设置
				{
					postassert = getPostAssertList(mtype, ctype, paras);
					match_result = assertMatch((*preassert)[i], *postassert, match_id);

					bool process_result = resultProcess(mtype, match_result, (*preassert)[i], "subpopulation: " + list.tag);
					delete postassert;

					if (!process_result)
					{
						return false;
					}
				}
			}

			return true;
		}

		static bool resultProcess(ModuleType mtype, Assert::MatchResult result, Assert& preassert, std::string tag)
		{
			switch (result)
			{
			case ECFC::Assert::MatchResult::unsatisfied:
				sys_logger.error("(" + tag + ") The component set for " + getModuleName(mtype) + " did not find a matching assertion");
				return false;
			case ECFC::Assert::MatchResult::satisfied:
				return true;
			case ECFC::Assert::MatchResult::notfullysatisfied:
				sys_logger.warning("(" + tag + ") The component set for " + getModuleName(mtype) + " produced an admissible error in the assertion match");
				return true;
			case ECFC::Assert::MatchResult::failsatisfied:
				sys_logger.error("(" + tag + ") The component set for " + getModuleName(mtype) + " is not satisfied in the assertion match");
				return false;
			default:
				sys_logger.error("(" + tag + ") An exception occurred in the assertion matching!");
				return false;
			}
		}

		static bool subpopulationCheck(ConfigureList::PopulationConfig& list)
		{
			// offspring generator

			// learning topology

			// learning strategy

			// environment selector

		}

		static bool subpopulationComplete(ConfigureList::PopulationConfig& list)
		{
			// offspring generator
			if (list.lgenerator_type != OffspringGeneratorType::F_default)
			{
				AssertList* preassert = getPreAssertList(ModuleType::T_offspringgenerator, int(list.lgenerator_type), list.l_para);
				if (!preassertMatch_SP(preassert, list))
					return false;
			}

			// learning topology
			if (list.ltopology_type != LearningTopologyType::F_default)
			{
				AssertList* preassert = getPreAssertList(ModuleType::T_learntopology, int(list.ltopology_type), list.ltopology_para);
				if (!preassertMatch_SP(preassert, list))
					return false;
			}

			// learning strategy
			if (list.lgenerator_type != OffspringGeneratorType::F_default)
			{
				AssertList* preassert = getPreAssertList(ModuleType::T_subswarmmanager, int(list.lgenerator_type), list.l_para);
				if (!preassertMatch_SP(preassert, list))
					return false;
			}

			// environment selector
			if (list.lgenerator_type != OffspringGeneratorType::F_default)
			{
				AssertList* preassert = getPreAssertList(ModuleType::T_subswarmmanager, int(list.lgenerator_type), list.l_para);
				if (!preassertMatch_SP(preassert, list))
					return false;
			}

			// 匹配成功
			return true;
		}

		static bool populationCheck(ConfigureList& list)
		{
			// manager
			if (list.smanager_type != SubswarmManagerType::F_default)
			{
				AssertList* preassert = getPreAssertList(ModuleType::T_subswarmmanager, int(list.smanager_type), list.s_para);
				if (!preassertMatch_P(preassert, list))
					return false;
			}

			// topology
			if (list.stopology_type != SubswarmTopologyType::F_default)
			{
				AssertList* preassert = getPreAssertList(ModuleType::T_subswarmtopology, int(list.stopology_type), list.stopology_para);
				if (!preassertMatch_P(preassert, list))
					return false;
			}

			// constructer
			if (list.sbuilder_type != SubswarmConstructerType::F_default)
			{
				AssertList* preassert = getPreAssertList(ModuleType::T_subswarbuilder, int(list.sbuilder_type), list.sbuilder_para);
				if (!preassertMatch_P(preassert, list))
					return false;
			}

			// 匹配成功
			return true;
		}

		static bool populationComplete(ConfigureList& list)
		{
			// manager

			// topology

			// constructer
		}

	public:
		static bool checkConfig(ConfigureList& _list)
		{
			bool pass;

			pass = populationComplete(_list);
			if (!pass)
			{
				sys_logger.error("The configuration check failed when checking population cooperation strategy."
					" There is no component that meets the requirements of all the other components that have been set up.");
				return false;
			}
			for (int i = 0; i < _list.subpopulations.size(); i++)
			{
				pass = subpopulationComplete(_list.subpopulations[i]);
				if (!pass)
				{
					sys_logger.error("The configuration complete failed when checking subpopulation " +
						_list.subpopulations[i] .tag +
						". There is no component that meets the requirements of all the other components that have been set up.");
					return false;
				}
			}

			return true;
		}

		static bool completeConfig(ConfigureList& _list)
		{
			bool pass;

			pass = populationCheck(_list);
			if (!pass)
			{
				sys_logger.error("The configuration check failed when checking population cooperation strategy, please check the requirements of the component being set up.");
				return false;
			}
			for (int i = 0; i < _list.subpopulations.size(); i++)
			{
				pass = subpopulationCheck(_list.subpopulations[i]);
				if (!pass)
				{
					sys_logger.error("The configuration check failed when checking subpopulation " +
						_list.subpopulations[i].tag +
						", please check the requirements of the component being set up.");
					return false;
				}
			}

			return true;
		}
	};
}
