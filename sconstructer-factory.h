#pragma once
#include"ecfc-constant.h"
#include"subswarm-constructer-type.h"
#include"sbuilder-basic.h"

namespace ECFC
{
	class SbuilderFactory
	{
	public:
		static SubswarmConstructer* produce(SubswarmConstructerType type, double* parameters)
		{
			switch (type)
			{
			case ECFC::SubswarmConstructerType::F_fix:
				return new FixSubswarm();
			case ECFC::SubswarmConstructerType::F_distance:
				return new DistanceBuilder();
			case ECFC::SubswarmConstructerType::F_fitness:
				return new FitnessBuilder();
			case ECFC::SubswarmConstructerType::F_end:
			case ECFC::SubswarmConstructerType::F_default:
			default:
				return nullptr;
			}
		}

		static size_t typeSize(SubswarmConstructerType type)
		{
			switch (type)
			{
			case ECFC::SubswarmConstructerType::F_fix:
				return sizeof(FixSubswarm);
			case ECFC::SubswarmConstructerType::F_distance:
				return sizeof(DistanceBuilder);
			case ECFC::SubswarmConstructerType::F_fitness:
				return sizeof(FitnessBuilder);
			case ECFC::SubswarmConstructerType::F_end:
			case ECFC::SubswarmConstructerType::F_default:
			default:
				return 0;
			}
		}

		static AssertList* preAssert(SubswarmConstructerType type, double* paras)
		{
			AssertList* back = new AssertList();
			switch (type)
			{
			case ECFC::SubswarmConstructerType::F_fix:
				FixSubswarm::preAssert(*back, paras);
				break;
			case ECFC::SubswarmConstructerType::F_distance:
				DistanceBuilder::preAssert(*back, paras);
				break;
			case ECFC::SubswarmConstructerType::F_fitness:
				FitnessBuilder::preAssert(*back, paras);
				break;
			case ECFC::SubswarmConstructerType::F_end:
			case ECFC::SubswarmConstructerType::F_default:
			default:
				break;
			}
			return back;
		}

		static AssertList* postAssert(SubswarmConstructerType type, double* paras)
		{
			AssertList* back = new AssertList();
			switch (type)
			{
			case ECFC::SubswarmConstructerType::F_fix:
				FixSubswarm::postAssert(*back, paras);
				break;
			case ECFC::SubswarmConstructerType::F_distance:
				DistanceBuilder::postAssert(*back, paras);
				break;
			case ECFC::SubswarmConstructerType::F_fitness:
				FitnessBuilder::postAssert(*back, paras);
				break;
			case ECFC::SubswarmConstructerType::F_end:
			case ECFC::SubswarmConstructerType::F_default:
			default:
				break;
			}
			return back;
		}
	};
}
