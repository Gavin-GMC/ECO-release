//------------------------Description------------------------
// This file defines setting helper of learning strategy, which 
// provides more clearly setting interface.
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
#include"configurelist.h"
#include"learning-strategy-type.h"
#include"mutation-setter.h"
#include"crossover-setter.h"

namespace ECFC
{
	class LStrategySetter
	{
	private:
		ConfigureList::PopulationConfig* _list_pointer;
	public:
		MutationSetter mutation;
		CrossoverSetter crossover;

		LStrategySetter()
		{
			_list_pointer = nullptr;
		}

		LStrategySetter(ConfigureList::PopulationConfig* clist)
		{
			_list_pointer = clist;
		}

		~LStrategySetter()
		{

		}

		void GA(double* crossover_para, double* mutation_para)
		{
			_list_pointer->lstrategy_type = LearningStrategyType::F_GA;
			_list_pointer->l_para[1 + PARANUM] = double(LearningStrategyType::F_GA);

			memcpy(_list_pointer->lstrategy_para, crossover_para, sizeof(double) * SUBPARA);
			memcpy(_list_pointer->lstrategy_para + SUBPARA, mutation_para, sizeof(double) * SUBPARA);

			delete[] crossover_para;
			delete[] mutation_para;
		}

		void DE(double* crossover4mutation, double cross_rate = 0.5)
		{
			_list_pointer->lstrategy_type = LearningStrategyType::F_DE;
			_list_pointer->l_para[1 + PARANUM] = double(LearningStrategyType::F_DE);
			_list_pointer->lstrategy_para[0] = cross_rate;
			memcpy(_list_pointer->lstrategy_para + 1, crossover4mutation, sizeof(double) * SUBPARA);
			delete[] crossover4mutation;
		}

		void EDA(DistributionType model = DistributionType::F_Gaussian, int good_number = EMPTYVALUE, double good_rate = 0.5)
		{
			_list_pointer->lstrategy_type = LearningStrategyType::F_EDA;
			_list_pointer->l_para[1 + PARANUM] = double(LearningStrategyType::F_EDA);
			_list_pointer->lstrategy_para[0] = double(model);
			_list_pointer->lstrategy_para[1] = good_number;
			_list_pointer->lstrategy_para[2] = good_rate;
		}

		void PSO(int object_number = 2, double c = 2, double w_ini = 0.9, double w_attenuation = EMPTYVALUE)
		{
			_list_pointer->lstrategy_type = LearningStrategyType::F_PSO;
			_list_pointer->l_para[1 + PARANUM] = double(LearningStrategyType::F_PSO);
			_list_pointer->lstrategy_para[0] = object_number;
			_list_pointer->lstrategy_para[1] = w_ini;
			_list_pointer->lstrategy_para[2] = w_attenuation;
			for (int i = 0; i < object_number; i++)
			{
				_list_pointer->lstrategy_para[3 + i] = c;
			}
		}

		void PSO(int object_number, double* c, double w_ini = 0.9, double w_attenuation = EMPTYVALUE)
		{
			_list_pointer->lstrategy_type = LearningStrategyType::F_PSO;
			_list_pointer->l_para[1 + PARANUM] = double(LearningStrategyType::F_PSO);
			_list_pointer->lstrategy_para[0] = object_number;
			_list_pointer->lstrategy_para[1] = w_ini;
			_list_pointer->lstrategy_para[2] = w_attenuation;
			for (int i = 0; i < object_number; i++)
			{
				_list_pointer->lstrategy_para[3 + i] = c[i];
			}
		}

		void BPSO(double c1 = 2, double c2 = 2, double w_ini = 0.9, double w_attenuation = EMPTYVALUE)
		{
			_list_pointer->lstrategy_type = LearningStrategyType::F_EDA;
			_list_pointer->l_para[1 + PARANUM] = double(LearningStrategyType::F_EDA);
			_list_pointer->lstrategy_para[0] = c1;
			_list_pointer->lstrategy_para[1] = c2;
			_list_pointer->lstrategy_para[2] = w_ini;
			_list_pointer->lstrategy_para[3] = w_attenuation;
		}

		void AS(double alpha = 1, double belta = 2, double rho = 0.5, double tao_ini = EMPTYVALUE)
		{
			_list_pointer->lstrategy_type = LearningStrategyType::F_AS;
			_list_pointer->l_para[1 + PARANUM] = double(LearningStrategyType::F_AS);
			_list_pointer->lstrategy_para[0] = alpha;
			_list_pointer->lstrategy_para[1] = belta;
			_list_pointer->lstrategy_para[2] = rho;
			_list_pointer->lstrategy_para[3] = tao_ini;
		}

		void ACS(double alpha = 1, double belta = 2, double rho_g = 0.1, double rho_l = 0.1, double q0 = 0.9, double tao_ini = EMPTYVALUE)
		{
			_list_pointer->lstrategy_type = LearningStrategyType::F_ACS;
			_list_pointer->l_para[1 + PARANUM] = double(LearningStrategyType::F_ACS);
			_list_pointer->lstrategy_para[0] = alpha;
			_list_pointer->lstrategy_para[1] = belta;
			_list_pointer->lstrategy_para[2] = rho_g;
			_list_pointer->lstrategy_para[3] = rho_l;
			_list_pointer->lstrategy_para[4] = q0;
			_list_pointer->lstrategy_para[5] = tao_ini;
		}

		void SetPSO(int object_number = 2, double c = 2, double w_ini = 0.9, bool v_heuristic = true, bool f_heuristic = true, double w_attenuation = EMPTYVALUE)
		{
			_list_pointer->lstrategy_type = LearningStrategyType::F_SetPSO;
			_list_pointer->l_para[1 + PARANUM] = double(LearningStrategyType::F_SetPSO);
			_list_pointer->lstrategy_para[0] = object_number;
			_list_pointer->lstrategy_para[1] = w_ini;
			_list_pointer->lstrategy_para[2] = v_heuristic;
			_list_pointer->lstrategy_para[3] = f_heuristic;
			_list_pointer->lstrategy_para[4] = w_attenuation;
			for (int i = 0; i < object_number; i++)
			{
				_list_pointer->lstrategy_para[5 + i] = c;
			}
		}

		void SetPSO(int object_number, double* c, double w_ini = 0.9, bool v_heuristic = true, bool f_heuristic = true, double w_attenuation = EMPTYVALUE)
		{
			_list_pointer->lstrategy_type = LearningStrategyType::F_SetPSO;
			_list_pointer->l_para[1 + PARANUM] = double(LearningStrategyType::F_SetPSO);
			_list_pointer->lstrategy_para[0] = object_number;
			_list_pointer->lstrategy_para[1] = w_ini;
			_list_pointer->lstrategy_para[2] = v_heuristic;
			_list_pointer->lstrategy_para[3] = f_heuristic;
			_list_pointer->lstrategy_para[4] = w_attenuation;
			for (int i = 0; i < object_number; i++)
			{
				_list_pointer->lstrategy_para[5 + i] = c[i];
			}
		}

	};
}