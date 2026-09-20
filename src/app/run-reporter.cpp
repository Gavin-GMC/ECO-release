//   反解 _log 目录 → 按 tag 分组 → 喂 ECAnalyzer → 取表(+可选出图)。

// SPDX-License-Identifier: BSD-3-Clause

#include "run-reporter.h"
#include "ec-analyzer.h"
#include <filesystem>
#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <cmath>
#include <sstream>
#include <iterator>

namespace EvoFlow
{
    namespace App
    {
        namespace fs = std::filesystem;

        namespace
        {
            // 从文件名反解 (tag, run)。与 ECAnalyzer::buildPath 严格对称:
            //   "<优化器>_<问题>(<tag>)_<run>.rslt"
            // ★ **问题名本身可能含括号**(如 "Synthetic(f1)")⇒ tag 必须取**最后一对**括号,
            //   用首个 '(' 会把问题名的括号误当 tag 起点。
            bool parseStem(const std::string& stem, std::string& tag, int& run)
            {
                size_t us = stem.find_last_of('_');
                if (us == std::string::npos) return false;
                std::string runPart = stem.substr(us + 1);
                if (runPart.empty()) return false;
                for (size_t i = 0; i < runPart.size(); i++)
                    if (!std::isdigit((unsigned char)runPart[i])) return false;
                try
                {
                    const unsigned long long parsed = std::stoull(runPart);
                    if (parsed > static_cast<unsigned long long>(
                            (std::numeric_limits<int>::max)())) return false;
                    run = static_cast<int>(parsed);
                }
                catch (...) { return false; }

                std::string left = stem.substr(0, us);          // "<优化器>_<问题>(<tag>)"
                if (left.empty() || left.back() != ')') return false;
                size_t open = left.find_last_of('(');
                if (open == std::string::npos) return false;
                tag = left.substr(open + 1, left.size() - open - 2);
                return true;
            }

            struct Key { std::string tag, opt, prob; };
            bool operator<(const Key& a, const Key& b)
            {
                if (a.tag != b.tag)   return a.tag < b.tag;
                if (a.opt != b.opt)   return a.opt < b.opt;
                return a.prob < b.prob;
            }

            void addUnique(std::vector<std::string>& v, const std::string& s)
            { if (std::find(v.begin(), v.end(), s) == v.end()) v.push_back(s); }

            using RunMap = std::map<Key, std::set<int>>;

            bool scanRuns(const std::string& logPath, RunMap& runs, std::string& error)
            {
                std::error_code ec;
                fs::path root = fs::u8path(logPath);
                if (!fs::exists(root, ec) || !fs::is_directory(root, ec))
                {
                    error = "log directory not found: " + logPath;
                    return false;
                }

                for (auto& optDir : fs::directory_iterator(root, ec))
                {
                    if (!optDir.is_directory()) continue;
                    std::string optName = optDir.path().filename().string();
                    for (auto& probDir : fs::directory_iterator(optDir.path(), ec))
                    {
                        if (!probDir.is_directory()) continue;
                        std::string probName = probDir.path().filename().string();
                        for (auto& f : fs::directory_iterator(probDir.path(), ec))
                        {
                            if (!f.is_regular_file() || f.path().extension() != ".rslt") continue;
                            std::string tag; int run = 0;
                            if (!parseStem(f.path().stem().string(), tag, run)) continue;
                            Key key; key.tag = tag; key.opt = optName; key.prob = probName;
                            runs[key].insert(run);
                        }
                    }
                }
                if (ec)
                {
                    error = "cannot scan log directory: " + logPath + ": " + ec.message();
                    return false;
                }
                return true;
            }

            bool contiguousRuns(const std::set<int>& runs)
            {
                if (runs.empty()) return true;
                std::set<int>::const_iterator previous = runs.begin();
                for (std::set<int>::const_iterator it = std::next(previous);
                     it != runs.end(); ++it, ++previous)
                    if (static_cast<long long>(*it) != static_cast<long long>(*previous) + 1)
                        return false;
                return true;
            }

