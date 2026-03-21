//------------------------Description------------------------
// This file is the factory class of Learning topology classes,
// which is used to build objects from types
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------


#pragma once
#include"ecfc-constant.h"
#include"learning-topology-type.h"
#include"ltopology-cso.h"
#include"ltopology-de.h"
#include"ltopology-ga.h"
#include"ltopology-isolate.h"
#include"ltopology-llso.h"
#include"ltopology-pso.h"
#include"ltopology-sdlso.h"

namespace ECFC
{
	class LTopologyFactory
	{
	public:
		static InSwarmTopology* produce(LearningTopologyType type, double* parameters)
		{
			switch (type)
			{
			case ECFC::LearningTopologyType::F_isolate:
				return new IsolateTopology();
			case ECFC::LearningTopologyType::F_roulette:
				return new Roulette(parameters[0]);
			case ECFC::LearningTopologyType::F_championship:
				return new Championship(parameters[0], parameters[1]);
			case ECFC::LearningTopologyType::F_uniform:
				return new Uniform(parameters[0]);
			case ECFC::LearningTopologyType::F_randomlearn:
				return new RandomLearning(parameters[0]);
			case ECFC::LearningTopologyType::F_pso:
				return new PGBestTopology();
			case ECFC::LearningTopologyType::F_cso:
				return new CSOTopology();
			case ECFC::LearningTopologyType::F_llso:
				return new LLSOTopology(parameters[0]);
			case ECFC::LearningTopologyType::F_sdlso:
				return new SDLTopology();
			case ECFC::LearningTopologyType::F_default:
			case ECFC::LearningTopologyType::F_end:
			default:
				return nullptr;
			}
		}

		static size_t typeSize(LearningTopologyType type)
		{
			switch (type)
			{
			case ECFC::LearningTopologyType::F_isolate:
				return sizeof(IsolateTopology);
			case ECFC::LearningTopologyType::F_roulette:
				return sizeof(Roulette);
			case ECFC::LearningTopologyType::F_championship:
				return sizeof(Championship);
			case ECFC::LearningTopologyType::F_uniform:
				return sizeof(Uniform);
			case ECFC::LearningTopologyType::F_randomlearn:
				return sizeof(RandomLearning);
			case ECFC::LearningTopologyType::F_pso:
				return sizeof(PGBestTopology);
			case ECFC::LearningTopologyType::F_cso:
				return sizeof(CSOTopology);
			case ECFC::LearningTopologyType::F_llso:
				return sizeof(LLSOTopology);
			case ECFC::LearningTopologyType::F_sdlso:
				return sizeof(SDLTopology);
			case ECFC::LearningTopologyType::F_default:
			case ECFC::LearningTopologyType::F_end:
			default:
				return 0;
			}
		}

		static AssertList* preAssert(LearningTopologyType type, double* paras)
		{
			AssertList* back = new AssertList();
			switch (type)
			{
			case ECFC::LearningTopologyType::F_isolate:
				IsolateTopology::preAssert(*back, paras);
				break;
			case ECFC::LearningTopologyType::F_roulette:
				Roulette::preAssert(*back, paras);
				break;
			case ECFC::LearningTopologyType::F_championship:
				Championship::preAssert(*back, paras);
				break;
			case ECFC::LearningTopologyType::F_uniform:
				Uniform::preAssert(*back, paras);
				break;
			case ECFC::LearningTopologyType::F_randomlearn:
				RandomLearning::preAssert(*back, paras);
				break;
			case ECFC::LearningTopologyType::F_pso:
				PGBestTopology::preAssert(*back, paras);
				break;
			case ECFC::LearningTopologyType::F_cso:
				CSOTopology::preAssert(*back, paras);
				break;
			case ECFC::LearningTopologyType::F_llso:
				LLSOTopology::preAssert(*back, paras);
				break;
			case ECFC::LearningTopologyType::F_sdlso:
				SDLTopology::preAssert(*back, paras);
				break;
			case ECFC::LearningTopologyType::F_default:
			case ECFC::LearningTopologyType::F_end:
			default:
				break;
			}
			return back;
		}

		static AssertList* postAssert(LearningTopologyType type, double* paras)
		{
			AssertList* back = new AssertList();
			switch (type)
			{
			case ECFC::LearningTopologyType::F_isolate:
				IsolateTopology::postAssert(*back, paras);
				break;
			case ECFC::LearningTopologyType::F_roulette:
				Roulette::postAssert(*back, paras);
				break;
			case ECFC::LearningTopologyType::F_championship:
				Championship::postAssert(*back, paras);
				break;
			case ECFC::LearningTopologyType::F_uniform:
				Uniform::postAssert(*back, paras);
				break;
			case ECFC::LearningTopologyType::F_randomlearn:
				RandomLearning::postAssert(*back, paras);
				break;
			case ECFC::LearningTopologyType::F_pso:
				PGBestTopology::postAssert(*back, paras);
				break;
			case ECFC::LearningTopologyType::F_cso:
				CSOTopology::postAssert(*back, paras);
				break;
			case ECFC::LearningTopologyType::F_llso:
				LLSOTopology::postAssert(*back, paras);
				break;
			case ECFC::LearningTopologyType::F_sdlso:
				SDLTopology::postAssert(*back, paras);
				break;
			case ECFC::LearningTopologyType::F_default:
			case ECFC::LearningTopologyType::F_end:
			default:
				break;
			}
			return back;
		}

		static int paraNum(LearningTopologyType type)
		{
			return 0;
		}
	};
}
