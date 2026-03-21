//------------------------Description------------------------
// This file defines setting helper of crossover, which 
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
#include"crossover-type.h"

namespace ECFC
{
	class CrossoverSetter
	{
	public:
		CrossoverSetter()
		{

		}

		~CrossoverSetter()
		{

		}

		double* point(int point_number = 1, double cross_rate = 0.9, bool coupled = true)
		{
			double* back = new double[SUBPARA];
			memset(back, 0, SUBPARA * sizeof(double));
			back[0] = double(CrossoverType::F_point);
			back[1] = point_number;
			back[2] = cross_rate;
			back[3] = coupled;
			return back;
		}

		double* SBX(double mu =1, double cross_rate = 0.9, bool coupled = true)
		{
			double* back = new double[SUBPARA];
			memset(back, 0, SUBPARA * sizeof(double));
			back[0] = double(CrossoverType::F_SBX);
			back[1] = mu;
			back[2] = cross_rate;
			back[3] = coupled;
			return back;
		}

		double* uniform(double cross_rate = 0.9, bool coupled = true)
		{
			double* back = new double[SUBPARA];
			memset(back, 0, SUBPARA * sizeof(double));
			back[0] = double(CrossoverType::F_uniform);
			back[1] = cross_rate;
			back[2] = coupled;
			return back;
		}

		double* difference(double cross_rate = 0.9, double factor = 0.5, bool coupled = true)
		{
			double* back = new double[SUBPARA];
			memset(back, 0, SUBPARA * sizeof(double));
			back[0] = double(CrossoverType::F_difference);
			back[1] = cross_rate;
			back[2] = factor;
			back[3] = coupled;
			return back;
		}

		double* no()
		{
			double* back = new double[SUBPARA];
			memset(back, 0, SUBPARA * sizeof(double));
			back[0] = double(CrossoverType::F_no);
			return back;
		}

		double* partialMap(double cross_rate = 0.9, bool coupled = true)
		{
			double* back = new double[SUBPARA];
			memset(back, 0, SUBPARA * sizeof(double));
			back[0] = double(CrossoverType::F_partialmap);
			back[1] = cross_rate;
			back[2] = coupled;
			return back;
		}

		double* cycle(double cross_rate = 0.9, bool coupled = true)
		{
			double* back = new double[SUBPARA];
			memset(back, 0, SUBPARA * sizeof(double));
			back[0] = double(CrossoverType::F_cycle);
			back[1] = cross_rate;
			back[2] = coupled;
			return back;
		}

		double* order(double cross_rate = 0.9, bool coupled = true)
		{
			double* back = new double[SUBPARA];
			memset(back, 0, SUBPARA * sizeof(double));
			back[0] = double(CrossoverType::F_order);
			back[1] = cross_rate;
			back[2] = coupled;
			return back;
		}

		double* subtourExchange(double cross_rate = 0.9, bool coupled = true)
		{
			double* back = new double[SUBPARA];
			memset(back, 0, SUBPARA * sizeof(double));
			back[0] = double(CrossoverType::F_subtourExchange);
			back[1] = cross_rate;
			back[2] = coupled;
			return back;
		}

		double* positionBased(double cross_rate = 0.9, double select_gene_proportion = 0.1, bool coupled = true)
		{
			double* back = new double[SUBPARA];
			memset(back, 0, SUBPARA * sizeof(double));
			back[0] = double(CrossoverType::F_positionBased);
			back[1] = cross_rate;
			back[2] = select_gene_proportion;
			back[3] = coupled;
			return back;
		}
	};
}