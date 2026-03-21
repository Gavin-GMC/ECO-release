//------------------------Description------------------------
// This file is the factory class of crossover classes,
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
#include"crossover-type.h"
#include"ga-crossover-basic.h"
#include"ga-crossover-sequence.h"
#include"assert.h"

namespace ECFC
{
	class CrossoverFactory
	{
	public:
		static Crossover* produce(CrossoverType type, double* parameters)
		{
			switch (type)
			{
			case ECFC::CrossoverType::F_point:
				return new PointCrossover(parameters[0], parameters[1], parameters[2]);
			case ECFC::CrossoverType::F_SBX:
				return new SBXCrossover(parameters[0], parameters[1], parameters[2]);
			case ECFC::CrossoverType::F_uniform:
				return new UniformCrossover(parameters[0], parameters[1]);
			case ECFC::CrossoverType::F_difference:
				return new DifferenceCrossover(parameters[0], parameters[1], parameters[2]);
			case ECFC::CrossoverType::F_no:
				return new NoCrossover();
			case ECFC::CrossoverType::F_partialmap:
				return new PartialMappedCrossover(parameters[0], parameters[1]);
			case ECFC::CrossoverType::F_cycle:
				return new CycleCrossover(parameters[0], parameters[1]);
			case ECFC::CrossoverType::F_order:
				return new OrderCrossover(parameters[0], parameters[1]);
			case ECFC::CrossoverType::F_subtourExchange:
				return new SubtourExchangeCrossover(parameters[0], parameters[1]);
			case ECFC::CrossoverType::F_positionBased:
				return new PositionBasedCrossover(parameters[0], parameters[1], parameters[2]);
			case ECFC::CrossoverType::F_default:
			case ECFC::CrossoverType::F_end:
			default:
				return nullptr;
			}
		}

		static size_t typeSize(CrossoverType type)
		{
			switch (type)
			{
			case ECFC::CrossoverType::F_point:
				return sizeof(PointCrossover);
			case ECFC::CrossoverType::F_SBX:
				return sizeof(SBXCrossover);
			case ECFC::CrossoverType::F_uniform:
				return sizeof(UniformCrossover);
			case ECFC::CrossoverType::F_difference:
				return sizeof(DifferenceCrossover);
			case ECFC::CrossoverType::F_no:
				return sizeof(NoCrossover);
			case ECFC::CrossoverType::F_partialmap:
				return sizeof(PartialMappedCrossover);
			case ECFC::CrossoverType::F_cycle:
				return sizeof(CycleCrossover);
			case ECFC::CrossoverType::F_order:
				return sizeof(OrderCrossover);
			case ECFC::CrossoverType::F_subtourExchange:
				return sizeof(SubtourExchangeCrossover);
			case ECFC::CrossoverType::F_positionBased:
				return sizeof(PositionBasedCrossover);
			case ECFC::CrossoverType::F_default:
			case ECFC::CrossoverType::F_end:
			default:
				return 0;
			}
		}

		static AssertList* preAssert(CrossoverType type, double* paras)
		{
			AssertList* back = new AssertList();

			switch (type)
			{
			case ECFC::CrossoverType::F_point:
				PointCrossover::preAssert(*back, paras);
				break;
			case ECFC::CrossoverType::F_SBX:
				SBXCrossover::preAssert(*back, paras);
				break;
			case ECFC::CrossoverType::F_uniform:
				UniformCrossover::preAssert(*back, paras);
				break;
			case ECFC::CrossoverType::F_difference:
				DifferenceCrossover::preAssert(*back, paras);
				break;
			case ECFC::CrossoverType::F_no:
				NoCrossover::preAssert(*back, paras);
				break;
			case ECFC::CrossoverType::F_partialmap:
				PartialMappedCrossover::preAssert(*back, paras);
				break;
			case ECFC::CrossoverType::F_cycle:
				CycleCrossover::preAssert(*back, paras);
				break;
			case ECFC::CrossoverType::F_order:
				OrderCrossover::preAssert(*back, paras);
				break;
			case ECFC::CrossoverType::F_subtourExchange:
				SubtourExchangeCrossover::preAssert(*back, paras);
				break;
			case ECFC::CrossoverType::F_positionBased:
				PositionBasedCrossover::preAssert(*back, paras);
				break;
			case ECFC::CrossoverType::F_default:
			case ECFC::CrossoverType::F_end:
			default:
				break;
			}

			return back;
		}

		static AssertList* postAssert(CrossoverType type, double* paras)
		{
			AssertList* back = new AssertList();

			switch (type)
			{
			case ECFC::CrossoverType::F_point:
				PointCrossover::postAssert(*back, paras);
				break;
			case ECFC::CrossoverType::F_SBX:
				SBXCrossover::postAssert(*back, paras);
				break;
			case ECFC::CrossoverType::F_uniform:
				UniformCrossover::postAssert(*back, paras);
				break;
			case ECFC::CrossoverType::F_difference:
				DifferenceCrossover::postAssert(*back, paras);
				break;
			case ECFC::CrossoverType::F_no:
				NoCrossover::postAssert(*back, paras);
				break;
			case ECFC::CrossoverType::F_partialmap:
				PartialMappedCrossover::postAssert(*back, paras);
				break;
			case ECFC::CrossoverType::F_cycle:
				CycleCrossover::postAssert(*back, paras);
				break;
			case ECFC::CrossoverType::F_order:
				OrderCrossover::postAssert(*back, paras);
				break;
			case ECFC::CrossoverType::F_subtourExchange:
				SubtourExchangeCrossover::postAssert(*back, paras);
				break;
			case ECFC::CrossoverType::F_positionBased:
				PositionBasedCrossover::postAssert(*back, paras);
				break;
			case ECFC::CrossoverType::F_default:
			case ECFC::CrossoverType::F_end:
			default:
				break;
			}

			return back;
		}
	};
}