            std::string formatRuns(const std::set<int>& runs)
            {
                std::ostringstream out;
                out << '[';
                bool first = true;
                for (std::set<int>::const_iterator it = runs.begin(); it != runs.end();)
                {
                    const int begin = *it;
                    int end = begin;
                    std::set<int>::const_iterator next = std::next(it);
                    while (next != runs.end()
                           && static_cast<long long>(*next) == static_cast<long long>(end) + 1)
                    {
                        end = *next;
                        ++next;
                    }
                    if (!first) out << ", ";
                    out << begin;
                    if (end != begin) out << '-' << end;
                    first = false;
                    it = next;
                }
                out << ']';
                return out.str();
            }

            std::string formatMissing(int low, int high, const std::set<int>& present)
            {
                std::ostringstream out;
                out << '[';
                bool first = true;
                long long cursor = low;
                for (int value : present)
                {
                    if (value < low || value > high) continue;
                    if (cursor < value)
                    {
                        if (!first) out << ", ";
                        out << cursor;
                        if (cursor != static_cast<long long>(value) - 1)
                            out << '-' << (static_cast<long long>(value) - 1);
                        first = false;
                    }
                    cursor = static_cast<long long>(value) + 1;
                }
                if (cursor <= high)
                {
                    if (!first) out << ", ";
                    out << cursor;
                    if (cursor != high) out << '-' << high;
                }
                out << ']';
                return out.str();
            }

            void analyzeRunSets(ExperimentBatch& batch, const RunMap& runs)
            {
                batch.repeats = 0;
                batch.runNumbers.clear();
                batch.runDiagnostics.clear();
                batch.rsltCount = 0;
                batch.ragged = false;

                bool haveEnvelope = false;
                int low = 0, high = 0;
                for (const std::string& optimizer : batch.optimizers)
                    for (const std::string& problem : batch.problems)
                    {
                        const Key key{batch.tag, optimizer, problem};
                        RunMap::const_iterator cell = runs.find(key);
                        if (cell == runs.end() || cell->second.empty()) continue;
                        batch.rsltCount += static_cast<int>(cell->second.size());
                        if (!haveEnvelope)
                        {
                            low = *cell->second.begin();
                            high = *cell->second.rbegin();
                            haveEnvelope = true;
                        }
                        else
                        {
                            low = (std::min)(low, *cell->second.begin());
                            high = (std::max)(high, *cell->second.rbegin());
                        }
                    }

                bool firstCell = true;
                bool missingCell = false;
                std::set<int> common;
                std::set<int> reference;
                for (const std::string& optimizer : batch.optimizers)
                    for (const std::string& problem : batch.problems)
                    {
                        const Key key{batch.tag, optimizer, problem};
                        RunMap::const_iterator cell = runs.find(key);
                        static const std::set<int> empty;
                        const std::set<int>& current = cell == runs.end() ? empty : cell->second;
                        if (current.empty())
                        {
                            batch.ragged = true;
                            missingCell = true;
                        }
                        else if (firstCell)
                        {
                            common = current;
                            reference = current;
                            firstCell = false;
                        }
                        else
                        {
                            std::set<int> intersection;
                            std::set_intersection(common.begin(), common.end(),
                                current.begin(), current.end(),
                                std::inserter(intersection, intersection.end()));
                            common.swap(intersection);
                            if (current != reference) batch.ragged = true;
                        }
                        if (!contiguousRuns(current)) batch.ragged = true;

                        std::string diagnostic = optimizer + " / " + problem
                            + ": found " + formatRuns(current);
                        if (haveEnvelope)
                            diagnostic += ", missing " + formatMissing(low, high, current);
                        else diagnostic += ", missing []";
                        batch.runDiagnostics.push_back(std::move(diagnostic));
                    }
                if (missingCell) common.clear();
                batch.runNumbers.assign(common.begin(), common.end());
                batch.repeats = static_cast<int>(batch.runNumbers.size());
            }

