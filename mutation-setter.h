//------------------------Description------------------------
// This file defines setting helper of mutation, which 
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
#include"mutation-type.h"

namespace ECFC
{
	class MutationSetter
	{
	public:
		MutationSetter()
		{

		}

		~MutationSetter()
		{

		}

		double* bit(double mutation_rate = 0.01)
		{
			double* back = new double[SUBPARA];
			memset(back, 0, SUBPARA * sizeof(double));
			back[0] = double(MutationType::F_bit);
			back[1] = mutation_rate;
			return back;
		}

		double* bitflip(double mutation_rate = 0.01)
		{
			double* back = new double[SUBPARA];
			memset(back, 0, SUBPARA * sizeof(double));
			back[0] = double(MutationType::F_bitflip);
			back[1] = mutation_rate;
			return back;
		}

		double* overturn(int times = 1, double mutation_rate = 0.01)
		{
			double* back = new double[SUBPARA];
			memset(back, 0, SUBPARA * sizeof(double));
			back[0] = double(MutationType::F_overturn);
			back[1] = times;
			back[2] = mutation_rate;
			return back;
		}

		double* exchange(int times = 1, double mutation_rate = 1)
		{
			double* back = new double[SUBPARA];
			memset(back, 0, SUBPARA * sizeof(double));
			back[0] = double(MutationType::F_exchange);
			back[1] = times;
			back[2] = mutation_rate;
			return back;
		}

		double* PM(double eta = 20, double mutation_rate = 0.01)
		{
			double* back = new double[SUBPARA];
			memset(back, 0, SUBPARA * sizeof(double));
			back[0] = double(MutationType::F_PM);
			back[1] = eta;
			back[2] = mutation_rate;
			return back;
		}

		double* gauss(double sigma, double mutation_rate = 0.01)
		{
			double* back = new double[SUBPARA];
			memset(back, 0, SUBPARA * sizeof(double));
			back[0] = double(MutationType::F_gauss);
			back[1] = sigma;
			back[2] = mutation_rate;
			return back;
		}

		double* no()
		{
			double* back = new double[SUBPARA];
			memset(back, 0, SUBPARA * sizeof(double));
			back[0] = double(MutationType::F_no);
			return back;
		}

		double* insert(int times = 1, double mutation_rate = 1)
		{
			double* back = new double[SUBPARA];
			memset(back, 0, SUBPARA * sizeof(double));
			back[0] = double(MutationType::F_insert);
			back[1] = times;
			back[2] = mutation_rate;
			return back;
		}

		double* reorder(int times = 1, double mutation_rate = 1)
		{
			double* back = new double[SUBPARA];
			memset(back, 0, SUBPARA * sizeof(double));
			back[0] = double(MutationType::F_reorder);
			back[1] = times;
			back[2] = mutation_rate;
			return back;
		}

	};
}