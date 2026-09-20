//------------------------Description------------------------
// 运行结果统计。CLI 的 `evoflow report` 与 UI 的「统计分析页」共用。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <string>
#include <vector>

namespace EvoFlow
{
    namespace App
    {
        // 一个实验批次(= 一个 tag 下的全部结果)
        struct ExperimentBatch
        {
            std::string tag;
            std::vector<std::string> optimizers;   // 该批次涉及的优化器名(有序去重)
            std::vector<std::string> problems;     // 该批次涉及的问题名(有序去重)
            int repeats = 0;                       // runNumbers.size(); kept for compatibility
            std::vector<int> runNumbers;           // every actual index common to all selected cells
            std::vector<std::string> runDiagnostics; // deterministic found/missing summary per cell
            int rsltCount = 0;                     // .rslt 文件总数
            bool ragged = false;                   // true = 组合缺失、运行集合不一或编号有空洞
        };

        // 每份报告恰有一个“单次运行归约”指标。
        struct ReportReducerSpec
        {
            std::string kind = "statistic";        // statistic / percentile / indicator
            std::string metric = "smallest";
            int objective = 0;
            double percentile = 0.5;
            std::vector<std::vector<double>> references;
        };

        // 跨多轮运行的聚合统计。metric == percentile 时使用 percentile 参数。
        struct ReportStatisticSpec
        {
            std::string metric;
            double percentile = 0.5;
        };

        struct ReportComparisonSpec
        {
            std::string metric;
            std::string on = "median";
            bool minimize = true;
        };

        struct ReportChartSpec
        {
            std::string id;
            std::string kind;
            int problem = 0;
            std::string statistic = "median";
            std::string bandLow;
            std::string bandHigh;
            std::string normalization = "none";
            bool jitter = false;
            int grid = 200;
            int objectiveX = 0;
            int objectiveY = 1;
            int run = -1;
            bool connect = false;
        };

        struct ReportChartArtifact
        {
            std::string id;
            std::string kind;
            std::string title;
            std::string file;
        };

        struct ReportOptions
        {
            std::string logPath = "_log";
            std::string tag;                       // 空 = 不出报告,只列批次(见文件头)
            std::vector<std::string> optimizers;   // 空 = 该批次全部优化器
            std::vector<std::string> problems;     // 空 = 该批次全部问题
            std::string chartsDir;                 // 非空 = 顺带出图表到该目录
            bool        chartsPng = false;         // 图表格式:false=SVG(默认) / true=PNG
            std::string pngLatinFont;              // 空 = PNG 后端默认字体
            std::string pngCjkFont;                // 空 = PNG 后端默认字体
            int         pngCjkIndex = 0;
            double      pngScale = 2.0;
            bool customAnalysis = false;           // false = 保持 CLI 既有默认口径
            ReportReducerSpec reducer;             // 单个归约指标
            std::vector<ReportStatisticSpec> statistics;
            std::vector<ReportComparisonSpec> comparisons;
            std::vector<ReportChartSpec> charts;
            std::string baselineOptimizer;
        };

        struct ReportResult
        {
            bool ok = false;
            std::string error;
            std::vector<std::string> warnings;
            std::vector<ExperimentBatch> batches;  // 发现的全部批次(总是填,供 --list 与"未指定 tag"提示)
            ExperimentBatch chosen;                // 指定 tag 时:被选中的那个批次
            // 报告表格(与 ECAnalyzer::report 同源;行=问题+汇总行,列=优化器×统计量)
            std::vector<std::string> header, subheader, rows;
            std::vector<std::vector<std::string>> cells;
            std::vector<std::string> chartFiles;   // --charts 时实际产出的文件
            std::vector<ReportChartArtifact> chartArtifacts;
        };

        // 只发现批次,不做统计。路径不存在等经 error 回报。
        ReportResult discoverBatches(const std::string& logPath);
        // 完整报告。opt.tag 为空时:只填 batches 并置 error 提示需指定 tag。
        ReportResult buildReport(const ReportOptions& opt);
    }
}
