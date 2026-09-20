// SPDX-License-Identifier: BSD-3-Clause

#include "bindings.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include "ec-analyzer.h"
#include "prcs-reader.h"
#include "run-reporter.h"

namespace py = pybind11;
namespace fs = std::filesystem;
using namespace EvoFlow;

namespace
{
    enum class PyChartKind
    {
        Box,
        MultiBox,
        Bar,
        MultiBar,
        Convergence,
        Pareto,
        Table,
        WinTieLose,
        RankHeatmap,
        RankBar,
        CdDiagram
    };

    enum class PyNormalization { None, MinMax, PooledRank, RatioToBest };

    struct NativeAnalysisTable
    {
        std::vector<std::string> header;
        std::vector<std::string> subheader;
        std::vector<std::string> rows;
        std::vector<std::vector<std::string>> cells;
        std::vector<std::vector<int>> bolds;
    };

    struct NativeChartOptions
    {
        PyChartKind kind = PyChartKind::Table;
        std::string path;
        std::string title;
        std::vector<int> problems;
        PyNormalization normalization = PyNormalization::None;
        bool jitter = false;
        bool hasStatistic = false;
        Stat statistic = Stat::Median;
        bool hasBand = false;
        Stat bandLow = Stat::Q1;
        Stat bandHigh = Stat::Q3;
        int grid = 200;
        int objectiveX = 0;
        int objectiveY = 1;
        int run = -1;
        std::string swarm;
        bool connect = false;
        FileOut::ChartFormat format = FileOut::ChartFormat::Svg;
        bool hasStyle = false;
        FileOut::Palette palette = FileOut::Palette::ColorblindSafe;
        FileOut::Lang language = FileOut::Lang::EN;
        bool caption = true;
        bool logScale = false;
        bool reverseY = false;
        FileOut::PngFontConfig pngFont;
    };

    struct ProcessLog
    {
        PrcsReader::Header header;
        std::vector<PrcsReader::Record> records;
    };

    struct ProcessScanResult
    {
        PrcsReader::Header header;
        long long records = 0;
    };

    std::mutex& analysisMutex()
    {
        static std::mutex value;
        return value;
    }

    FileOut::ChartStyle chartStyle(const NativeChartOptions& options)
    {
        FileOut::ChartStyle style;
        style.palette(options.palette)
            .lang(options.language)
            .caption(options.caption)
            .logScale(options.logScale)
            .reverseY(options.reverseY);
        return style;
    }

    chart_detail::Norm chartNorm(PyNormalization value)
    {
        switch (value)
        {
        case PyNormalization::MinMax: return chart_detail::Norm::MinMax;
        case PyNormalization::PooledRank: return chart_detail::Norm::PooledRank;
        case PyNormalization::RatioToBest: return chart_detail::Norm::RatioToBest;
        default: return chart_detail::Norm::None;
        }
    }

    std::string slug(const std::string& value)
    {
        std::string result;
        for (unsigned char ch : value)
            result += (std::isalnum(ch) || ch == '-' || ch == '_')
                ? static_cast<char>(ch) : '_';
        return result;
    }

    bool hasExtension(const std::string& path)
    {
        const size_t slash = path.find_last_of("/\\");
        const size_t dot = path.find_last_of('.');
        return dot != std::string::npos
            && (slash == std::string::npos || dot > slash + 1);
    }

