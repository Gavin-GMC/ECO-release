#pragma once
#include"ecfc-constant.h"
#include"offspring-generator-type.h"
#include"offspring-generator.h"
#include"ltopology-factory.h"
#include"lstrategy-factory.h"
#include"repaire-factory.h"

namespace ECFC
{
	class OgeneratorFactory
	{
	public:
		static OffspringGenerator* produce(OffspringGeneratorType type, double* parameters)
		{
			switch (type)
			{
			case ECFC::OffspringGeneratorType::F_generation_nocheck:
				return new GeneratorWithNoCheckGeneration(LTopologyFactory::produce(LearningTopologyType(parameters[0]), parameters + 1),
					LStrategyFactory::produce(LearningStrategyType(parameters[1 + PARANUM]), parameters + 2 + PARANUM),
					RepaireFactory::newRepair(RepairType(parameters[2 + 2 * PARANUM])));
			case ECFC::OffspringGeneratorType::F_generation:
				return new GeneratorWithGenerationFramework(LTopologyFactory::produce(LearningTopologyType(parameters[0]), parameters + 1),
					LStrategyFactory::produce(LearningStrategyType(parameters[1 + PARANUM]), parameters + 2 + PARANUM),
					RepaireFactory::newRepair(RepairType(parameters[2 + 2 * PARANUM])));
			case ECFC::OffspringGeneratorType::F_construct_order:
				return new GeneratorWithOrderedConstruct(LTopologyFactory::produce(LearningTopologyType(parameters[0]), parameters + 1),
					LStrategyFactory::produce(LearningStrategyType(parameters[1 + PARANUM]), parameters + 2 + PARANUM),
					RepaireFactory::newRepair(RepairType(parameters[2 + 2 * PARANUM])));
			case ECFC::OffspringGeneratorType::F_construct_parallel:
				return new GeneratorWithParallelConstruct(LTopologyFactory::produce(LearningTopologyType(parameters[0]), parameters + 1),
					LStrategyFactory::produce(LearningStrategyType(parameters[1 + PARANUM]), parameters + 2 + PARANUM),
					RepaireFactory::newRepair(RepairType(parameters[2 + 2 * PARANUM])));
			case ECFC::OffspringGeneratorType::F_default:
			case ECFC::OffspringGeneratorType::F_end:
			default:
				return nullptr;
			}
		}

		static size_t typeSize(OffspringGeneratorType type)
		{
			switch (type)
			{
			case ECFC::OffspringGeneratorType::F_generation_nocheck:
				return sizeof(GeneratorWithNoCheckGeneration);
			case ECFC::OffspringGeneratorType::F_generation:
				return sizeof(GeneratorWithGenerationFramework);
			case ECFC::OffspringGeneratorType::F_construct_order:
				return sizeof(GeneratorWithOrderedConstruct);
			case ECFC::OffspringGeneratorType::F_construct_parallel:
				return sizeof(GeneratorWithParallelConstruct);
			case ECFC::OffspringGeneratorType::F_default:
			case ECFC::OffspringGeneratorType::F_end:
			default:
				return 0;
			}
		}

		static AssertList* preAssert(OffspringGeneratorType type, double* paras)
		{
			AssertList* back = new AssertList();
			switch (type)
			{
			case ECFC::OffspringGeneratorType::F_generation_nocheck:
				GeneratorWithNoCheckGeneration::preAssert(*back, paras);
				break;
			case ECFC::OffspringGeneratorType::F_generation:
				GeneratorWithGenerationFramework::preAssert(*back, paras);
				break;
			case ECFC::OffspringGeneratorType::F_construct_order:
				GeneratorWithOrderedConstruct::preAssert(*back, paras);
				break;
			case ECFC::OffspringGeneratorType::F_construct_parallel:
				GeneratorWithParallelConstruct::preAssert(*back, paras);
				break;
			case ECFC::OffspringGeneratorType::F_default:
			case ECFC::OffspringGeneratorType::F_end:
			default:
				break;
			}
			return back;
		}

		static AssertList* postAssert(OffspringGeneratorType type, double* paras)
		{
			AssertList* back = new AssertList();
			switch (type)
			{
			case ECFC::OffspringGeneratorType::F_generation_nocheck:
				GeneratorWithNoCheckGeneration::postAssert(*back, paras);
				break;
			case ECFC::OffspringGeneratorType::F_generation:
				GeneratorWithGenerationFramework::postAssert(*back, paras);
				break;
			case ECFC::OffspringGeneratorType::F_construct_order:
				GeneratorWithOrderedConstruct::postAssert(*back, paras);
				break;
			case ECFC::OffspringGeneratorType::F_construct_parallel:
				GeneratorWithParallelConstruct::postAssert(*back, paras);
				break;
			case ECFC::OffspringGeneratorType::F_default:
			case ECFC::OffspringGeneratorType::F_end:
			default:
				break;
			}
			return back;
		}
	};
}
