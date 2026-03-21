#pragma once
#include"mutation-type.h"
#include"ga-mutation.h"
#include"assert.h"

namespace ECFC
{
	class MutationFactory
	{
	public:
		static Mutation* produce(MutationType type, double* parameters)
		{
			switch (type)
			{
			case ECFC::MutationType::F_bit:
				return new BitMutation(parameters[0]);
			case ECFC::MutationType::F_bitflip:
				return new BitFlipMutation(parameters[0]);
			case ECFC::MutationType::F_overturn:
				return new OverturnMutation(parameters[0], parameters[1]);
			case ECFC::MutationType::F_exchange:
				return new ExchangeMutation(parameters[0], parameters[1]);
			case ECFC::MutationType::F_PM:
				return new PM_Mutation(parameters[0], parameters[1]);
			case ECFC::MutationType::F_gauss:
				return new GaussMutation(parameters[0], parameters[1]);
			case ECFC::MutationType::F_no:
				return new NoMutation();
			case ECFC::MutationType::F_insert:
				return new InsertMutation(parameters[0], parameters[1]);
			case ECFC::MutationType::F_reorder:
				return new ReorderMutation(parameters[0], parameters[1]);
			case ECFC::MutationType::F_end:
			case ECFC::MutationType::F_default:
			default:
				return nullptr;
			}
		}

		static size_t typeSize(MutationType type)
		{
			switch (type)
			{
			case ECFC::MutationType::F_bit:
				return sizeof(BitMutation);
			case ECFC::MutationType::F_bitflip:
				return sizeof(BitFlipMutation);
			case ECFC::MutationType::F_overturn:
				return sizeof(OverturnMutation);
			case ECFC::MutationType::F_exchange:
				return sizeof(ExchangeMutation);
			case ECFC::MutationType::F_PM:
				return sizeof(PM_Mutation);
			case ECFC::MutationType::F_gauss:
				return sizeof(GaussMutation);
			case ECFC::MutationType::F_no:
				return sizeof(NoMutation); 
			case ECFC::MutationType::F_insert:
				return sizeof(InsertMutation); 
			case ECFC::MutationType::F_reorder:
				return sizeof(ReorderMutation);
			case ECFC::MutationType::F_end:
			case ECFC::MutationType::F_default:
			default:
				return 0;
			}
		}

		static AssertList* preAssert(MutationType type, double* paras)
		{
			AssertList* back = new AssertList();

			switch (type)
			{
			case ECFC::MutationType::F_bit:
				BitMutation::preAssert(*back, paras);
				break;
			case ECFC::MutationType::F_bitflip:
				BitFlipMutation::preAssert(*back, paras);
				break;
			case ECFC::MutationType::F_overturn:
				OverturnMutation::preAssert(*back, paras);
				break;
			case ECFC::MutationType::F_exchange:
				ExchangeMutation::preAssert(*back, paras);
				break;
			case ECFC::MutationType::F_PM:
				PM_Mutation::preAssert(*back, paras);
				break;
			case ECFC::MutationType::F_gauss:
				GaussMutation::preAssert(*back, paras);
				break;
			case ECFC::MutationType::F_no:
				NoMutation::preAssert(*back, paras);
				break;
			case ECFC::MutationType::F_insert:
				InsertMutation::preAssert(*back, paras);
				break;
			case ECFC::MutationType::F_reorder:
				ReorderMutation::preAssert(*back, paras);
				break;
			case ECFC::MutationType::F_end:
			case ECFC::MutationType::F_default:
			default:
				break;
			}

			return back;
		}

		static AssertList* postAssert(MutationType type, double* paras)
		{
			AssertList* back = new AssertList();

			switch (type)
			{
			case ECFC::MutationType::F_bit:
				BitMutation::postAssert(*back, paras);
				break;
			case ECFC::MutationType::F_bitflip:
				BitFlipMutation::postAssert(*back, paras);
				break;
			case ECFC::MutationType::F_overturn:
				OverturnMutation::postAssert(*back, paras);
				break;
			case ECFC::MutationType::F_exchange:
				ExchangeMutation::postAssert(*back, paras);
				break;
			case ECFC::MutationType::F_PM:
				PM_Mutation::postAssert(*back, paras);
				break;
			case ECFC::MutationType::F_gauss:
				GaussMutation::postAssert(*back, paras);
				break;
			case ECFC::MutationType::F_no:
				NoMutation::postAssert(*back, paras);
				break;
			case ECFC::MutationType::F_insert:
				InsertMutation::postAssert(*back, paras);
				break;
			case ECFC::MutationType::F_reorder:
				ReorderMutation::postAssert(*back, paras);
				break;
			case ECFC::MutationType::F_end:
			case ECFC::MutationType::F_default:
			default:
				break;
			}

			return back;
		}
	};
}