    std::string outputPath(std::string path, const std::string& extension)
    {
        if (path.find('/') == std::string::npos
            && path.find('\\') == std::string::npos)
            path = "_analysis/" + path;

        const size_t slash = path.find_last_of("/\\");
        const size_t fileBegin = slash == std::string::npos ? 0 : slash + 1;
        const size_t dot = path.find_last_of('.');
        const std::string wanted = "." + extension;
        if (dot == std::string::npos || dot < fileBegin || dot == fileBegin)
            return path + wanted;

        std::string current = path.substr(dot);
        std::string lowerCurrent = current;
        std::string lowerWanted = wanted;
        std::transform(lowerCurrent.begin(), lowerCurrent.end(), lowerCurrent.begin(),
            [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        std::transform(lowerWanted.begin(), lowerWanted.end(), lowerWanted.begin(),
            [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        return lowerCurrent == lowerWanted ? path : path.substr(0, dot) + wanted;
    }

    void prepareArtifact(const std::string& path)
    {
        const fs::path native = fs::u8path(path);
        const fs::path parent = native.parent_path();
        std::error_code error;
        if (!parent.empty()) fs::create_directories(parent, error);
        if (error)
            throw std::runtime_error(
                "cannot create output directory for " + path + ": " + error.message());
        fs::remove(native, error);
        if (error)
            throw std::runtime_error(
                "cannot replace output file " + path + ": " + error.message());
    }

    void verifyArtifact(const std::string& path)
    {
        std::error_code error;
        const fs::path native = fs::u8path(path);
        const bool exists = fs::is_regular_file(native, error);
        const uintmax_t size = exists ? fs::file_size(native, error) : 0;
        if (error || !exists || size == 0)
            throw std::runtime_error("native analysis did not produce a non-empty file: " + path);
    }

    template <class Builder>
    void finishChart(Builder& builder, const NativeChartOptions& options)
    {
        if (options.hasStyle) builder.style(chartStyle(options));
        builder.format(options.format).pngFont(options.pngFont).draw();
    }

    class NativeAnalyzer
    {
    public:
        explicit NativeAnalyzer(const std::string& logRoot)
        {
            _analyzer.logRoot(logRoot);
        }

        NativeAnalyzer& addOptimizer(
            const std::string& optimizer, const std::string& tag, int repeats)
        {
            requireIdle();
            if (optimizer.empty()) throw py::value_error("optimizer name cannot be empty");
            if (tag.empty()) throw py::value_error("optimizer tag cannot be empty");
            if (repeats <= 0) throw py::value_error("repeats must be positive");
            _analyzer.addOptimizer(optimizer, tag, repeats);
            _optimizerCount++;
            _ran = false;
            return *this;
        }

        NativeAnalyzer& addProblem(const std::string& problem)
        {
            requireIdle();
            if (problem.empty()) throw py::value_error("problem name cannot be empty");
            _analyzer.addProblem(problem);
            _problems.push_back(problem);
            _ran = false;
            return *this;
        }

        NativeAnalyzer& reduceStatistic(Stat statistic, int objective)
        {
            requireIdle();
            if (objective < 0) throw py::value_error("objective must be non-negative");
            _analyzer.reduceBy(statistic, objective);
            _hasReducer = true;
            _ran = false;
            return *this;
        }

        NativeAnalyzer& reducePercentile(double percentile, int objective)
        {
            requireIdle();
            if (!std::isfinite(percentile) || percentile < 0.0 || percentile > 1.0)
                throw py::value_error("percentile must be between 0 and 1");
            if (objective < 0) throw py::value_error("objective must be non-negative");
            _analyzer.reduceByPercentile(percentile, objective);
            _hasReducer = true;
            _ran = false;
            return *this;
        }

        NativeAnalyzer& reduceIndicator(
            ECAnalyzer::Indicator indicator,
            const std::vector<std::vector<double>>& references)
        {
            requireIdle();
            if (references.size() != _problems.size())
                throw py::value_error(
                    "indicator references need one value list per configured problem");
            for (const std::vector<double>& reference : references)
                if (reference.empty())
                    throw py::value_error("indicator reference values cannot be empty");
            _analyzer.reduceBy(indicator, references);
            _hasReducer = true;
            _ran = false;
            return *this;
        }

        NativeAnalyzer& addStatistic(Stat statistic)
        {
            requireIdle();
            if (!_statistics.insert(statistic).second)
                throw py::value_error("duplicate aggregation statistic");
            _analyzer.addStatistic(statistic);
            _ran = false;
            return *this;
        }

        NativeAnalyzer& addPercentile(double percentile)
        {
            requireIdle();
            if (!std::isfinite(percentile) || percentile < 0.0 || percentile > 1.0)
                throw py::value_error("percentile must be between 0 and 1");
            if (!_percentiles.insert(percentile).second)
                throw py::value_error("duplicate aggregation percentile");
            _analyzer.addPercentile(percentile);
            _ran = false;
            return *this;
        }

        NativeAnalyzer& addComparison(
            ECAnalyzer::Compare comparison, Stat on, ECAnalyzer::Direction direction)
        {
            requireIdle();
            if (!_comparisons.insert(comparison).second)
                throw py::value_error("duplicate comparison");
            if (comparison != ECAnalyzer::Compare::Significance
                && comparison != ECAnalyzer::Compare::KruskalWallis
                && comparison != ECAnalyzer::Compare::Dunn
                && _statistics.find(on) == _statistics.end())
                throw py::value_error(
                    "comparison source must be an added named aggregation statistic");
            _analyzer.addComparison(comparison, on, direction);
            _ran = false;
            return *this;
        }

        NativeAnalysisTable run()
        {
            requireIdle();
            if (_optimizerCount == 0) throw py::value_error("add at least one optimizer");
            if (_problems.empty()) throw py::value_error("add at least one problem");
            if (!_hasReducer) throw py::value_error("configure one run reducer");
            if (_statistics.empty() && _percentiles.empty())
                throw py::value_error("add at least one aggregation statistic");

            Busy busy(*this);
            NativeAnalysisTable table;
            {
                py::gil_scoped_release release;
                std::lock_guard<std::mutex> lock(analysisMutex());
                _analyzer.run();
                _analyzer.tableForReport(
                    table.header, table.subheader, table.rows, table.cells, table.bolds);
            }
            if (table.cells.empty())
                throw std::runtime_error(
                    "native analysis produced an empty table; check result files and plan");
            _ran = true;
            return table;
        }

        NativeAnalysisTable table()
        {
            requireRan();
            Busy busy(*this);
            NativeAnalysisTable result;
            {
                py::gil_scoped_release release;
                std::lock_guard<std::mutex> lock(analysisMutex());
                _analyzer.tableForReport(
                    result.header, result.subheader, result.rows, result.cells, result.bolds);
            }
            return result;
        }

        std::string report(ECAnalyzer::Format format, const std::string& path)
        {
            requireRan();
            if (path.empty()) throw py::value_error("report path cannot be empty");
            const char* extension = format == ECAnalyzer::Format::Txt ? "txt"
                : format == ECAnalyzer::Format::Csv ? "csv" : "tex";
            const std::string artifact = outputPath(path, extension);
            prepareArtifact(artifact);
            Busy busy(*this);
            {
                py::gil_scoped_release release;
                std::lock_guard<std::mutex> lock(analysisMutex());
                _analyzer.report(format, path);
            }
            verifyArtifact(artifact);
            return artifact;
        }

        NativeAnalyzer& setChartStyle(
            FileOut::Palette palette, FileOut::Lang language,
            bool caption, bool logScale, bool reverseY)
        {
            requireIdle();
            FileOut::ChartStyle style;
            style.palette(palette).lang(language).caption(caption)
                .logScale(logScale).reverseY(reverseY);
            _analyzer.chartStyle(style);
            return *this;
        }

        std::vector<std::string> chart(const NativeChartOptions& options)
        {
            requireRan();
            if (options.path.empty()) throw py::value_error("chart path cannot be empty");
            if (options.grid < 2 || options.grid > 10000)
                throw py::value_error("chart grid must be between 2 and 10000");
            if (options.objectiveX < 0 || options.objectiveY < 0)
                throw py::value_error("chart objective indices must be non-negative");
            if (options.run < -1) throw py::value_error("chart run must be -1 or non-negative");
            if (!std::isfinite(options.pngFont.scale) || options.pngFont.scale <= 0.0)
                throw py::value_error("PNG scale must be positive");

            std::vector<int> selected = options.problems;
            if (selected.empty())
                for (size_t i = 0; i < _problems.size(); i++)
                    selected.push_back(static_cast<int>(i));
            std::set<int> unique;
            for (int index : selected)
            {
                if (index < 0 || index >= static_cast<int>(_problems.size()))
                    throw py::index_error("chart problem index is out of range");
                if (!unique.insert(index).second)
                    throw py::value_error("chart problem indices must be unique");
            }

            const bool perProblem = options.kind == PyChartKind::Box
                || options.kind == PyChartKind::Bar
                || options.kind == PyChartKind::Convergence
                || options.kind == PyChartKind::Pareto;
            if (perProblem && selected.size() > 1 && hasExtension(options.path))
                throw py::value_error(
                    "a multi-problem chart path must be a stem without an extension");
            if (options.kind == PyChartKind::WinTieLose
                && _comparisons.find(ECAnalyzer::Compare::WinTieLose) == _comparisons.end())
                throw py::value_error("win/tie/loss chart requires its comparison");
            if ((options.kind == PyChartKind::RankBar
                 || (options.kind == PyChartKind::RankHeatmap && !options.hasStatistic))
                && _comparisons.find(ECAnalyzer::Compare::Rank) == _comparisons.end()
                && _comparisons.find(ECAnalyzer::Compare::Nemenyi) == _comparisons.end())
                throw py::value_error("rank chart requires Rank or Nemenyi comparison");
            if (options.kind == PyChartKind::CdDiagram
                && _comparisons.find(ECAnalyzer::Compare::Nemenyi) == _comparisons.end())
                throw py::value_error("critical-difference chart requires Nemenyi comparison");

            const std::string extension = options.format == FileOut::ChartFormat::Png
                ? "png" : "svg";
            std::vector<std::string> artifacts;
            if (perProblem && selected.size() > 1)
                for (int index : selected)
                    artifacts.push_back(outputPath(
                        options.path + "_" + slug(_problems[index]), extension));
            else
                artifacts.push_back(outputPath(options.path, extension));
            for (const std::string& artifact : artifacts) prepareArtifact(artifact);

            Busy busy(*this);
            {
                py::gil_scoped_release release;
                std::lock_guard<std::mutex> lock(analysisMutex());
                drawChart(options);
            }
            for (const std::string& artifact : artifacts) verifyArtifact(artifact);
            return artifacts;
        }

        NativeAnalyzer& clearMetrics()
        {
            requireIdle();
            _analyzer.clearMetric();
            _statistics.clear();
            _percentiles.clear();
            _comparisons.clear();
            _hasReducer = false;
            _ran = false;
            return *this;
        }

        NativeAnalyzer& clearAll()
        {
            requireIdle();
            _analyzer.clearAll();
            _optimizerCount = 0;
            _problems.clear();
            _statistics.clear();
            _percentiles.clear();
            _comparisons.clear();
            _hasReducer = false;
            _ran = false;
            return *this;
        }

    private:
        class Busy
        {
        public:
            explicit Busy(NativeAnalyzer& owner) : _owner(owner) { _owner._busy = true; }
            ~Busy() { _owner._busy = false; }
        private:
            NativeAnalyzer& _owner;
        };

        void requireIdle() const
        {
            if (_busy) throw std::runtime_error("Analyzer is already active");
        }

        void requireRan() const
        {
            requireIdle();
            if (!_ran) throw std::runtime_error("call Analyzer.run() first");
        }

        template <class Builder>
        void applyProblems(Builder& builder, const NativeChartOptions& options)
        {
            if (!options.problems.empty()) builder.problems(options.problems);
        }

        void drawChart(const NativeChartOptions& options)
        {
            const chart_detail::Norm norm = chartNorm(options.normalization);
            switch (options.kind)
            {
            case PyChartKind::Box:
            {
                BoxPlotBuilder builder = _analyzer.chart().boxPlot(options.path, options.title);
                applyProblems(builder, options);
                builder.norm(norm).jitter(options.jitter);
                finishChart(builder, options);
                break;
            }
            case PyChartKind::MultiBox:
            {
                BoxPlotBuilder builder = _analyzer.chart().multiBoxPlot(options.path, options.title);
                applyProblems(builder, options);
                builder.norm(norm).jitter(options.jitter);
                finishChart(builder, options);
                break;
            }
            case PyChartKind::Bar:
            {
                BarChartBuilder builder = _analyzer.chart().barChart(options.path, options.title);
                applyProblems(builder, options);
                builder.norm(norm);
                if (options.hasStatistic) builder.statistic(options.statistic);
                if (options.hasBand) builder.band(options.bandLow, options.bandHigh);
                else builder.noBand();
                finishChart(builder, options);
                break;
            }
            case PyChartKind::MultiBar:
            {
                BarChartBuilder builder = _analyzer.chart().multiBarChart(options.path, options.title);
                applyProblems(builder, options);
                builder.norm(norm);
                if (options.hasStatistic) builder.statistic(options.statistic);
                if (options.hasBand) builder.band(options.bandLow, options.bandHigh);
                else builder.noBand();
                finishChart(builder, options);
                break;
            }
            case PyChartKind::Convergence:
            {
                ConvergenceBuilder builder = _analyzer.chart().convergence(options.path, options.title);
                applyProblems(builder, options);
                builder.grid(options.grid).swarm(options.swarm);
                if (options.hasStatistic) builder.statistic(options.statistic);
                if (options.hasBand) builder.band(options.bandLow, options.bandHigh);
                finishChart(builder, options);
                break;
            }
            case PyChartKind::Pareto:
            {
                ParetoBuilder builder = _analyzer.chart().paretoFront(options.path, options.title);
                applyProblems(builder, options);
                builder.objectives(options.objectiveX, options.objectiveY)
                    .run(options.run).connect(options.connect);
                finishChart(builder, options);
                break;
            }
            case PyChartKind::Table:
            {
                TableBuilder builder = _analyzer.chart().table(options.path, options.title);
                finishChart(builder, options);
                break;
            }
            case PyChartKind::WinTieLose:
            {
                WinTieLoseBuilder builder =
                    _analyzer.chart().winTieLose(options.path, options.title);
                finishChart(builder, options);
                break;
            }
            case PyChartKind::RankHeatmap:
            {
                HeatmapBuilder builder =
                    _analyzer.chart().rankHeatmap(options.path, options.title);
                if (options.hasStatistic) builder.statistic(options.statistic);
                finishChart(builder, options);
                break;
            }
            case PyChartKind::RankBar:
            {
                RankBarBuilder builder = _analyzer.chart().rankBar(options.path, options.title);
                finishChart(builder, options);
                break;
            }
            case PyChartKind::CdDiagram:
            {
                CdDiagramBuilder builder =
                    _analyzer.chart().cdDiagram(options.path, options.title);
                finishChart(builder, options);
                break;
            }
            }
        }

        ECAnalyzer _analyzer;
        int _optimizerCount = 0;
        std::vector<std::string> _problems;
        std::set<Stat> _statistics;
        std::set<double> _percentiles;
        std::set<ECAnalyzer::Compare> _comparisons;
        bool _hasReducer = false;
        bool _ran = false;
        bool _busy = false;
    };

    App::ReportResult buildReport(const App::ReportOptions& options)
    {
        py::gil_scoped_release release;
        std::lock_guard<std::mutex> lock(analysisMutex());
        return App::buildReport(options);
    }

    App::ReportResult discoverBatches(const std::string& path)
    {
        py::gil_scoped_release release;
        return App::discoverBatches(path);
    }

    ProcessLog readProcess(const std::string& path)
    {
        ProcessLog result;
        std::string error;
        bool ok = false;
        {
            py::gil_scoped_release release;
            ok = PrcsReader::read(path, result.header,
                [&](const PrcsReader::Record& record) { result.records.push_back(record); },
                error);
        }
        if (!ok) throw std::runtime_error(error);
        return result;
    }

    ProcessScanResult scanProcess(const std::string& path, const py::function& callback)
    {
        ProcessScanResult result;
        std::string error;
        bool ok = false;
        std::exception_ptr failure;
        {
            py::gil_scoped_release release;
            try
            {
                ok = PrcsReader::read(path, result.header,
                    [&](const PrcsReader::Record& record) {
                        py::gil_scoped_acquire acquire;
                        py::object owned = py::cast(PrcsReader::Record(record));
                        callback(owned);
                        result.records++;
                    }, error);
            }
            catch (...)
            {
                failure = std::current_exception();
            }
        }
        if (failure) std::rethrow_exception(failure);
        if (!ok) throw std::runtime_error(error);
        return result;
    }
}

void bind_analysis(py::module_& module)
{
    py::enum_<Stat>(module, "Statistic")
        .value("MEAN", Stat::Mean)
        .value("VARIANCE", Stat::Variance)
        .value("STD", Stat::Std)
        .value("MEDIAN", Stat::Median)
        .value("MODE", Stat::Mode)
        .value("SMALLEST", Stat::Smallest)
        .value("LARGEST", Stat::Largest)
        .value("Q1", Stat::Q1)
        .value("Q3", Stat::Q3)
        .value("IQR", Stat::IQR)
        .value("RANGE", Stat::Range)
        .value("SEM", Stat::SEM)
        .value("CV", Stat::CV)
        .value("GEOMETRIC_MEAN", Stat::GeometricMean)
        .value("HARMONIC_MEAN", Stat::HarmonicMean);

    py::enum_<ECAnalyzer::Indicator>(module, "Indicator")
        .value("HYPERVOLUME", ECAnalyzer::Indicator::HyperVolume)
        .value("GD", ECAnalyzer::Indicator::GD)
        .value("IGD", ECAnalyzer::Indicator::IGD)
        .value("PEAK_RATIO", ECAnalyzer::Indicator::PeakRatio)
        .value("PEAK_DISTANCE", ECAnalyzer::Indicator::PeakDistance)
        .value("PEAK_INACCURACY", ECAnalyzer::Indicator::PeakInaccuracy);

    py::enum_<ECAnalyzer::Compare>(module, "Comparison")
        .value("SIGNIFICANCE", ECAnalyzer::Compare::Significance)
        .value("RANK", ECAnalyzer::Compare::Rank)
        .value("WIN_TIE_LOSE", ECAnalyzer::Compare::WinTieLose)
        .value("BEST", ECAnalyzer::Compare::Best)
        .value("KRUSKAL_WALLIS", ECAnalyzer::Compare::KruskalWallis)
        .value("WILCOXON_SIGNED_RANK", ECAnalyzer::Compare::WilcoxonSignedRank)
        .value("FRIEDMAN", ECAnalyzer::Compare::Friedman)
        .value("NEMENYI", ECAnalyzer::Compare::Nemenyi)
        .value("DUNN", ECAnalyzer::Compare::Dunn);

    py::enum_<ECAnalyzer::Direction>(module, "Direction")
        .value("MINIMIZE", ECAnalyzer::Direction::MinIsBetter)
        .value("MAXIMIZE", ECAnalyzer::Direction::MaxIsBetter);

    py::enum_<ECAnalyzer::Format>(module, "ReportFormat")
        .value("TXT", ECAnalyzer::Format::Txt)
        .value("CSV", ECAnalyzer::Format::Csv)
        .value("LATEX", ECAnalyzer::Format::Latex);

    py::enum_<PyChartKind>(module, "ChartKind")
        .value("BOX", PyChartKind::Box)
        .value("MULTI_BOX", PyChartKind::MultiBox)
        .value("BAR", PyChartKind::Bar)
        .value("MULTI_BAR", PyChartKind::MultiBar)
        .value("CONVERGENCE", PyChartKind::Convergence)
        .value("PARETO", PyChartKind::Pareto)
        .value("TABLE", PyChartKind::Table)
        .value("WIN_TIE_LOSE", PyChartKind::WinTieLose)
        .value("RANK_HEATMAP", PyChartKind::RankHeatmap)
        .value("RANK_BAR", PyChartKind::RankBar)
        .value("CD_DIAGRAM", PyChartKind::CdDiagram);

    py::enum_<FileOut::ChartFormat>(module, "ChartFormat")
        .value("SVG", FileOut::ChartFormat::Svg)
        .value("PNG", FileOut::ChartFormat::Png);

    py::enum_<PyNormalization>(module, "Normalization")
        .value("NONE", PyNormalization::None)
        .value("MIN_MAX", PyNormalization::MinMax)
        .value("POOLED_RANK", PyNormalization::PooledRank)
        .value("RATIO_TO_BEST", PyNormalization::RatioToBest);

    py::enum_<FileOut::Palette>(module, "Palette")
        .value("COLORBLIND_SAFE", FileOut::Palette::ColorblindSafe)
        .value("CATEGORICAL", FileOut::Palette::Categorical)
        .value("GRAYSCALE", FileOut::Palette::Grayscale)
        .value("VIBRANT", FileOut::Palette::Vibrant);

    py::enum_<FileOut::Lang>(module, "ChartLanguage")
        .value("EN", FileOut::Lang::EN)
        .value("ZH", FileOut::Lang::ZH);

    py::class_<NativeAnalysisTable>(module, "_AnalysisTable")
        .def_readonly("header", &NativeAnalysisTable::header)
        .def_readonly("subheader", &NativeAnalysisTable::subheader)
        .def_readonly("rows", &NativeAnalysisTable::rows)
        .def_readonly("cells", &NativeAnalysisTable::cells)
        .def_readonly("bolds", &NativeAnalysisTable::bolds);

    py::class_<FileOut::PngFontConfig>(module, "_PngFontConfig")
        .def(py::init<>())
        .def_readwrite("latin", &FileOut::PngFontConfig::latin)
        .def_readwrite("cjk", &FileOut::PngFontConfig::cjk)
        .def_readwrite("cjk_index", &FileOut::PngFontConfig::cjkIndex)
        .def_readwrite("scale", &FileOut::PngFontConfig::scale);

    py::class_<NativeChartOptions>(module, "_ChartOptions")
        .def(py::init<>())
        .def_readwrite("kind", &NativeChartOptions::kind)
        .def_readwrite("path", &NativeChartOptions::path)
        .def_readwrite("title", &NativeChartOptions::title)
        .def_readwrite("problems", &NativeChartOptions::problems)
        .def_readwrite("normalization", &NativeChartOptions::normalization)
        .def_readwrite("jitter", &NativeChartOptions::jitter)
        .def_readwrite("has_statistic", &NativeChartOptions::hasStatistic)
        .def_readwrite("statistic", &NativeChartOptions::statistic)
        .def_readwrite("has_band", &NativeChartOptions::hasBand)
        .def_readwrite("band_low", &NativeChartOptions::bandLow)
        .def_readwrite("band_high", &NativeChartOptions::bandHigh)
        .def_readwrite("grid", &NativeChartOptions::grid)
        .def_readwrite("objective_x", &NativeChartOptions::objectiveX)
        .def_readwrite("objective_y", &NativeChartOptions::objectiveY)
        .def_readwrite("run", &NativeChartOptions::run)
        .def_readwrite("swarm", &NativeChartOptions::swarm)
        .def_readwrite("connect", &NativeChartOptions::connect)
        .def_readwrite("format", &NativeChartOptions::format)
        .def_readwrite("has_style", &NativeChartOptions::hasStyle)
        .def_readwrite("palette", &NativeChartOptions::palette)
        .def_readwrite("language", &NativeChartOptions::language)
        .def_readwrite("caption", &NativeChartOptions::caption)
        .def_readwrite("log_scale", &NativeChartOptions::logScale)
        .def_readwrite("reverse_y", &NativeChartOptions::reverseY)
        .def_readwrite("png_font", &NativeChartOptions::pngFont);

    py::class_<NativeAnalyzer>(module, "_Analyzer")
        .def(py::init<const std::string&>(), py::arg("log_root") = "_log")
        .def("add_optimizer", &NativeAnalyzer::addOptimizer,
             py::return_value_policy::reference_internal)
        .def("add_problem", &NativeAnalyzer::addProblem,
             py::return_value_policy::reference_internal)
        .def("reduce_statistic", &NativeAnalyzer::reduceStatistic,
             py::return_value_policy::reference_internal)
        .def("reduce_percentile", &NativeAnalyzer::reducePercentile,
             py::return_value_policy::reference_internal)
        .def("reduce_indicator", &NativeAnalyzer::reduceIndicator,
             py::return_value_policy::reference_internal)
        .def("add_statistic", &NativeAnalyzer::addStatistic,
             py::return_value_policy::reference_internal)
        .def("add_percentile", &NativeAnalyzer::addPercentile,
             py::return_value_policy::reference_internal)
        .def("add_comparison", &NativeAnalyzer::addComparison,
             py::return_value_policy::reference_internal)
        .def("run", &NativeAnalyzer::run)
        .def("table", &NativeAnalyzer::table)
        .def("report", &NativeAnalyzer::report)
        .def("set_chart_style", &NativeAnalyzer::setChartStyle,
             py::return_value_policy::reference_internal)
        .def("chart", &NativeAnalyzer::chart)
        .def("clear_metrics", &NativeAnalyzer::clearMetrics,
             py::return_value_policy::reference_internal)
        .def("clear_all", &NativeAnalyzer::clearAll,
             py::return_value_policy::reference_internal);

    py::class_<App::ExperimentBatch>(module, "ExperimentBatch")
        .def_readonly("tag", &App::ExperimentBatch::tag)
        .def_readonly("optimizers", &App::ExperimentBatch::optimizers)
        .def_readonly("problems", &App::ExperimentBatch::problems)
        .def_readonly("repeats", &App::ExperimentBatch::repeats)
        .def_readonly("run_numbers", &App::ExperimentBatch::runNumbers)
        .def_readonly("run_diagnostics", &App::ExperimentBatch::runDiagnostics)
        .def_readonly("result_file_count", &App::ExperimentBatch::rsltCount)
        .def_readonly("ragged", &App::ExperimentBatch::ragged);

    py::class_<App::ReportReducerSpec>(module, "_ReportReducerSpec")
        .def(py::init<>())
        .def_readwrite("kind", &App::ReportReducerSpec::kind)
        .def_readwrite("metric", &App::ReportReducerSpec::metric)
        .def_readwrite("objective", &App::ReportReducerSpec::objective)
        .def_readwrite("percentile", &App::ReportReducerSpec::percentile)
        .def_readwrite("references", &App::ReportReducerSpec::references);

    py::class_<App::ReportStatisticSpec>(module, "_ReportStatisticSpec")
        .def(py::init<>())
        .def_readwrite("metric", &App::ReportStatisticSpec::metric)
        .def_readwrite("percentile", &App::ReportStatisticSpec::percentile);

    py::class_<App::ReportComparisonSpec>(module, "_ReportComparisonSpec")
        .def(py::init<>())
        .def_readwrite("metric", &App::ReportComparisonSpec::metric)
        .def_readwrite("on", &App::ReportComparisonSpec::on)
        .def_readwrite("minimize", &App::ReportComparisonSpec::minimize);

    py::class_<App::ReportChartSpec>(module, "_ReportChartSpec")
        .def(py::init<>())
        .def_readwrite("id", &App::ReportChartSpec::id)
        .def_readwrite("kind", &App::ReportChartSpec::kind)
        .def_readwrite("problem", &App::ReportChartSpec::problem)
        .def_readwrite("statistic", &App::ReportChartSpec::statistic)
        .def_readwrite("band_low", &App::ReportChartSpec::bandLow)
        .def_readwrite("band_high", &App::ReportChartSpec::bandHigh)
        .def_readwrite("normalization", &App::ReportChartSpec::normalization)
        .def_readwrite("jitter", &App::ReportChartSpec::jitter)
        .def_readwrite("grid", &App::ReportChartSpec::grid)
        .def_readwrite("objective_x", &App::ReportChartSpec::objectiveX)
        .def_readwrite("objective_y", &App::ReportChartSpec::objectiveY)
        .def_readwrite("run", &App::ReportChartSpec::run)
        .def_readwrite("connect", &App::ReportChartSpec::connect);

    py::class_<App::ReportChartArtifact>(module, "ChartArtifact")
        .def_readonly("id", &App::ReportChartArtifact::id)
        .def_readonly("kind", &App::ReportChartArtifact::kind)
        .def_readonly("title", &App::ReportChartArtifact::title)
        .def_readonly("file", &App::ReportChartArtifact::file);

    py::class_<App::ReportOptions>(module, "_ReportOptions")
        .def(py::init<>())
        .def_readwrite("log_path", &App::ReportOptions::logPath)
        .def_readwrite("tag", &App::ReportOptions::tag)
        .def_readwrite("optimizers", &App::ReportOptions::optimizers)
        .def_readwrite("problems", &App::ReportOptions::problems)
        .def_readwrite("charts_dir", &App::ReportOptions::chartsDir)
        .def_readwrite("charts_png", &App::ReportOptions::chartsPng)
        .def_readwrite("png_latin_font", &App::ReportOptions::pngLatinFont)
        .def_readwrite("png_cjk_font", &App::ReportOptions::pngCjkFont)
        .def_readwrite("png_cjk_index", &App::ReportOptions::pngCjkIndex)
        .def_readwrite("png_scale", &App::ReportOptions::pngScale)
        .def_readwrite("custom_analysis", &App::ReportOptions::customAnalysis)
        .def_readwrite("reducer", &App::ReportOptions::reducer)
        .def_readwrite("statistics", &App::ReportOptions::statistics)
        .def_readwrite("comparisons", &App::ReportOptions::comparisons)
        .def_readwrite("charts", &App::ReportOptions::charts)
        .def_readwrite("baseline_optimizer", &App::ReportOptions::baselineOptimizer);

    py::class_<App::ReportResult>(module, "_ReportResult")
        .def_readonly("ok", &App::ReportResult::ok)
        .def_readonly("error", &App::ReportResult::error)
        .def_readonly("warnings", &App::ReportResult::warnings)
        .def_readonly("batches", &App::ReportResult::batches)
        .def_readonly("chosen", &App::ReportResult::chosen)
        .def_readonly("header", &App::ReportResult::header)
        .def_readonly("subheader", &App::ReportResult::subheader)
        .def_readonly("rows", &App::ReportResult::rows)
        .def_readonly("cells", &App::ReportResult::cells)
        .def_readonly("chart_files", &App::ReportResult::chartFiles)
        .def_readonly("chart_artifacts", &App::ReportResult::chartArtifacts);

    py::class_<PrcsReader::Header>(module, "ProcessHeader")
        .def_readonly("version", &PrcsReader::Header::version)
        .def_readonly("optimizer", &PrcsReader::Header::optimizer)
        .def_readonly("tag", &PrcsReader::Header::tag)
        .def_readonly("problem", &PrcsReader::Header::problem)
        .def_readonly("run", &PrcsReader::Header::run)
        .def_readonly("full_process", &PrcsReader::Header::full_process);

    py::class_<PrcsReader::Record>(module, "ProcessRecord")
        .def_readonly("fes", &PrcsReader::Record::fes)
        .def_readonly("swarm", &PrcsReader::Record::swarm)
        .def_readonly("best_count", &PrcsReader::Record::nbest)
        .def_readonly("population_count", &PrcsReader::Record::npop)
        .def_readonly("best", &PrcsReader::Record::best)
        .def_readonly("population", &PrcsReader::Record::pop);

    py::class_<ProcessLog>(module, "ProcessLog")
        .def_readonly("header", &ProcessLog::header)
        .def_readonly("records", &ProcessLog::records);

    py::class_<ProcessScanResult>(module, "ProcessScanResult")
        .def_readonly("header", &ProcessScanResult::header)
        .def_readonly("record_count", &ProcessScanResult::records);

    module.def("_discover_batches", &discoverBatches, py::arg("log_path"));
    module.def("_build_report", &buildReport, py::arg("options"));
    module.def("_read_process", &readProcess, py::arg("path"));
    module.def("_scan_process", &scanProcess, py::arg("path"), py::arg("callback"));
}
