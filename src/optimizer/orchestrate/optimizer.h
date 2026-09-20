//------------------------Description------------------------
// Optimizer:顶层优化器。持 Population(顶层聚合)+ Terminator + Logger + ProblemHandle 副本。
//   exe(seed) 播种跑一次;exe(n,seed) 播种一次跑 n 次并逐次记结果;continue_exe 换终止器续跑。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <functional>
#include <string>
#include <ctime>
#include <exception>
#include <stdexcept>
#include "solution.h"
#include "population.h"
#include "terminator.h"
#include "problem-handle.h"
#include "problem.h"
#include "evoflow-rand.h"
#include "logger.hpp"

namespace EvoFlow
{
    class OptimizationCancelled : public std::runtime_error
    {
    public:
        OptimizationCancelled() : std::runtime_error("optimization cancelled") {}
    };

    class Optimizer
    {
    private:
        std::string    _tag;
        Population*     _population;
        Terminator*    _terminator;
        Logger*        _logger;
        time_t         _exe_time;
        int            exe_counter;
        int            _requested_run_start;
        bool           _replace_existing;

        ProblemHandle* _handle;   // 内部持副本:优化器内可用问题、与外部输入解耦,更稳

        void ini()
        {
            // ★ 次序要紧(v1.4.8):**先 reset 再 ini**。`_population->ini()` 现在要**读全局剩余**给初始种群设限
            //   (见 SubpopulationManager::ini 的 cappedBy);若仍是原来的 `ini(); reset();`,exe(n) 的第 2 次运行
            //   会读到上一次的残账 → 剩余算作 0 → **一个个体都不初始化**。
            //   注:改动前二者次序确实无关紧要(ini 的 FES 计在**子群**终止器上,而 reset 清的是**全局**,
            //   两个不同对象,抹不着;曾据"计了又被抹"的错误模型改过次序,实测数字纹丝不动)——
            //   是本轮"初始种群按全局剩余设限"的引入,才使这个次序变成承重的。
            _terminator->reset();
            _population->ini();

            // v2.5.3 LOG-PRCS-MULTIPOP:主 Logger 的 .prcs = 【全局级】(逐 epoch,全局 best+统一 FES),
            //   **所有配置一律产出**(去 single 特殊化,格式统一);子群级各子群自持另写。.rslt 照开。
            const std::string gextra = "\tLevel:\tglobal\tSwarmNumber:\t" + std::to_string(_population->getSwarmNumber());
            _logger->newOptimization(exe_counter, gextra);
            _population->newProcessLog(exe_counter);   // 各子群开自己的子群级 .prcs
        }

        // v2.5.3:写一条【全局级】过程记录(逐 epoch)——复用 9-token #rec 格式(swarm=global,npop=0),
        //   使既有 PrcsReader 原样可解析;收敛曲线读的正是这条(经统一 FES,单调)。
        void _logGlobalProcess()
        {
            _population->globalOptimumCollection();   // 刷新全局 best 到全局档案
            Solution* best = nullptr; int bs = 0;
            getBest(best, bs);
            _logger->logProcessMeta("#rec\tfes\t" + std::to_string(_terminator->getFESTimes())
                + "\tswarm\tglobal\tnbest\t" + std::to_string(bs) + "\tnpop\t0");
            const bool full = _logger->full_process_print();
            for (int i = 0; i < bs; i++)
                _logger->logprocess(std::to_string(i + 1) + ":\t" +
                    best[i].decoder_pointer->toString(best[i].result, best[i].fitness, full));
        }

        void _setProblem()
        {
            _population->setProblem(_handle);
            _logger->setProblem(_handle->getName());
            exe_counter = -1;
        }

        // 单次运行(不重置随机种子;供 exe/exe(n) 复用)。LOG-DETAIL:info 生命周期 + 异常落盘(记录后 re-throw,不吞)
        void _run(const std::function<bool()>& cancelled = std::function<bool()>())
        {
            exe_counter++;
            _exe_time = time(NULL);
            _logger->info("run " + std::to_string(exe_counter) + " started");
            try
            {
                if (cancelled && cancelled()) throw OptimizationCancelled();
                ini();
                // v2.5.3:全局级过程日志 —— 开进程记录时每 epoch 写一条(所有配置一律,不再按管理器分支)。
                const bool glog = _logger->process_print();
                while (!_terminator->termination())
                {
                    _population->nextIteration();   // = manager.runEpoch():配额/并账由框架包办(v1.4.8 删 updateTerminator 兜底并账)
                    if (glog) _logGlobalProcess();
                }
                if (cancelled && cancelled()) throw OptimizationCancelled();
                _population->globalOptimumCollection();
            }
            catch (const OptimizationCancelled&)
            {
                _exe_time = time(NULL) - _exe_time;
                _logger->info("run " + std::to_string(exe_counter) + " cancelled");
                throw;
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
            _requested_run_start = -1;
            _replace_existing = false;
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

        // Configure the result-file identity for the next exe(...) call.
        // run_start == -1 selects the first range after the greatest occupied
        // result/process index. Replacement is deliberately unavailable in that
        // automatic mode so an explicit index is always required for data loss.
        void configureRunArtifacts(int run_start = -1, bool replace_existing = false)
        {
            if (run_start < -1)
                throw std::invalid_argument("run start must be non-negative or automatic");
            if (replace_existing && run_start < 0)
                throw std::invalid_argument("replace-existing requires an explicit run start");
            _requested_run_start = run_start;
            _replace_existing = replace_existing;
        }

        int getRunIndex() const { return exe_counter; }

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
            // Keep the one-run overload on the same artifact lifecycle as the
            // batch overload. Historically it never called logResult(), leaving
            // a misleading empty .rslt for direct C++ callers.
            exe(1, seed, std::function<void(int, int)>(), std::function<bool()>());
        }

        // n 次运行:仅播种一次,逐次记结果(修 Seed-EMPTY:去 EMPTYVALUE(NaN) 塞 time_t 的坏路径)
        void exe(int n, time_t seed)
        {
            exe(n, seed, std::function<void(int, int)>());
        }

        // 多轮运行进度：每轮结果落盘后回报“已完成轮数 / 总轮数”。
        // 观察器只用于展示，不允许其异常影响优化本身。
        void exe(int n, time_t seed, const std::function<void(int, int)>& progress)
        {
            exe(n, seed, progress, std::function<bool()>());
        }

        void exe(int n, time_t seed, const std::function<void(int, int)>& progress,
                 const std::function<bool()>& cancelled)
        {
            const int requested_start = _requested_run_start;
            const bool replace_existing = _replace_existing;
            // Configuration is one-shot. A subsequent invocation returns to safe
            // automatic allocation unless its caller opts into another range.
            _requested_run_start = -1;
            _replace_existing = false;

            const int actual_start = _logger->prepareRunBatch(
                requested_start, n, replace_existing);
            exe_counter = actual_start - 1;
            _terminator->setCancellationCheck(cancelled);
            _population->setCancellationCheck(cancelled);
            set_seed((unsigned int)seed);
            _population->seedEngines((unsigned int)seed);   // 2.5.2:播种一次,n 次 run 共享延续同一引擎流
            try
            {
                for (int i = 0; i < n; i++)
                {
                    _run(cancelled);
                    logResult();
                    _logger->commitOptimization(exe_counter);
                    if (progress)
                    {
                        try { progress(i + 1, n); }
                        catch (...) {}
                    }
                }
            }
            catch (...)
            {
                _logger->abortOptimization(exe_counter);
                _logger->finishRunBatch(false);
                throw;
            }
            _logger->finishRunBatch(true);
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
