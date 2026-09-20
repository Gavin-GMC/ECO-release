//------------------------Description------------------------
// 应用门面层。CLI 与将来的 UI【共用同一套逻辑】的分界线。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <functional>
#include <string>
#include <vector>

namespace EvoFlow
{
    namespace App
    {
        // ---- 问题定义态元信息（不 compile）----
        struct VariableInfo
        {
            std::string name;
            std::string kind;
            std::string valueType;
            int         length = 0;
            double      lowerBound = 0;
            double      upperBound = 0;
            bool        hasBounds = false;
        };

        struct ObjectiveInfo
        {
            std::string              name;
            std::vector<std::string> variables;
            bool                     minimize = true;
            int                      priority = 0;
        };

        struct ConstraintInfo
        {
            std::string              type;
            std::vector<std::string> variables;
            double                   penalty = 0;
            std::vector<std::string> objectives;
        };

        struct ProblemDescription
        {
            bool                        ok = false;
            std::string                 error;
            std::string                 type;
            std::string                 name;
            int                         dimension = 0;
            std::vector<VariableInfo>   variables;
            std::vector<ObjectiveInfo>  objectives;
            std::vector<ConstraintInfo> constraints;
        };

        ProblemDescription describeProblem(
            const std::string& path,
            const std::string& logicalName = std::string());

        // Generated model data is untrusted: require the classified TYPE to match,
        // load through the registry, preserve definition-side metadata, and compile
        // once as a mechanical assembly check. No optimization is run.
        ProblemDescription compileAndDescribeProblem(
            const std::string& path, const std::string& expectedType,
            const std::string& logicalName = std::string());

        // ---- 跑一次优化 ----
        struct RunOptions
        {
            std::string problemPath;
            // 应用层已确认的稳定问题标识。空值保留模板原有命名规则。
            std::string problemName;
            std::string configPath;
            // Optional stable output identity. Empty keeps the optimizer name
            // declared by the configuration file.
            std::string optimizerName;
            // 实验批次标识。**空 = 保持既有行为**(用 .cfg 里的 tag;若那也是空,装配时回落到时间戳)。
            //   v2.3.1.5 加此项的原因:`.cfg` 的 tag 普遍为空 → 每次 run 都得到一个**新的时间戳 tag**
            //   ⇒ 同一实验跑多次会散成多个批次,`report --tag` 根本聚合不起来。
            //   tag 即"实验批次":同批各次运行显式共用一个 tag,report 按它分组。
            std::string tag;
            unsigned    seed    = 1;
            int         runs    = 1;
            int         maxFES  = -1;      // <0 = 不覆盖配置里的预算
            int         maxConv = -1;
            int         maxTime = -1;
            int         runStart = -1;      // -1 = allocate after the greatest occupied index
            bool        replaceExisting = false; // requires an explicit runStart
            bool        verbose = false;   // 逐代进度回显到控制台
        };

        struct RunSolution
        {
            double              fitness = 0;
            std::vector<double> objectives;
            std::string         text;      // decoder 格式化后的完整解
        };

        struct RunRoundResult
        {
            int                      run = 0; // 1-based
            int                      resultIndex = -1; // actual zero-based .rslt/.prcs suffix
            std::vector<RunSolution> best;
            double                   bestFitness = 0;
        };

        struct RunResult
        {
            bool        ok = false;
            bool        cancelled = false;
            std::string error;             // ok=false 时的原因(不抛异常)
            std::string tag;               // 实际使用的运行标签
            std::string optimizerName;     // actual output identity
            int         runStart = -1;     // first actual result index, if a round completed
            std::vector<RunSolution> best; // 最终解集(多目标时含多解)
            std::vector<RunRoundResult> rounds; // 每轮独立结果，按运行顺序保存
            // 便捷字段:best 为空时无意义
            double      bestFitness = 0;
        };

        // 载问题 → 读配置 → 覆盖预算 → 装配 → 运行。返回结果,不输出、不抛。
        using RunProgressCallback = std::function<void(int, int)>;
        using RunCancellationCallback = std::function<bool()>;
        RunResult runOptimization(const RunOptions& opt);
        RunResult runOptimization(const RunOptions& opt,
                                  const RunProgressCallback& progress);
        RunResult runOptimization(const RunOptions& opt,
                                  const RunProgressCallback& progress,
                                  const RunCancellationCallback& cancelled);
    }
}
