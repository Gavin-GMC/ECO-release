//------------------------Description------------------------
// 
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2022 所有人名称（待确认）, All Rights Reserved.
// You are free to use the myEC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "myEC" and reference "未确定"
//-----------------------------------------------------------

#pragma once
#include"subswarm.h"
#include"subswarm-constructer.h"
#include"subswarm-topology.h"

namespace ECFC
{
	// 种群管理包含三个关键环节：子种群的构建方式（基于索引、基于解距离、基于适应度值）、
	// 子种群的互动方式（最优个体迁移、子种群重建、协同演化、不互动）、
	// 子种群的互动拓扑图（全连接、环形、星形，无连接等）
	// 所以应该是，三个子类、各负责一个环节
	// 其中，互动可能需要知道构建和互动拓扑，另两个则可以独立完成，所以可以把互动作为主类，另两个作为互动类的参数
	// 

	/// <summary>
	/// 协同模型
	/// </summary>
	class SwarmManager
	{
	protected:
		SubswarmConstructer* _builder;
		TopologyModel* _model;
		Subswarm** _subswarms;
		Terminator* _terminator_pointer;
		Terminator** _subterminator_pointers;
		int _swarm_number;

		void clearSwarm()
		{

		}

	public:
		SwarmManager(SubswarmConstructer* builder, TopologyModel* model)
		{
			_builder = builder;
			_model = model;
			_swarm_number = 0;
			_subswarms = nullptr;
			_terminator_pointer = nullptr;
			_subterminator_pointers = nullptr;
		}

		virtual ~SwarmManager()
		{
			delete _builder;
			delete _model;
			for (int i = 0; i < _swarm_number; i++)
			{
				delete _subswarms[i];
			}
			delete[] _subswarms;
			delete[] _subterminator_pointers;
		}

		void setSwarmNumber(int number)
		{
			_swarm_number = number;
			_subswarms = new Subswarm * [number];
			_subterminator_pointers = new Terminator* [number];
		}

		void setTerminator(Terminator* terminator)
		{
			_terminator_pointer = terminator;
		}

		void updateTerminator(Terminator* terminator)
		{
			for (int i = 0; i < _swarm_number; i++)
			{
				terminator->merge(_subterminator_pointers[i]);
			}
		}

		virtual void globalOptimumCollection(BestArchive* global_archive)
		{
			Solution* bests;
			int size = 0;
			for (int i = 0; i < _swarm_number; i++)
			{
				_subswarms[i]->getBest(bests, size);
				for (int j = 0; j < size; j++)
				{
					global_archive->updateBest(bests[j]);
				}
			}
		}

		void setSwarm(Subswarm* swarm, int swarmid)
		{
			_subswarms[swarmid] = swarm;
			_subterminator_pointers[swarmid] = swarm->getTerminator();
		}

		void setSwarms(Subswarm* subswarms, int swarm_number)
		{
			_swarm_number = swarm_number;

			clearSwarm();

			_subswarms = new Subswarm * [swarm_number];
			for (int i = 0; i < swarm_number; i++)
			{
				_subswarms[i] = subswarms + i;
				_subterminator_pointers[i] = subswarms[i].getTerminator();
			}
		}

		int getSwarmNumber()
		{
			return _swarm_number;
		}

		Subswarm& operator[](const int index)
		{
			return *_subswarms[index];
		}

		virtual void setProblem(ProblemHandle* problem_handle)
		{
			for (int i = 0; i < _swarm_number; i++)
			{
				_subswarms[i]->setProblem(problem_handle);
			}
		}

		virtual void ini()
		{
			for (int i = 0; i < _swarm_number; i++)
			{
				_subswarms[i]->resetTerminator();
				_subswarms[i]->ini();
			}

			_builder->ini(_subswarms, _swarm_number);
			_model->ini(_subswarms, _swarm_number);

			for (int i = 0; i < _swarm_number; i++)
			{
				_subswarms[i]->setNeibors(_model->neighborhoods[i].getNSet(), _model->neighborhoods[i].getSize());
			}
		}

		virtual void update() = 0;
	};
}