            bool selectNames(const std::vector<std::string>& requested,
                             const std::vector<std::string>& available,
                             const char* kind,
                             std::vector<std::string>& selected,
                             std::string& error)
            {
                if (requested.empty())
                {
                    selected = available;
                    return !selected.empty();
                }
                for (size_t i = 0; i < requested.size(); i++)
                {
                    if (std::find(available.begin(), available.end(), requested[i]) == available.end())
                    {
                        error = std::string("selected ") + kind + " is not in batch: " + requested[i];
                        return false;
                    }
                    addUnique(selected, requested[i]);
                }
                if (selected.empty())
                {
                    error = std::string("select at least one ") + kind;
                    return false;
                }
                return true;
            }

            bool statFromId(const std::string& id, ECAnalyzer::Stat& value)
            {
                struct Entry { const char* id; ECAnalyzer::Stat value; };
                static const Entry entries[] = {
                    { "mean", ECAnalyzer::Stat::Mean },
                    { "variance", ECAnalyzer::Stat::Variance },
                    { "std", ECAnalyzer::Stat::Std },
                    { "median", ECAnalyzer::Stat::Median },
                    { "mode", ECAnalyzer::Stat::Mode },
                    { "smallest", ECAnalyzer::Stat::Smallest },
                    { "largest", ECAnalyzer::Stat::Largest },
                    { "q1", ECAnalyzer::Stat::Q1 },
                    { "q3", ECAnalyzer::Stat::Q3 },
                    { "iqr", ECAnalyzer::Stat::IQR },
                    { "range", ECAnalyzer::Stat::Range },
                    { "sem", ECAnalyzer::Stat::SEM },
                    { "cv", ECAnalyzer::Stat::CV },
                    { "geometric_mean", ECAnalyzer::Stat::GeometricMean },
                    { "harmonic_mean", ECAnalyzer::Stat::HarmonicMean }
                };
                for (const Entry& entry : entries)
                    if (id == entry.id) { value = entry.value; return true; }
                return false;
            }

            bool indicatorFromId(const std::string& id, ECAnalyzer::Indicator& value)
            {
                if (id == "hypervolume") value = ECAnalyzer::Indicator::HyperVolume;
                else if (id == "gd") value = ECAnalyzer::Indicator::GD;
                else if (id == "igd") value = ECAnalyzer::Indicator::IGD;
                else if (id == "peak_ratio") value = ECAnalyzer::Indicator::PeakRatio;
                else if (id == "peak_distance") value = ECAnalyzer::Indicator::PeakDistance;
                else if (id == "peak_inaccuracy") value = ECAnalyzer::Indicator::PeakInaccuracy;
                else return false;
                return true;
            }

            bool comparisonFromId(const std::string& id, ECAnalyzer::Compare& value)
            {
                if (id == "significance") value = ECAnalyzer::Compare::Significance;
                else if (id == "rank") value = ECAnalyzer::Compare::Rank;
                else if (id == "win_tie_lose") value = ECAnalyzer::Compare::WinTieLose;
                else if (id == "best") value = ECAnalyzer::Compare::Best;
                else if (id == "kruskal_wallis") value = ECAnalyzer::Compare::KruskalWallis;
                else if (id == "wilcoxon_signed_rank") value = ECAnalyzer::Compare::WilcoxonSignedRank;
                else if (id == "friedman") value = ECAnalyzer::Compare::Friedman;
                else if (id == "nemenyi") value = ECAnalyzer::Compare::Nemenyi;
                else if (id == "dunn") value = ECAnalyzer::Compare::Dunn;
                else return false;
                return true;
            }

            bool normFromId(const std::string& id, chart_detail::Norm& value)
            {
                if (id.empty() || id == "none") value = chart_detail::Norm::None;
                else if (id == "min_max") value = chart_detail::Norm::MinMax;
                else if (id == "pooled_rank") value = chart_detail::Norm::PooledRank;
                else if (id == "ratio_to_best") value = chart_detail::Norm::RatioToBest;
                else return false;
                return true;
            }

            bool validPercentile(double value)
            {
                return std::isfinite(value) && value >= 0.0 && value <= 1.0;
            }

