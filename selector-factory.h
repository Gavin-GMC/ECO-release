#pragma once
#include"ecfc-constant.h"
#include"selector-type.h"
#include"select-basic.h"

namespace ECFC
{
	class SelectorFactory
	{
	public:
		static EnvirSelect* produce(SelectorType type, double* parameters)
		{
			switch (type)
			{
			case ECFC::SelectorType::F_noupdate:
				return new NoUpdater();
			case ECFC::SelectorType::F_index:
				return new IndexUpdater(parameters[0]);
			case ECFC::SelectorType::F_rank:
				return new RankUpdater();
			case ECFC::SelectorType::F_close:
				return new CloseUpdater(parameters[0]);
			case ECFC::SelectorType::F_end:
			case ECFC::SelectorType::F_default:
			default:
				return nullptr;
			}
		}

		static size_t typeSize(SelectorType type)
		{
			switch (type)
			{
			case ECFC::SelectorType::F_noupdate:
				return sizeof(NoUpdater);
			case ECFC::SelectorType::F_index:
				return sizeof(IndexUpdater);
			case ECFC::SelectorType::F_rank:
				return sizeof(RankUpdater);
			case ECFC::SelectorType::F_close:
				return sizeof(CloseUpdater);
			case ECFC::SelectorType::F_end:
			case ECFC::SelectorType::F_default:
			default:
				return 0;
			}
		}

		static AssertList* preAssert(SelectorType type, double* paras)
		{
			AssertList* back = new AssertList();
			switch (type)
			{
			case ECFC::SelectorType::F_noupdate:
				NoUpdater::preAssert(*back, paras);
				break;
			case ECFC::SelectorType::F_index:
				IndexUpdater::preAssert(*back, paras);
				break;
			case ECFC::SelectorType::F_rank:
				RankUpdater::preAssert(*back, paras);
				break;
			case ECFC::SelectorType::F_close:
				CloseUpdater::preAssert(*back, paras);
				break;
			case ECFC::SelectorType::F_end:
			case ECFC::SelectorType::F_default:
			default:
				break;
			}
			return back;
		}

		static AssertList* postAssert(SelectorType type, double* paras)
		{
			AssertList* back = new AssertList();
			switch (type)
			{
			case ECFC::SelectorType::F_noupdate:
				NoUpdater::postAssert(*back, paras);
				break;
			case ECFC::SelectorType::F_index:
				IndexUpdater::postAssert(*back, paras);
				break;
			case ECFC::SelectorType::F_rank:
				RankUpdater::postAssert(*back, paras);
				break;
			case ECFC::SelectorType::F_close:
				CloseUpdater::postAssert(*back, paras);
				break;
			case ECFC::SelectorType::F_end:
			case ECFC::SelectorType::F_default:
			default:
				break;
			}
			return back;
		}
	};
}
