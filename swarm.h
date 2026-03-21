//------------------------Description------------------------
// 
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2022 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFA（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFA" and reference "未确定"
//-----------------------------------------------------------

#pragma once
#include"subswarm.h"
#include"best-archive.h"
#include"subswarm-manager.h"

namespace ECFC {
	class Swarm
	{
	private:
		//int _subswarm_number;
		SwarmManager* _subswarms;
		BestArchive* _bestholder;
	public:
		Swarm(SwarmManager* subswarms, BestArchive* archive)
		{
			//_subswarm_number = swarm_number;
			_subswarms = subswarms;
			_bestholder = archive;
		}

		~Swarm()
		{
			delete _subswarms;
			delete _bestholder;
		}

		void setProblem(ProblemHandle* problem_handle)
		{
			_subswarms->setProblem(problem_handle);
			_bestholder->setProblem(problem_handle);
		}

		void setTerminator(Terminator* terminator)
		{
			_subswarms->setTerminator(terminator);
		}

		void updateTerminator(Terminator* terminator)
		{
			_subswarms->updateTerminator(terminator);
		}

		void ini()
		{
			_subswarms->ini();
			_bestholder->clear();
		}

		void nextIteration()
		{
			_subswarms->update();
			for (int i = 0; i < _subswarms->getSwarmNumber(); i++)
			{
				(*_subswarms)[i].run();
			}
		}

		void globalOptimumCollection()
		{
			_subswarms->globalOptimumCollection(_bestholder); 
		}

		bool getBest(Solution*& best_pointer, int& best_size)
		{
			best_size = _bestholder->getBestSize();
			best_pointer = _bestholder->getBest();
			return true;
		}
	};
}