            std::vector<ReportStatisticSpec> defaultStatistics()
            {
                std::vector<ReportStatisticSpec> result(3);
                result[0].metric = "mean";
                result[1].metric = "std";
                result[2].metric = "median";
                return result;
            }

            std::vector<ReportComparisonSpec> defaultComparisons()
            {
                std::vector<ReportComparisonSpec> result(2);
                result[0].metric = "best"; result[0].on = "median";
                result[1].metric = "rank"; result[1].on = "median";
                return result;
            }

            std::vector<ReportChartSpec> defaultCharts()
            {
                std::vector<ReportChartSpec> result(3);
                result[0].id = "table"; result[0].kind = "table";
                result[1].id = "dist"; result[1].kind = "multi_box";
                result[2].id = "rank"; result[2].kind = "rank_bar";
                return result;
            }

            bool hasMetric(const std::vector<ReportStatisticSpec>& statistics,
                           const std::string& metric)
            {
                for (const ReportStatisticSpec& statistic : statistics)
                    if (statistic.metric == metric) return true;
                return false;
            }

            bool hasComparison(const std::vector<ReportComparisonSpec>& comparisons,
                               const std::string& metric)
            {
                for (const ReportComparisonSpec& comparison : comparisons)
                    if (comparison.metric == metric) return true;
                return false;
            }

            bool comparisonUsesSource(const std::string& metric)
            {
                return metric != "significance"
                    && metric != "kruskal_wallis"
                    && metric != "dunn";
            }

