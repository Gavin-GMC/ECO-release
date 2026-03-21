//------------------------Description------------------------
// This file defines the factory object of individuals.
// It can product different individual array based on parameters.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include"individual-type.h"
#include"individual.h"
#include"individual-pbest.h"
#include"individual-particle.h"
#include"individual-setparticle.h"

namespace ECFC
{
	class IndividualFactory
	{
	public:
		static Individual** newIndividualArray(IndividualType type, int length = 1)
		{
			Individual** back = new Individual* [length];
			switch (type)
			{
			case IndividualType::F_individual:
				for (int i = 0; i < length; i++)
					back[i] = new Individual();
				break;
			case IndividualType::F_pbest_individual:
				for (int i = 0; i < length; i++)
					back[i] = new PbestIndividual();
				break;
			case IndividualType::F_particle:
				for (int i = 0; i < length; i++)
					back[i] = new Particle();
				break;
			case IndividualType::F_set_particle:
				for (int i = 0; i < length; i++)
					back[i] = new SetParticle();
				break;
			case IndividualType::F_default:
			default:
				delete[] back;
				return nullptr;
			}
			return back;
		}

		static size_t typeSize(IndividualType type)
		{
			switch (type)
			{
			case IndividualType::F_individual:
				return sizeof(Individual);
			case IndividualType::F_pbest_individual:
				return sizeof(PbestIndividual);
			case IndividualType::F_particle:
				return sizeof(Particle);
			case IndividualType::F_set_particle:
				return sizeof(SetParticle);
			case IndividualType::F_default:
			default:
				return 0;
			}
		}

		static AssertList* preAssert(IndividualType type, double* paras)
		{
			AssertList* back = new AssertList();
			switch (type)
			{
			case ECFC::IndividualType::F_individual:
				Individual::preAssert(*back, paras);
				break;
			case ECFC::IndividualType::F_pbest_individual:
				PbestIndividual::preAssert(*back, paras);
				break;
			case ECFC::IndividualType::F_particle:
				Particle::preAssert(*back, paras);
				break;
			case IndividualType::F_set_particle:
				SetParticle::preAssert(*back, paras);
				break;
			case IndividualType::F_default:
			default:
				break;
			}
			return back;
		}

		static AssertList* postAssert(IndividualType type, double* paras)
		{
			AssertList* back = new AssertList();
			switch (type)
			{
			case ECFC::IndividualType::F_individual:
				Individual::postAssert(*back, paras);
				break;
			case ECFC::IndividualType::F_pbest_individual:
				PbestIndividual::postAssert(*back, paras);
				break;
			case ECFC::IndividualType::F_particle:
				Particle::postAssert(*back, paras);
				break;
			case IndividualType::F_set_particle:
				SetParticle::postAssert(*back, paras);
				break;
			case IndividualType::F_default:
			default:
				break;
			}
			return back;
		}
	};
}
