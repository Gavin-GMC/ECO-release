//------------------------Description------------------------
// Optimizer:顶层优化器。持 Population(顶层聚合)+ Terminator + Logger + ProblemHandle 副本。
//   exe(seed) 播种跑一次;exe(n,seed) 播种一次跑 n 次并逐次记结果;continue_exe 换终止器续跑。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <string>
#include <ctime>
#include <exception>
#include "solution.h"
#include "population.h"
#include "terminator.h"
#include "problem-handle.h"
#include "problem.h"
#include "ecflow-rand.h"
#include "logger.hpp"

namespace ECFlow
{
    class Optimizer
    {
    private:
        std::string    _tag;
        Population*     _population;
        Terminator*    _terminator;
        Logger*        _logger;
        time_t         _exe_time;
        int            exe_counter;

        ProblemHandle* _handle;   // 内部持副本:优化器内可用问题、与外部输入解耦,更稳

        void ini()
        {
            // ★ 次序要紧:**先 reset 再 ini**。`_population->ini()` 现在要**读全局剩余**给初始种群设限
            //   (见 SubpopulationManager::ini 的 cappedBy);若仍是原来的 `ini(); reset();`,exe(n) 的第 2 次运行
            //   会读到上一次的残账 → 剩余算作 0 → **一个个体都不初始化**。
            //   注:改动前二者次序确实无关紧要(ini 的 FES 计在**子群**终止器上,而 reset 清的是**全局**,
            //   两个不同对象,抹不着;曾据"计了又被抹"的错误模型改过次序,实测数字纹丝不动)——
            //   是本轮"初始种群按全局剩余设限"的引入,才使这个次序变成承重的。
            _terminator->reset();
            _population->ini();
            _logger->newOptimization(exe_counter);
        }

        void _setProblem()
        {
            _population->setProblem(_handle);
            _logger->setProblem(_handle->getName());
            exe_counter = -1;
        }

        // 单次运行(不重置随机种子;供 exe/exe(n) 复用)。LOG-DETAIL:info 生命周期 + 异常落盘(记录后 re-throw,不吞)
        void _run()
        {
            exe_counter++;
            _exe_time = time(NULL);
            _logger->info("run " + std::to_string(exe_counter) + " started");
            try
            {
                ini();
                while (!_terminator->termination())
                    _population->nextIteration();   // = manager.runEpoch():配额/并账由框架包办
                _population->globalOptimumCollection();
            }
            catch (const std::exception& ex)
            {
                _exe_time = time(NULL) - _exe_time;
                std::string msg = "run " + std::to_string(exe_counter) + " FAILED after " + std::to_string(_exe_time)
                    + "s (FES=" + std::to_string(_terminator->getFESTimes()) + "): " + ex.what();
                _logger->error(msg);                     // per-optimizer .err/.log(std::endl 已 flush)
                sys_logger.error("[" + _tag + "] " + msg); // 全局系统 .err
                throw;                                    // 记录后重抛,不吞异常
            }
            _exe_time = time(NULL) - _exe_time;

            Solution* best = nullptr; int bs = 0;
            getBest(best, bs);
            _logger->info("run " + std::to_string(exe_counter) + " done: exe_time=" + std::to_string(_exe_time)
                + "s FES=" + std::to_string(_terminator->getFESTimes())
                + " best=" + ((best && bs > 0) ? std::to_string(best->fitness[0]) : std::string("n/a")));
        }

    public:
        Optimizer(Population* population, Terminator* terminate, Logger* log, std::string tag)
        {
            _tag = tag;
            _population = population;
            _terminator = terminate;
            _population->setTerminator(terminate);
            _logger = log;
            _exe_time = 0;
            exe_counter = -1;
            _handle = nullptr;
        }

        ~Optimizer()
        {
            delete _population;
            delete _terminator;
            delete _logger;
            delete _handle;
        }

        std::string getTag() { return _tag; }

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

        time_t getExeTime() { return _exe_time; }

        bool getBest(Solution*& best_pointer, int& best_size)
        {
            return _population->getBest(best_pointer, best_size);
        }

        Subpopulation* getSubswarm(std::string id)
        {
            return _population->getSubswarm(id);
        }

        void logResult()
        {
            int size = 0;
            Solution* bests = nullptr;

            _logger->logresult("<ver= 1." + std::to_string(_logger->full_result()) + " >");
            _logger->logresult("Exe_Time:\t" + std::to_string(_exe_time) + "\tFES:\t" + std::to_string(_terminator->getFESTimes()));
            _logger->logresult("----------------------------------------");

            getBest(bests, size);

            _logger->logresult("Object_Number:\t" + std::to_string(bests[0].decoder_pointer->getObjectNumber()) + "\tSolution_Number:\t" + std::to_string(size));

            for (int i = 0; i < size; i++)
                _logger->logresult(std::to_string(i + 1) + ":\t" +
                    bests[i].decoder_pointer->toString(bests[i].result, bests[i].fitness, _logger->full_result()));
        }

        // 单次运行:播种一次后跑(修 Seed-RNG:srand→set_seed)
        void exe(time_t seed = time(NULL))
        {
            set_seed((unsigned int)seed);
            _run();
        }

        // n 次运行:仅播种一次,逐次记结果(修 Seed-EMPTY:去 EMPTYVALUE(NaN) 塞 time_t 的坏路径)
        void exe(int n, time_t seed)
        {
            set_seed((unsigned int)seed);
            for (int i = 0; i < n; i++)
            {
                _run();
                logResult();
            }
        }

        // 换终止器续跑(不重新初始化种群,接着当前状态)
        void continue_exe(Terminator* new_terminator)
        {
            delete _terminator;
            _terminator = new_terminator;
            _population->setTerminator(_terminator);   // 迁移补:让 Population/manager 指向新终止器(原缺,SingleSwarm.update 会用旧指针)

            _exe_time = time(NULL);
            while (!_terminator->termination())
                _population->nextIteration();
            _population->globalOptimumCollection();
            _exe_time = time(NULL) - _exe_time;
        }
    };
}