            bool validateAnalysisPlan(
                const ReportOptions& opt,
                const ExperimentBatch& chosen,
                const std::vector<ReportStatisticSpec>& statistics,
                const std::vector<ReportComparisonSpec>& comparisons,
                const std::vector<ReportChartSpec>& charts,
                std::string& error)
            {
                if (opt.reducer.objective < 0)
                {
                    error = "reducer objective index must be non-negative";
                    return false;
                }
                if (opt.reducer.kind == "statistic")
                {
                    ECAnalyzer::Stat value;
                    if (!statFromId(opt.reducer.metric, value))
                    {
                        error = "unknown reducer statistic: " + opt.reducer.metric;
                        return false;
                    }
                }
                else if (opt.reducer.kind == "percentile")
                {
                    if (!validPercentile(opt.reducer.percentile))
                    {
                        error = "reducer percentile must be between 0 and 1";
                        return false;
                    }
                }
                else if (opt.reducer.kind == "indicator")
                {
                    ECAnalyzer::Indicator value;
                    if (!indicatorFromId(opt.reducer.metric, value))
                    {
                        error = "unknown reducer indicator: " + opt.reducer.metric;
                        return false;
                    }
                    if (opt.reducer.references.size() != chosen.problems.size())
                    {
                        error = "indicator references must contain one value list per selected problem";
                        return false;
                    }
                    for (size_t i = 0; i < opt.reducer.references.size(); i++)
                        if (opt.reducer.references[i].empty())
                        {
                            error = "indicator reference values cannot be empty";
                            return false;
                        }
                }
                else
                {
                    error = "reducer kind must be statistic, percentile or indicator";
                    return false;
                }

                if (statistics.empty())
                {
                    error = "select at least one aggregation statistic";
                    return false;
                }
                if (statistics.size() > 32 || comparisons.size() > 16 || charts.size() > 32)
                {
                    error = "analysis plan contains too many entries";
                    return false;
                }

                std::set<std::string> statisticKeys;
                for (const ReportStatisticSpec& statistic : statistics)
                {
                    std::string key = statistic.metric;
                    if (statistic.metric == "percentile")
                    {
                        if (!validPercentile(statistic.percentile))
                        {
                            error = "aggregation percentile must be between 0 and 1";
                            return false;
                        }
                        key += ":" + std::to_string(statistic.percentile);
                    }
                    else
                    {
                        ECAnalyzer::Stat value;
                        if (!statFromId(statistic.metric, value))
                        {
                            error = "unknown aggregation statistic: " + statistic.metric;
                            return false;
                        }
                    }
                    if (!statisticKeys.insert(key).second)
                    {
                        error = "duplicate aggregation statistic: " + key;
                        return false;
                    }
                }

                std::set<std::string> comparisonKeys;
                for (const ReportComparisonSpec& comparison : comparisons)
                {
                    ECAnalyzer::Compare compareValue;
                    ECAnalyzer::Stat onValue;
                    if (!comparisonFromId(comparison.metric, compareValue))
                    {
                        error = "unknown comparison metric: " + comparison.metric;
                        return false;
                    }
                    if (comparisonUsesSource(comparison.metric)
                        && (!statFromId(comparison.on, onValue)
                            || !hasMetric(statistics, comparison.on)))
                    {
                        error = "comparison source statistic is not selected: " + comparison.on;
                        return false;
                    }
                    if (!comparisonKeys.insert(comparison.metric).second)
                    {
                        error = "duplicate comparison metric: " + comparison.metric;
                        return false;
                    }
                }

                std::set<std::string> chartIds;
                for (const ReportChartSpec& chart : charts)
                {
                    if (chart.id.empty() || chart.id.size() > 64)
                    {
                        error = "each chart needs an id of at most 64 characters";
                        return false;
                    }
                    for (unsigned char ch : chart.id)
                        if (!std::isalnum(ch) && ch != '-' && ch != '_')
                        {
                            error = "chart id contains unsupported characters";
                            return false;
                        }
                    if (!chartIds.insert(chart.id).second)
                    {
                        error = "duplicate chart id: " + chart.id;
                        return false;
                    }
                    static const char* kinds[] = {
                        "table", "box", "multi_box", "bar", "multi_bar",
                        "rank_bar", "rank_heatmap", "win_tie_lose",
                        "cd_diagram", "convergence", "pareto"
                    };
                    bool known = false;
                    for (const char* kind : kinds) if (chart.kind == kind) known = true;
                    if (!known)
                    {
                        error = "unknown chart kind: " + chart.kind;
                        return false;
                    }
                    chart_detail::Norm norm;
                    if (!normFromId(chart.normalization, norm))
                    {
                        error = "unknown chart normalization: " + chart.normalization;
                        return false;
                    }
                    const bool perProblem = chart.kind == "box" || chart.kind == "bar"
                        || chart.kind == "convergence" || chart.kind == "pareto";
                    if (perProblem
                        && (chart.problem < 0
                            || chart.problem >= static_cast<int>(chosen.problems.size())))
                    {
                        error = "chart problem index is outside the selected problem list";
                        return false;
                    }
                    if ((chart.kind == "bar" || chart.kind == "multi_bar"
                         || chart.kind == "rank_heatmap" || chart.kind == "convergence")
                        && (!hasMetric(statistics, chart.statistic)
                            || chart.statistic == "percentile"))
                    {
                        error = "chart statistic is not a selected named statistic: "
                            + chart.statistic;
                        return false;
                    }
                    if (!chart.bandLow.empty() || !chart.bandHigh.empty())
                    {
                        ECAnalyzer::Stat low, high;
                        if (chart.bandLow.empty() || chart.bandHigh.empty()
                            || !statFromId(chart.bandLow, low)
                            || !statFromId(chart.bandHigh, high))
                        {
                            error = "chart band needs two named statistics";
                            return false;
                        }
                    }
                    if (chart.kind == "rank_bar"
                        && !hasComparison(comparisons, "rank")
                        && !hasComparison(comparisons, "nemenyi"))
                    {
                        error = "average-rank chart requires Rank or Nemenyi comparison";
                        return false;
                    }
                    if (chart.kind == "win_tie_lose"
                        && !hasComparison(comparisons, "win_tie_lose"))
                    {
                        error = "win/tie/loss chart requires Win/Tie/Lose comparison";
                        return false;
                    }
                    if (chart.kind == "cd_diagram"
                        && !hasComparison(comparisons, "nemenyi"))
                    {
                        error = "critical-difference chart requires Nemenyi comparison";
                        return false;
                    }
                    if (chart.grid < 2 || chart.grid > 10000
                        || chart.objectiveX < 0 || chart.objectiveY < 0
                        || chart.run < -1)
                    {
                        error = "chart parameters are outside their supported range";
                        return false;
                    }
                }
                return true;
            }

