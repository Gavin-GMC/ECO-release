#pragma once
#include"ecfc-constant.h"
#include"subswarm-topology-type.h"
#include"stopology-basic.h"

namespace ECFC
{
	class STopologyFactory
	{
	public:
		static TopologyModel* produce(SubswarmTopologyType type, double* parameters)
		{
			switch (type)
			{
			case ECFC::SubswarmTopologyType::F_noconnected:
				return new NoConnectModel();
			/*
			case ECFC::SubswarmTopologyType::F_given:
			{
				// 需要一个转化
				int* size = new int[parameters[0]];
				int* number = new int[parameters[1]];
				for (int i = 0; i < parameters[0]; i++)
				{
					size[i] = parameters[2 + i];
				}
				int offset = 2 + parameters[0];
				for (int j = 0; j < parameters[1]; j++)
				{
					number[j] = parameters[offset + j];
				}
				TopologyModel* back = new GivenModel(size, number);
				delete[] size;
				delete[] number;
				return back;
			}
			*/
			case ECFC::SubswarmTopologyType::F_star:
				return new StarModel();
			case ECFC::SubswarmTopologyType::F_toroidal:
				return new ToroidalModel();
				//case ECFC::SubswarmTopologyType::F_cell:
					//return new CellModel(parameters[0]);
			case ECFC::SubswarmTopologyType::F_connected:
				return new ConnectedModel();
			case ECFC::SubswarmTopologyType::F_preswarm:
				return new PreswarmModel();
			// case ECFC::SubswarmTopologyType::F_random:
				// return new RandomModel(parameters[0]);
			case ECFC::SubswarmTopologyType::F_default:
			case ECFC::SubswarmTopologyType::F_end:
			default:
				return nullptr;
			}
		}

		static size_t typeSize(SubswarmTopologyType type)
		{
			switch (type)
			{
			case ECFC::SubswarmTopologyType::F_noconnected:
				return sizeof(NoConnectModel);
			// case ECFC::SubswarmTopologyType::F_given:
				// return sizeof(GivenModel);
			case ECFC::SubswarmTopologyType::F_star:
				return sizeof(StarModel);
			case ECFC::SubswarmTopologyType::F_toroidal:
				return sizeof(ToroidalModel);
			// case ECFC::SubswarmTopologyType::F_cell:
				// return sizeof(CellModel);
			case ECFC::SubswarmTopologyType::F_connected:
				return sizeof(ConnectedModel);
			case ECFC::SubswarmTopologyType::F_preswarm:
				return sizeof(PreswarmModel);
			// case ECFC::SubswarmTopologyType::F_random:
				// return sizeof(RandomModel);
			case ECFC::SubswarmTopologyType::F_default:
			case ECFC::SubswarmTopologyType::F_end:
			default:
				return 0;
			}
		}

		static AssertList* preAssert(SubswarmTopologyType type, double* paras)
		{
			AssertList* back = new AssertList();
			switch (type)
			{
			case ECFC::SubswarmTopologyType::F_noconnected:
				NoConnectModel::preAssert(*back, paras);
				break;
			// case ECFC::SubswarmTopologyType::F_given:
				// GivenModel::preAssert(*back);
				// break;
			case ECFC::SubswarmTopologyType::F_star:
				StarModel::preAssert(*back, paras);
				break;
			case ECFC::SubswarmTopologyType::F_toroidal:
				ToroidalModel::preAssert(*back, paras);
				break;
			// case ECFC::SubswarmTopologyType::F_cell:
				// CellModel::preAssert(*back);
				// break;
			case ECFC::SubswarmTopologyType::F_connected:
				ConnectedModel::preAssert(*back, paras);
				break;
			case ECFC::SubswarmTopologyType::F_preswarm:
				PreswarmModel::preAssert(*back, paras);
				break;
			// case ECFC::SubswarmTopologyType::F_random:
				// RandomModel::preAssert(*back);
				// break;
			case ECFC::SubswarmTopologyType::F_default:
			case ECFC::SubswarmTopologyType::F_end:
			default:
				break;
			}
			return back;
		}

		static AssertList* postAssert(SubswarmTopologyType type, double* paras)
		{
			AssertList* back = new AssertList();
			switch (type)
			{
			case ECFC::SubswarmTopologyType::F_noconnected:
				NoConnectModel::postAssert(*back, paras);
				break;
			// case ECFC::SubswarmTopologyType::F_given:
				// GivenModel::postAssert(*back);
				// break;
			case ECFC::SubswarmTopologyType::F_star:
				StarModel::postAssert(*back, paras);
				break;
			case ECFC::SubswarmTopologyType::F_toroidal:
				ToroidalModel::postAssert(*back, paras);
				break;
			// case ECFC::SubswarmTopologyType::F_cell:
				// CellModel::postAssert(*back);
				// break;
			case ECFC::SubswarmTopologyType::F_connected:
				ConnectedModel::postAssert(*back, paras);
				break;
			case ECFC::SubswarmTopologyType::F_preswarm:
				PreswarmModel::postAssert(*back, paras);
				break;
			// case ECFC::SubswarmTopologyType::F_random:
				// RandomModel::postAssert(*back);
				// break;
			case ECFC::SubswarmTopologyType::F_default:
			case ECFC::SubswarmTopologyType::F_end:
			default:
				break;
			}
			return back;
		}
	};
}
