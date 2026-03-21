//------------------------Description------------------------
// This file is consisted of some commonly used basic distribution models,
// support for sample-based model building, updating, and value generation.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include<cmath>
#include"basicfunc.h"

namespace ECFC
{
	// 分布模型的抽象模板类
	class DistributionModel
	{
	public:
		DistributionModel() {}

		~DistributionModel() {}

		// 清除分布和样本信息
		virtual void clear() = 0;

		// 为分布添加样本
		virtual void addSample(double sample) = 0;

		// 基于已有样本构建模型
		virtual void build() = 0;

		// 基于给出的样本集对模型进行构建（初始化）
		virtual void ini(double* sample_set, size_t size)
		{
			clear();
			for (int i = 0; i < size; i++)
			{
				addSample(sample_set[i]);
			}
			build();
		}

		// 获得基于分布模型的随机值
		virtual double getValue() = 0;

		// 直接基于参数设置模型（参数含义随模型不同而改变）
		virtual void setModel(double* paras) = 0;
	};

	// 高斯分布模型
	class DM_Gaussian final : public DistributionModel
	{
	private:
		double _mean;
		double _stdv;
		double _difference;
		int _sample_number;

		double guassion(double mean = 0, double stdv = 1)
		{
			return mean + stdv * (
				sqrt((-2) * log(ECFC::rand01_()))
				* sin(2 * 3.1415926 * ECFC::rand01()));
		}
	public:
		DM_Gaussian()
			:DistributionModel()
		{
			clear();
		}

		void clear()
		{
			_mean = 0;
			_stdv = 0;
			_difference = 0;
			_sample_number = 0;
		}

		void addSample(double sample)
		{
			// 基于Welford法实现元素迭代添加
			_sample_number++;
			double new_mean = _mean + (sample - _mean) / _sample_number;
			_difference = _difference + (sample - _mean) * (sample - new_mean);
			_mean = new_mean;
		}

		void build()
		{
			if (_sample_number == 1)
				return;
			_stdv = sqrt(_difference / (_sample_number - 1));
		}

		double getValue()
		{
			return guassion(_mean, _stdv);
		}

		void setModel(double* paras)
		{
			_mean = paras[0];
			_stdv = paras[1];
			_sample_number = int(paras[2]);
			_difference = pow(_stdv, 2) * (_sample_number - 1);
		}
	};
}