            std::string chartTitle(const ReportChartSpec& chart,
                                   const ExperimentBatch& chosen)
            {
                std::string title;
                if (chart.kind == "table") title = "Results";
                else if (chart.kind == "box") title = "Distribution";
                else if (chart.kind == "multi_box") title = "Cross-problem distribution";
                else if (chart.kind == "bar") title = "Statistic";
                else if (chart.kind == "multi_bar") title = "Cross-problem statistic";
                else if (chart.kind == "rank_bar") title = "Average rank";
                else if (chart.kind == "rank_heatmap") title = "Rank heatmap";
                else if (chart.kind == "win_tie_lose") title = "Win / tie / loss";
                else if (chart.kind == "cd_diagram") title = "Critical difference";
                else if (chart.kind == "convergence") title = "Convergence";
                else if (chart.kind == "pareto") title = "Pareto front";
                const bool perProblem = chart.kind == "box" || chart.kind == "bar"
                    || chart.kind == "convergence" || chart.kind == "pareto";
                if (perProblem && chart.problem >= 0
                    && chart.problem < static_cast<int>(chosen.problems.size()))
                    title += " - " + chosen.problems[chart.problem];
                return title + " - " + chosen.tag;
            }
        }

        ReportResult discoverBatches(const std::string& logPath)
        {
            ReportResult r;
            RunMap runs;
            if (!scanRuns(logPath, runs, r.error)) return r;

            std::map<std::string, ExperimentBatch> byTag;
            for (RunMap::const_iterator it = runs.begin(); it != runs.end(); ++it)
            {
                ExperimentBatch& b = byTag[it->first.tag];
                b.tag = it->first.tag;
                addUnique(b.optimizers, it->first.opt);
                addUnique(b.problems,   it->first.prob);
            }
            for (std::map<std::string, ExperimentBatch>::iterator it = byTag.begin(); it != byTag.end(); ++it)
            {
                ExperimentBatch& batch = it->second;
                analyzeRunSets(batch, runs);
                r.batches.push_back(it->second);
            }
            r.ok = true;
            if (r.batches.empty()) r.error = "no .rslt found under: " + logPath;
            return r;
        }

