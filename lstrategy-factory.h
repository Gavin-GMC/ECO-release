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
#include"learning-strategy-type.h"
#include"learning-strategy.h"
#include"lstrategy-ga.h"
#include"lstrategy-de.h"
#include"lstrategy-eda.h"
#include"lstrategy-pso.h"
#include"lstrategy-bpso.h"
#include"lstrategy-aco.h"
#include"lstrategy-setpso.h"
#include"mutation-factory.h"
#include"crossover-factory.h"

namespace ECFC
{
	class LStrategyFactory
	{
	public:
		static LearningStrategy* produce(LearningStrategyType type, double* parameters)
		{
			switch (type)
			{
			case ECFC::LearningStrategyType::F_GA:
				return new GAStrategy(MutationFactory::produce(MutationType(parameters[SUBPARA]), parameters + 1 + SUBPARA), CrossoverFactory::produce(CrossoverType(parameters[0]), parameters + 1));
			case ECFC::LearningStrategyType::F_DE:
				return new DEStrategy(parameters[0], CrossoverFactory::produce(CrossoverType(parameters[1]), parameters + 2));
			case ECFC::LearningStrategyType::F_EDA:
				return new EDAStrategy(DistributionType(parameters[0]), parameters[1], parameters[2]);
			case ECFC::LearningStrategyType::F_PSO:
			{
				double* c = new double[int(parameters[0])];
				memcpy(c, parameters + 3, sizeof(double) * parameters[0]);
				LearningStrategy* buffer = new PSOStrategy(parameters[0], c, parameters[1], parameters[2]);
				delete[] c;
				return buffer;
			}	
			case ECFC::LearningStrategyType::F_BPSO:
				return new BPSOStrategy(parameters[0], parameters[1], parameters[2], parameters[3]);
			case ECFC::LearningStrategyType::F_AS:
				return new ASStrategy(parameters[0], parameters[1], parameters[2], parameters[3]);
			case ECFC::LearningStrategyType::F_ACS:
				return new ACSStrategy(parameters[0], parameters[1], parameters[2], parameters[3], parameters[4], parameters[5]);
			case ECFC::LearningStrategyType::F_SetPSO:
			{
				double* c = new double[int(parameters[0])];
				memcpy(c, parameters + 5, sizeof(double) * parameters[0]);
				LearningStrategy* buffer = new SetPSOStrategy(parameters[0], c, parameters[1], parameters[2], parameters[3], parameters[4]);
				delete[] c;
				return buffer;
			}
			case ECFC::LearningStrategyType::F_default:
			case ECFC::LearningStrategyType::F_end:
			default:
				return nullptr;
			}
		}

		static size_t typeSize(LearningStrategyType type)
		{
			switch (type)
			{
			case ECFC::LearningStrategyType::F_GA:
				return sizeof(GAStrategy);
			case ECFC::LearningStrategyType::F_DE:
				return sizeof(DEStrategy);
			case ECFC::LearningStrategyType::F_EDA:
				return sizeof(EDAStrategy);
			case ECFC::LearningStrategyType::F_PSO:
				return sizeof(PSOStrategy);
			case ECFC::LearningStrategyType::F_BPSO:
				return sizeof(BPSOStrategy);
			case ECFC::LearningStrategyType::F_AS:
				return sizeof(ASStrategy);
			case ECFC::LearningStrategyType::F_ACS:
				return sizeof(ACSStrategy);
			case ECFC::LearningStrategyType::F_SetPSO:
				return sizeof(SetPSOStrategy);
			case ECFC::LearningStrategyType::F_default:
			case ECFC::LearningStrategyType::F_end:
			default:
				return 0;
			}
		}
		
		static AssertList* preAssert(LearningStrategyType type, double* paras)
		{
			AssertList* back = new AssertList();
			switch (type)
			{
			case ECFC::LearningStrategyType::F_GA:
				GAStrategy::preAssert(*back, paras);
				break;
			case ECFC::LearningStrategyType::F_DE:
				DEStrategy::preAssert(*back, paras);
				break;
			case ECFC::LearningStrategyType::F_EDA:
				EDAStrategy::preAssert(*back, paras);
				break;
			case ECFC::LearningStrategyType::F_PSO:
				PSOStrategy::preAssert(*back, paras);
				break;
			case ECFC::LearningStrategyType::F_BPSO:
				BPSOStrategy::preAssert(*back, paras);
				break;
			case ECFC::LearningStrategyType::F_AS:
				ASStrategy::preAssert(*back, paras);
				break;
			case ECFC::LearningStrategyType::F_ACS:
				ACSStrategy::preAssert(*back, paras);
				break;
			case ECFC::LearningStrategyType::F_SetPSO:
				SetPSOStrategy::preAssert(*back, paras);
				break;
			case ECFC::LearningStrategyType::F_default:
			case ECFC::LearningStrategyType::F_end:
			default:
				break;
			}
			return back;
		}

		static AssertList* postAssert(LearningStrategyType type, double* paras)
		{
			AssertList* back = new AssertList();
			switch (type)
			{
			case ECFC::LearningStrategyType::F_GA:
				GAStrategy::postAssert(*back, paras);
				break;
			case ECFC::LearningStrategyType::F_DE:
				DEStrategy::postAssert(*back, paras);
				break;
			case ECFC::LearningStrategyType::F_EDA:
				EDAStrategy::postAssert(*back, paras);
				break;
			case ECFC::LearningStrategyType::F_PSO:
				PSOStrategy::postAssert(*back, paras);
				break;
			case ECFC::LearningStrategyType::F_BPSO:
				BPSOStrategy::postAssert(*back, paras);
				break;
			case ECFC::LearningStrategyType::F_AS:
				ASStrategy::postAssert(*back, paras);
				break;
			case ECFC::LearningStrategyType::F_ACS:
				ACSStrategy::postAssert(*back, paras);
				break;
			case ECFC::LearningStrategyType::F_SetPSO:
				SetPSOStrategy::postAssert(*back, paras);
				break;
			case ECFC::LearningStrategyType::F_default:
			case ECFC::LearningStrategyType::F_end:
			default:
				break;
			}
			return back;
		}
	};
}
