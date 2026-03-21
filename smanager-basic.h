//------------------------Description------------------------
// This file defines some optional swarm manager components,
// which are basic and commonly used.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include"assert.h"
#include"subswarm-manager.h"
#include"sbuilder-basic.h"
#include"stopology-basic.h"

namespace ECFC
{
	class NoInteraction final : public SwarmManager
	{
	public:
		NoInteraction(SubswarmConstructer* builder, TopologyModel* model)
			: SwarmManager(builder, model)
		{

		}

		~NoInteraction()
		{

		}

		static void preAssert(AssertList& list, double* paras)
		{
			// none
		}

		static void postAssert(AssertList& list, double* paras)
		{
			// none
		}

		void update() 
		{

		}
	};

	class RebuildTopology final : public SwarmManager
	{
	public:
		RebuildTopology(SubswarmConstructer* builder, TopologyModel* model)
			: SwarmManager(builder, model)
		{
			_builder = builder;
			_model = model;
		}

		~RebuildTopology()
		{

		}

		static void preAssert(AssertList& list, double* paras)
		{
			// none
		}

		static void postAssert(AssertList& list, double* paras)
		{
			// none
		}

		virtual void update()
		{
			_builder->build(_subswarms, _swarm_number);
			_model->build(_subswarms, _swarm_number);
		}
	};

	class ImmigrantModel final : public SwarmManager
	{
	private:
		bool _random_replace;
		double _immigrant_rate;
		int _immigrant_number;

	public:
		ImmigrantModel(SubswarmConstructer* builder, TopologyModel* model, bool random_replace, double immigrant_rate = EMPTYVALUE, int immigrant_number = 1)
			: SwarmManager(builder, model)
		{
			_random_replace = random_replace;
			_immigrant_rate = immigrant_rate;
			_immigrant_number = immigrant_number;
		}

		virtual ~ImmigrantModel()
		{

		}

		static void preAssert(AssertList& list, double* paras)
		{
			// none
		}

		static void postAssert(AssertList& list, double* paras)
		{
			// none
		}

		virtual void update()
		{
			Solution* optimal;
			int optimal_size = 0;

			if (_random_replace)
			{
				for (int i = 0; i < _swarm_number; i++)
				{
					_subswarms[i]->getBest(optimal, optimal_size);
					for (int j = 0; j < _model->neighborhoods[i].getSize(); j++)
					{
						_model->neighborhoods[i][j]->replaceIndividualR(optimal + (rand() % optimal_size));
					}
				}
			}
			else
			{
				for (int i = 0; i < _swarm_number; i++)
				{
					_subswarms[i]->getBest(optimal, optimal_size);
					for (int j = 0; j < _model->neighborhoods[i].getSize(); j++)
					{
						_model->neighborhoods[i][j]->replaceIndividualW(optimal + (rand() % optimal_size));
					}
				}
			}

		}
	};

	// 因为过于常用，作为不互动拓扑的一种特例提供
	class SingleSwarm final : public SwarmManager
	{
	public:
		SingleSwarm()
			: SwarmManager(nullptr, nullptr)
		{
			_builder = new FixSubswarm();
			_model = new NoConnectModel();
		}

		~SingleSwarm()
		{

		}

		static void preAssert(AssertList& list, double* paras)
		{
			// none
		}

		static void postAssert(AssertList& list, double* paras)
		{
			// none
		}

		void update()
		{
			// 更新种群结构
			
			// 种群交互

			// 更新终止条件
			_terminator_pointer->merge(_subswarms[0]->getTerminator());
			_subswarms[0]->resetTerminator();
		}

	};
}
