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
#include"solution.h"
#include"problem.h"
#include"swarm.h"
#include"terminator.h"
#include"logger.h"

namespace ECFC
{
	class Optimizer
	{
	private:
		std::string _tag;
		Swarm* _swarm;
		Terminator* _terminator;
		Logger* _logger;
		time_t _exe_time;
		int exe_counter;

		ProblemHandle* _handle; // 副本的存在可以使得优化器内部环境与外部隔离，使运行稳定

		void ini()
		{
			_swarm->ini();
			_terminator->reset();
			_logger->newOptimization(exe_counter);
		}

		void _setProblem()
		{
			_swarm->setProblem(_handle);
			_logger->setProblem(_handle->getName());
            exe_counter = -1;
		}

	public:
		Optimizer(Swarm* swarms, Terminator* terminate, Logger* log, std::string tag)
		{
			_tag = tag;
			_swarm = swarms;
			_terminator = terminate;
			_swarm->setTerminator(terminate);
			_logger = log;
			_exe_time = 0;
			exe_counter = -1;

			_handle = nullptr;
		}

		~Optimizer()
		{
			delete _swarm;
			delete _terminator;
			delete _logger;

			delete _handle;
		}

		std::string getTag()
		{
			return _tag;
		}

		void setProblem(ProblemHandle* problem_handle)
		{
			delete _handle;

			_handle = new ProblemHandle(*problem_handle);
			_setProblem();
		}

		void setProblem(Problem* problem)
		{
			delete _handle;

			_handle = problem->compile();
			_setProblem();
		}

		time_t getExeTime()
		{
			return _exe_time;
		}

		bool getBest(Solution*& best_pointer, int& best_size)
		{
			return _swarm->getBest(best_pointer, best_size);
		}

		void logResult()
		{
			int size = 0;
			Solution* bests = nullptr;

			// 打印版本信息
			_logger->logresult("<ver= 1." + std::to_string(_logger->full_result()) + " >");
			
			// 打印运行时间
			_logger->logresult("Exe_Time:\t" + std::to_string(_exe_time) + "\tFES:\t" + std::to_string(_terminator->getFESTimes()));
			_logger->logresult("----------------------------------------");

			getBest(bests, size);

			// 打印结果信息
			_logger->logresult("Object_Number:\t" + std::to_string(bests[0].decoder_pointer->getObjectNumber()) + "\tSolution_Number:\t" + std::to_string(size));

			for (int i = 0; i < size; i++)
			{
				_logger->logresult(std::to_string(i + 1) + ":\t" +
					bests[i].decoder_pointer->toString(bests[i].result, bests[i].fitness, _logger->full_result()));
			}
		}

		void exe(time_t seed = time(NULL))
		{
			if(seed!=EMPTYVALUE)
				srand(seed);

			exe_counter++;
			_exe_time = time(NULL);
			ini();
			while (!_terminator->termination())
			{
				_swarm->nextIteration();
				_swarm->updateTerminator(_terminator);
			}
			_swarm->globalOptimumCollection();
			_exe_time = time(NULL) - _exe_time;
		}

		void exe(int n, time_t seed)
		{
			srand(seed);

			for (int i = 0; i < n; i++)
			{
				exe(EMPTYVALUE);
				logResult();
			}
		}
	};
}