        ReportResult buildReport(const ReportOptions& opt)
        {
            ReportResult r = discoverBatches(opt.logPath);
            if (!r.ok || r.batches.empty()) return r;

            if (opt.tag.empty())
            {
                // **不猜**:跨批次统计在语义上就是错的(见头文件)。列出批次让用户选。
                r.ok = false;
                r.error = "no --tag given; pick one of the experiment batches listed above";
                return r;
            }

            const ExperimentBatch* found = nullptr;
            for (size_t i = 0; i < r.batches.size(); i++)
                if (r.batches[i].tag == opt.tag) { found = &r.batches[i]; break; }
            if (!found) { r.ok = false; r.error = "no such tag: " + opt.tag; return r; }

            ExperimentBatch chosen;
            chosen.tag = found->tag;
            if (!selectNames(opt.optimizers, found->optimizers, "optimizer",
                             chosen.optimizers, r.error)
                || !selectNames(opt.problems, found->problems, "problem",
                                chosen.problems, r.error))
            {
                r.ok = false;
                return r;
            }
            if (!opt.baselineOptimizer.empty())
            {
                std::vector<std::string>::iterator baseline =
                    std::find(chosen.optimizers.begin(), chosen.optimizers.end(),
                              opt.baselineOptimizer);
                if (baseline == chosen.optimizers.end())
                {
                    r.ok = false;
                    r.error = "baseline optimizer is not selected: " + opt.baselineOptimizer;
                    return r;
                }
                std::rotate(chosen.optimizers.begin(), baseline, baseline + 1);
            }

            RunMap runs;
            if (!scanRuns(opt.logPath, runs, r.error))
            {
                r.ok = false;
                return r;
            }
            analyzeRunSets(chosen, runs);
            if (chosen.repeats <= 0)
            {
                r.ok = false;
                r.error = "selected batch has no common runs";
                r.chosen = chosen;
                return r;
            }
            if (chosen.ragged)
                r.warnings.push_back("run sets differ or contain gaps; report uses every common result index");
            r.chosen = chosen;

            const std::vector<ReportStatisticSpec> statistics =
                opt.customAnalysis ? opt.statistics : defaultStatistics();
            const std::vector<ReportComparisonSpec> comparisons =
                opt.customAnalysis ? opt.comparisons : defaultComparisons();
            const std::vector<ReportChartSpec> charts =
                opt.customAnalysis ? opt.charts : defaultCharts();
            if (!validateAnalysisPlan(opt, chosen, statistics, comparisons, charts, r.error))
            {
                r.ok = false;
                return r;
            }

            try
            {
                ECAnalyzer az;
                az.logRoot(opt.logPath);
                for (size_t i = 0; i < chosen.optimizers.size(); i++)
                    az.addOptimizer(chosen.optimizers[i], chosen.tag, chosen.runNumbers);
                az.addProblem(chosen.problems);
                if (opt.reducer.kind == "percentile")
                    az.reduceByPercentile(opt.reducer.percentile, opt.reducer.objective);
                else if (opt.reducer.kind == "indicator")
                {
                    ECAnalyzer::Indicator indicator;
                    indicatorFromId(opt.reducer.metric, indicator);
                    az.reduceBy(indicator, opt.reducer.references);
                }
                else
                {
                    ECAnalyzer::Stat reducer;
                    statFromId(opt.reducer.metric, reducer);
                    az.reduceBy(reducer, opt.reducer.objective);
                }
                for (const ReportStatisticSpec& statistic : statistics)
                {
                    if (statistic.metric == "percentile")
                        az.addPercentile(statistic.percentile);
                    else
                    {
                        ECAnalyzer::Stat value;
                        statFromId(statistic.metric, value);
                        az.addStatistic(value);
                    }
                }
                for (const ReportComparisonSpec& comparison : comparisons)
                {
                    ECAnalyzer::Compare compareValue;
                    ECAnalyzer::Stat onValue = ECAnalyzer::Stat::Mean;
                    comparisonFromId(comparison.metric, compareValue);
                    statFromId(comparison.on, onValue);
                    az.addComparison(
                        compareValue, onValue,
                        comparison.minimize
                            ? ECAnalyzer::Direction::MinIsBetter
                            : ECAnalyzer::Direction::MaxIsBetter);
                }
                az.run();

                // 取表:与 report(Format) 同源(analyzer 内部同一个 _analysis_table)
                std::vector<std::vector<int>> bolds;
                az.tableForReport(r.header, r.subheader, r.rows, r.cells, bolds);

                if (!opt.chartsDir.empty())
                {
                    std::error_code ec;
                    const fs::path chartRoot = fs::u8path(opt.chartsDir);
                    fs::create_directories(chartRoot, ec);
                    if (ec)
                    {
                        r.ok = false;
                        r.error = "cannot create chart directory: " + opt.chartsDir + ": " + ec.message();
                        return r;
                    }
                    FileOut::ChartFormat cf = opt.chartsPng ? FileOut::ChartFormat::Png
                                                            : FileOut::ChartFormat::Svg;
                    const std::string ext = opt.chartsPng ? ".png" : ".svg";
                    FileOut::PngFontConfig font;
                    if (!opt.pngLatinFont.empty()) font.latin = opt.pngLatinFont;
                    if (!opt.pngCjkFont.empty()) font.cjk = opt.pngCjkFont;
                    font.cjkIndex = opt.pngCjkIndex;
                    font.scale = opt.pngScale;

                    for (const ReportChartSpec& chart : charts)
                    {
                        const std::string base = (chartRoot / chart.id).u8string();
                        const std::string fileName = base + ext;
                        const std::string title = chartTitle(chart, chosen);
                        fs::remove(fs::u8path(fileName), ec); ec.clear();

                        ECAnalyzer::Stat statistic = ECAnalyzer::Stat::Median;
                        statFromId(chart.statistic, statistic);
                        ECAnalyzer::Stat bandLow = ECAnalyzer::Stat::Q1;
                        ECAnalyzer::Stat bandHigh = ECAnalyzer::Stat::Q3;
                        const bool hasBand = !chart.bandLow.empty() && !chart.bandHigh.empty();
                        if (hasBand)
                        {
                            statFromId(chart.bandLow, bandLow);
                            statFromId(chart.bandHigh, bandHigh);
                        }
                        chart_detail::Norm norm = chart_detail::Norm::None;
                        normFromId(chart.normalization, norm);

                        if (chart.kind == "table")
                        {
                            az.chart().table(base, title)
                                .format(cf).pngFont(font).draw();
                        }
                        else if (chart.kind == "box")
                        {
                            az.chart().boxPlot(base, title)
                                .problem(chart.problem).norm(norm).jitter(chart.jitter)
                                .format(cf).pngFont(font).draw();
                        }
                        else if (chart.kind == "multi_box")
                        {
                            az.chart().multiBoxPlot(base, title)
                                .norm(norm).jitter(chart.jitter)
                                .format(cf).pngFont(font).draw();
                        }
                        else if (chart.kind == "bar")
                        {
                            BarChartBuilder builder = az.chart().barChart(base, title);
                            builder.problem(chart.problem).norm(norm).statistic(statistic);
                            if (hasBand) builder.band(bandLow, bandHigh);
                            else builder.noBand();
                            builder.format(cf).pngFont(font).draw();
                        }
                        else if (chart.kind == "multi_bar")
                        {
                            BarChartBuilder builder = az.chart().multiBarChart(base, title);
                            builder.norm(norm).statistic(statistic);
                            if (hasBand) builder.band(bandLow, bandHigh);
                            else builder.noBand();
                            builder.format(cf).pngFont(font).draw();
                        }
                        else if (chart.kind == "rank_bar")
                        {
                            az.chart().rankBar(base, title)
                                .format(cf).pngFont(font).draw();
                        }
                        else if (chart.kind == "rank_heatmap")
                        {
                            az.chart().rankHeatmap(base, title).statistic(statistic)
                                .format(cf).pngFont(font).draw();
                        }
                        else if (chart.kind == "win_tie_lose")
                        {
                            az.chart().winTieLose(base, title)
                                .format(cf).pngFont(font).draw();
                        }
                        else if (chart.kind == "cd_diagram")
                        {
                            az.chart().cdDiagram(base, title)
                                .format(cf).pngFont(font).draw();
                        }
                        else if (chart.kind == "convergence")
                        {
                            ConvergenceBuilder builder = az.chart().convergence(base, title);
                            builder.problem(chart.problem).grid(chart.grid).statistic(statistic);
                            if (hasBand) builder.band(bandLow, bandHigh);
                            builder.format(cf).pngFont(font).draw();
                        }
                        else if (chart.kind == "pareto")
                        {
                            az.chart().paretoFront(base, title)
                                .problem(chart.problem)
                                .objectives(chart.objectiveX, chart.objectiveY)
                                .run(chart.run).connect(chart.connect)
                                .format(cf).pngFont(font).draw();
                        }

                        const fs::path file = fs::u8path(fileName);
                        if (!fs::exists(file, ec) || fs::file_size(file, ec) == 0 || ec)
                        {
                            r.ok = false;
                            r.error = "chart output was not created for '" + chart.id
                                + "'; verify that its required data and comparison metrics exist";
                            return r;
                        }
                        ReportChartArtifact artifact;
                        artifact.id = chart.id;
                        artifact.kind = chart.kind;
                        artifact.title = title;
                        artifact.file = fileName;
                        r.chartArtifacts.push_back(artifact);
                        r.chartFiles.push_back(fileName);
                    }
                }
                r.ok = true;
            }
            catch (const std::exception& ex) { r.ok = false; r.error = ex.what(); }
            catch (...)                      { r.ok = false; r.error = "unknown error while building report"; }
            return r;
        }
    }
}
