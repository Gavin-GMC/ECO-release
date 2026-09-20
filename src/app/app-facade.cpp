// SPDX-License-Identifier: BSD-3-Clause

#include "app-facade.h"
#include <fstream>
#include <memory>
#include <stdexcept>
#include "evoflow.h"
#include "problem-template.h"

namespace EvoFlow
{
    namespace App
    {
        namespace
        {
            // 与 main.cpp 原实现一致:把 "dir/name.cfg" 拆成 (dir, name)
            void splitConfigPath(const std::string& p, std::string& dir, std::string& name)
            {
                size_t slash = p.find_last_of("/\\");
                if (slash == std::string::npos) { dir = "."; name = p; }
                else { dir = p.substr(0, slash); name = p.substr(slash + 1); }
                size_t dot = name.find_last_of('.');
                if (dot != std::string::npos) name = name.substr(0, dot);
            }

            void validateOptimizerName(const std::string& name)
            {
                if (name.empty())
                    throw std::invalid_argument("optimizer name must not be empty");
                if (name == "." || name == "..")
                    throw std::invalid_argument("optimizer name cannot be a path segment");
                if (name.size() > 128)
                    throw std::invalid_argument("optimizer name must not exceed 128 characters");
                for (unsigned char ch : name)
                    if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
                          || (ch >= '0' && ch <= '9') || ch == '_'
                          || ch == '-' || ch == '.' || ch == '+'))
                        throw std::invalid_argument(
                            "optimizer name may contain only ASCII letters, digits, '.', '_', '-' and '+'");
            }

            ProblemDescription inspectProblem(const std::string& path,
                                              const std::string& expectedType,
                                              const std::string& logicalName,
                                              bool compile)
            {
                ProblemDescription result;
                try
                {
                    result.type = peekProblemType(path);
                    if (!expectedType.empty() && result.type != expectedType)
                        throw std::runtime_error(
                            "problem TYPE does not match the classified template");
                    std::unique_ptr<Problem> problem(loadProblem(path));
                    if (!logicalName.empty()) problem->setName(logicalName);
                    ProblemDefinitionInfo info = problem->describeDefinition();
                    result.name = info.name;
                    result.dimension = info.dimension;

                    for (const ProblemVariableInfo& source : info.variables)
                    {
                        VariableInfo target;
                        target.name = source.name;
                        target.kind = source.kind;
                        target.valueType = source.valueType;
                        target.length = source.length;
                        target.lowerBound = source.lowerBound;
                        target.upperBound = source.upperBound;
                        target.hasBounds = source.hasBounds;
                        result.variables.push_back(std::move(target));
                    }
                    for (const ProblemObjectiveInfo& source : info.objectives)
                    {
                        ObjectiveInfo target;
                        target.name = source.name;
                        target.variables = source.variables;
                        target.minimize = source.minimize;
                        target.priority = source.priority;
                        result.objectives.push_back(std::move(target));
                    }
                    for (const ProblemConstraintInfo& source : info.constraints)
                    {
                        ConstraintInfo target;
                        target.type = source.type;
                        target.variables = source.variables;
                        target.penalty = source.penalty;
                        target.objectives = source.objectives;
                        result.constraints.push_back(std::move(target));
                    }

                    if (compile)
                    {
                        std::unique_ptr<ProblemHandle> handle(problem->compile());
                        if (!handle)
                            throw std::runtime_error("problem compile returned no handle");
                    }
                    result.ok = true;
                }
                catch (const std::exception& ex)
                {
                    result.ok = false;
                    result.error = ex.what();
                }
                catch (...)
                {
                    result.ok = false;
                    result.error = "unknown error";
                }
                return result;
            }

