#pragma once
#include"ecfc-constant.h"
#include"subswarm-manager-type.h"
#include"smanager-basic.h"
#include"sconstructer-factory.h"
#include"stopology-factory.h"

namespace ECFC
{
	class SManagerFactory
	{
	public:
		static SwarmManager* produce(SubswarmManagerType type, double* parameters)
		{
			switch (type)
			{
			case ECFC::SubswarmManagerType::F_single:
				return new SingleSwarm();
			case ECFC::SubswarmManagerType::F_nointeraction:
				return new NoInteraction(SbuilderFactory::produce(SubswarmConstructerType(parameters[0]), parameters + 1),
					STopologyFactory::produce(SubswarmTopologyType(parameters[1 + PARANUM]), parameters + 2 + PARANUM));
			case ECFC::SubswarmManagerType::F_rebuild:
				return new RebuildTopology(SbuilderFactory::produce(SubswarmConstructerType(parameters[0]), parameters + 1),
					STopologyFactory::produce(SubswarmTopologyType(parameters[1 + PARANUM]), parameters + 2 + PARANUM));
			case ECFC::SubswarmManagerType::F_immigrant:
				return new ImmigrantModel(SbuilderFactory::produce(SubswarmConstructerType(parameters[0]), parameters + 1),
					STopologyFactory::produce(SubswarmTopologyType(parameters[1 + PARANUM]), parameters + 2 + PARANUM), parameters[2 + 2 * PARANUM]);
			case ECFC::SubswarmManagerType::F_default:
			case ECFC::SubswarmManagerType::F_end:
			default:
				return nullptr;
			}
		}

		static size_t typeSize(SubswarmManagerType type)
		{
			switch (type)
			{
			case ECFC::SubswarmManagerType::F_single:
				return sizeof(SingleSwarm);
			case ECFC::SubswarmManagerType::F_nointeraction:
				return sizeof(NoInteraction);
			case ECFC::SubswarmManagerType::F_rebuild:
				return sizeof(RebuildTopology);
			case ECFC::SubswarmManagerType::F_immigrant:
				return sizeof(ImmigrantModel);
			case ECFC::SubswarmManagerType::F_default:
			case ECFC::SubswarmManagerType::F_end:
			default:
				return 0;
			}
		}

		static AssertList* preAssert(SubswarmManagerType type, double* paras)
		{
			AssertList* back = new AssertList();
			switch (type)
			{
			case ECFC::SubswarmManagerType::F_single:
				SingleSwarm::preAssert(*back, paras);
				break;
			case ECFC::SubswarmManagerType::F_nointeraction:
				NoInteraction::preAssert(*back, paras);
				break;
			case ECFC::SubswarmManagerType::F_rebuild:
				RebuildTopology::preAssert(*back, paras);
				break;
			case ECFC::SubswarmManagerType::F_immigrant:
				ImmigrantModel::preAssert(*back, paras);
				break;
			case ECFC::SubswarmManagerType::F_default:
			case ECFC::SubswarmManagerType::F_end:
			default:
				break;
			}
			return back;
		}

		static AssertList* postAssert(SubswarmManagerType type, double* paras)
		{
			AssertList* back = new AssertList();
			switch (type)
			{
			case ECFC::SubswarmManagerType::F_single:
				SingleSwarm::postAssert(*back, paras);
				break;
			case ECFC::SubswarmManagerType::F_nointeraction:
				NoInteraction::postAssert(*back, paras);
				break;
			case ECFC::SubswarmManagerType::F_rebuild:
				RebuildTopology::postAssert(*back, paras);
				break;
			case ECFC::SubswarmManagerType::F_immigrant:
				ImmigrantModel::postAssert(*back, paras);
				break;
			case ECFC::SubswarmManagerType::F_default:
			case ECFC::SubswarmManagerType::F_end:
			default:
				break;
			}
			return back;
		}
	};
}