            RunRoundResult collectRoundResult(Optimizer* optimizer, int run)
            {
                RunRoundResult round;
                round.run = run;
                round.resultIndex = optimizer ? optimizer->getRunIndex() : -1;
                Solution* best = nullptr;
                int bestSize = 0;
                if (!optimizer || !optimizer->getBest(best, bestSize)
                    || !best || bestSize <= 0)
                    return round;
                for (int i = 0; i < bestSize; i++)
                {
                    RunSolution solution;
                    solution.fitness = best[i].fitness[0];
                    const int objectiveCount =
                        best[i].decoder_pointer->getObjectNumber();
                    for (int k = 0; k < objectiveCount; k++)
                        solution.objectives.push_back(best[i].fitness[k]);
                    solution.text = best[i].decoder_pointer->toString(
                        best[i].result, best[i].fitness, 1);
                    round.best.push_back(std::move(solution));
                }
                round.bestFitness = round.best[0].fitness;
                return round;
            }
        }

        ProblemDescription describeProblem(
            const std::string& path, const std::string& logicalName)
        {
            return inspectProblem(path, std::string(), logicalName, false);
        }

        ProblemDescription compileAndDescribeProblem(
            const std::string& path, const std::string& expectedType,
            const std::string& logicalName)
        {
            return inspectProblem(path, expectedType, logicalName, true);
        }

        RunResult runOptimization(const RunOptions& o)
        {
            return runOptimization(o, RunProgressCallback());
        }

        RunResult runOptimization(const RunOptions& o,
                                  const RunProgressCallback& progress)
        {
            return runOptimization(o, progress, RunCancellationCallback());
        }

        RunResult runOptimization(const RunOptions& o,
                                  const RunProgressCallback& progress,
                                  const RunCancellationCallback& cancelled)
        {
            RunResult r;
            Problem*   problem = nullptr;
            Optimizer* opt     = nullptr;
            try
            {
                // 1) 读问题
                problem = loadProblem(o.problemPath);
                if (!o.problemName.empty()) problem->setName(o.problemName);

                // 2) 读配置
                std::string cdir, cname;
                splitConfigPath(o.configPath, cdir, cname);
                FullConfig fc = ConfigBuilder::loadConfig(cname, cdir);
                if (!o.optimizerName.empty()) fc.optimizer.name = o.optimizerName;
                validateOptimizerName(fc.optimizer.name);

                // 3) 覆盖:预算(总预算 → 全局与各子群同置) + verbose(进度回显)
                auto applyTerm = [&](int idx, int val) {
                    if (val < 0) return;
                    fc.optimizer.terminate_conditions[idx] = val;
                    for (auto& sp : fc.optimizer.subpopulations) sp.terminate_conditions[idx] = val;
                };
                applyTerm(0, o.maxFES);
                applyTerm(1, o.maxConv);
                applyTerm(2, o.maxTime);
                if (!o.tag.empty()) fc.optimizer.tag = o.tag;   // v2.3.1.5:实验批次标识(空则不覆盖)
                if (o.verbose) { fc.optimizer.logger_process = true; fc.optimizer.logger_console_echo = true; }

                // 4) 装配 Optimizer
                OptimizerBuilder builder;
                for (auto& wf : fc.workflows) builder.registerWorkflow(wf);
                opt = builder.buildOptimizer(fc.optimizer);
                r.tag = opt->getTag();
                r.optimizerName = fc.optimizer.name;

                // 5) 运行(runs 次;n>1 逐次落 .rslt 日志)
                opt->setProblem(problem);
                opt->configureRunArtifacts(o.runStart, o.replaceExisting);
                opt->exe(
                    (o.runs < 1 ? 1 : o.runs), (time_t)o.seed,
                    [&](int completed, int total) {
                        r.rounds.push_back(collectRoundResult(opt, completed));
                        if (progress) progress(completed, total);
                    },
                    cancelled);

                // 兼容旧消费方：顶层 best 仍指向最后一轮；完整逐轮结果见 rounds。
                if (!r.rounds.empty() && !r.rounds.back().best.empty())
                {
                    r.runStart = r.rounds.front().resultIndex;
                    r.best = r.rounds.back().best;
                    r.bestFitness = r.rounds.back().bestFitness;
                    r.ok = true;
                }
                else
                {
                    r.error = "no solution produced";
                }
            }
            catch (const OptimizationCancelled& ex)
            {
                r.ok = false;
                r.cancelled = true;
                r.error = ex.what();
            }
            catch (const std::exception& ex)
            {
                r.ok = false;
                r.error = ex.what();          // 契约:不抛,经返回值回报
            }
            catch (...)
            {
                r.ok = false;
                r.error = "unknown error";
            }
            if (r.runStart < 0 && opt && opt->getRunIndex() >= 0)
                r.runStart = !r.rounds.empty()
                    ? r.rounds.front().resultIndex : opt->getRunIndex();
            delete opt;
            delete problem;
            return r;
        }
    }
}
