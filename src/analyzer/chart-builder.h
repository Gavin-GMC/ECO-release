//------------------------Description------------------------
// 图表流式接口。每个图种【自暴露一个方法】,方法返回该图种【专属的构建器】,
// 构建器上只有对它有效的选项 ⇒ 传错参数 = 编译不过。
//   az.chart().boxPlot("box", "分布").jitter(true).draw();
//   az.chart().convergence("conv", "收敛").grid(400).band(Q1, Q3).draw();
// 与项目既有惯用法同构(config-setter.h 的 WorkflowSetter:wf.strategy.velocityDriven(...))。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <string>
#include <vector>
#include "out-chart.h"
#include "out-png.h"
#include "analysis-enums.h"

namespace EvoFlow
{
    class ECAnalyzer;

    namespace chart_detail
    {
        // 图种(内部)。公开面上由 ChartMaker 的具名方法替代。
        enum class ChartKind {
            CdDiagram, BoxPlot, MultiBoxPlot, BarChart, MultiBarChart,
            Table, WinTieLose, RankHeatmap, Convergence, ParetoFront, RankBar
        };

    }

    // 图表输出格式:定义下沉到 out-chart.h(与 ChartStyle 同族、且 draw*() 需在此之前用到);
    //   此别名令用户可写 `ChartFormat::Png`(EvoFlow 命名空间内)。
    //   二选一而非标志位(用户定):"两个都要"由 draw() 返回自身的改参重画机制表达 ——
    //     b.draw();  b.format(ChartFormat::Png).draw();
    using ChartFormat = FileOut::ChartFormat;

    namespace chart_detail
    {
        // 展示加工口径(默认【原始数据】,一切加工均为可选)。变换在 analyzer 内做,SVG 层零计算。
        enum class Norm { None, MinMax, PooledRank, RatioToBest };

        // 内部参数载体:各构建器把自己收到的选项填进来,统一交给 analyzer 分派。
        // 用户【不】直接构造它 —— 那正是本次重构要消除的东西。
        struct ChartSpec
        {
            Norm norm = Norm::None;
            bool jitter = false;
            std::vector<int> problems;          // 空 = 全部
            bool hasStat = false;  int statQ = -1;      // statQ = _Q 的整数值,由 analyzer 解释
            bool hasBand = false;  int bandLoQ = -1, bandHiQ = -1;
            int  grid = 200;
            int  objX = 0, objY = 1;
            int  run = -1;                      // -1 = 全部运行叠加
            std::string swarm = "";             // v2.5.3:收敛曲线取源级别——空=种群级(全局 _<run>.prcs);非空=该子群级(_<run>_<id>.prcs)
            bool connect = false;
            int  format = 0;                    // 0=Svg 1=Png(取值同 ChartFormat)
            FileOut::PngFontConfig pngFont;     // 仅 Png 用
        };
    }

    // ---- CRTP 基类:共有项。返回 Self& ⇒ 与专属选项可任意顺序链式 ----
    template <class Self>
    class ChartBase
    {
    public:
        ChartBase(ECAnalyzer* az, chart_detail::ChartKind kind, std::string path, std::string title)
            : _az(az), _kind(kind), _path(std::move(path)), _title(std::move(title)) {}

        // 路径/标题可改 ⇒ 同一构建器改参重画时不必重建
        Self& path(std::string p)  { _path = std::move(p); return self(); }
        Self& title(std::string t) { _title = std::move(t); return self(); }
        // 逐图覆盖样式;不设则用 analyzer 的 chartStyle()
        Self& style(const FileOut::ChartStyle& s) { _style = s; _hasStyle = true; return self(); }

        // 输出格式:默认 Svg。二选一 —— 要两种就 draw() 两次(见本文件头 draw() 说明)。
        Self& format(ChartFormat f) { _spec.format = (int)f; return self(); }
        // PNG 字体/倍率。字体是【运行期从系统加载】的商业字体,不随项目分发。
        Self& pngFont(const FileOut::PngFontConfig& f) { _spec.pngFont = f; return self(); }

        // 出图。返回自身 ⇒ 可继续改参再 draw()。
        Self& draw();

    protected:
        Self& self() { return static_cast<Self&>(*this); }
        ECAnalyzer*               _az;
        chart_detail::ChartKind   _kind;
        std::string               _path, _title;
        chart_detail::ChartSpec   _spec;
        FileOut::ChartStyle       _style;
        bool                      _hasStyle = false;
    };

    // 逐问题类共有:选问题(不选 = 每问题各出一张,文件名自动加问题后缀)
    template <class Self>
    class PerProblemChart : public ChartBase<Self>
    {
    public:
        using ChartBase<Self>::ChartBase;
        Self& problem(int i)                { this->_spec.problems.assign(1, i); return this->self(); }
        Self& problems(std::vector<int> v)  { this->_spec.problems = std::move(v); return this->self(); }
    };

    // ============ 各图种专属构建器:只暴露对自己有效的选项 ============

    class BoxPlotBuilder : public PerProblemChart<BoxPlotBuilder>
    {
    public:
        using PerProblemChart<BoxPlotBuilder>::PerProblemChart;
        BoxPlotBuilder& jitter(bool on = true) { _spec.jitter = on; return *this; }   // 仅箱线图
        BoxPlotBuilder& norm(chart_detail::Norm n) { _spec.norm = n; return *this; }
    };

    class BarChartBuilder : public PerProblemChart<BarChartBuilder>
    {
    public:
        using PerProblemChart<BarChartBuilder>::PerProblemChart;
        BarChartBuilder& norm(chart_detail::Norm n) { _spec.norm = n; return *this; }
        BarChartBuilder& statistic(Stat s) { _spec.hasStat = true; _spec.statQ = (int)s; return *this; }
        BarChartBuilder& band(Stat lo, Stat hi)
        { _spec.hasBand = true; _spec.bandLoQ = (int)lo; _spec.bandHiQ = (int)hi; return *this; }
        BarChartBuilder& noBand()          { _spec.hasBand = false; return *this; }
    };

    class ConvergenceBuilder : public PerProblemChart<ConvergenceBuilder>
    {
    public:
        using PerProblemChart<ConvergenceBuilder>::PerProblemChart;
        ConvergenceBuilder& grid(int n)    { _spec.grid = (n > 1 ? n : 2); return *this; }   // 仅收敛曲线
        ConvergenceBuilder& statistic(Stat s) { _spec.hasStat = true; _spec.statQ = (int)s; return *this; }
        ConvergenceBuilder& band(Stat lo, Stat hi)
        { _spec.hasBand = true; _spec.bandLoQ = (int)lo; _spec.bandHiQ = (int)hi; return *this; }
        // v2.5.3 LOG-PRCS-MULTIPOP:显式选取源级别。默认(不调)=【种群级】,读全局 `_<run>.prcs`
        //   (逐 epoch、统一 FES)。调 `.swarm("s0")`=【子群级】,读该子群 `_<run>_s0.prcs`(逐代细粒度)。
        ConvergenceBuilder& swarm(std::string id) { _spec.swarm = std::move(id); return *this; }
    };

    class ParetoBuilder : public PerProblemChart<ParetoBuilder>
    {
    public:
        using PerProblemChart<ParetoBuilder>::PerProblemChart;
        ParetoBuilder& objectives(int xi, int yi) { _spec.objX = xi; _spec.objY = yi; return *this; }  // 仅 Pareto
        ParetoBuilder& run(int i)          { _spec.run = i; return *this; }
        ParetoBuilder& connect(bool on = true) { _spec.connect = on; return *this; }
    };

    class HeatmapBuilder : public ChartBase<HeatmapBuilder>
    {
    public:
        using ChartBase<HeatmapBuilder>::ChartBase;
        HeatmapBuilder& statistic(Stat s)  { _spec.hasStat = true; _spec.statQ = (int)s; return *this; }
    };

    // 无专属选项者:只有共有项(path/title/style)+ draw()
    class TableBuilder      : public ChartBase<TableBuilder>      { public: using ChartBase<TableBuilder>::ChartBase; };
    class WinTieLoseBuilder : public ChartBase<WinTieLoseBuilder> { public: using ChartBase<WinTieLoseBuilder>::ChartBase; };
    class RankBarBuilder    : public ChartBase<RankBarBuilder>    { public: using ChartBase<RankBarBuilder>::ChartBase; };
    class CdDiagramBuilder  : public ChartBase<CdDiagramBuilder>  { public: using ChartBase<CdDiagramBuilder>::ChartBase; };

    // ============ 门面:az.chart() 返回它 ============
    //   注:本对象是临时的 —— 构建器持 ECAnalyzer* 而非它的引用,故
    //   `auto b = az.chart().boxPlot(...);` 跨语句持有安全。
    class ChartMaker
    {
    public:
        explicit ChartMaker(ECAnalyzer* az) : _az(az) {}

        // 逐问题类(不指定问题则每问题各出一张)
        BoxPlotBuilder     boxPlot(std::string path, std::string title)
        { return BoxPlotBuilder(_az, chart_detail::ChartKind::BoxPlot, std::move(path), std::move(title)); }
        BarChartBuilder    barChart(std::string path, std::string title)
        { return BarChartBuilder(_az, chart_detail::ChartKind::BarChart, std::move(path), std::move(title)); }
        ConvergenceBuilder convergence(std::string path, std::string title)
        { return ConvergenceBuilder(_az, chart_detail::ChartKind::Convergence, std::move(path), std::move(title)); }
        ParetoBuilder      paretoFront(std::string path, std::string title)
        { return ParetoBuilder(_az, chart_detail::ChartKind::ParetoFront, std::move(path), std::move(title)); }

        // 跨问题类(一图涵盖全部问题)
        BoxPlotBuilder     multiBoxPlot(std::string path, std::string title)
        { return BoxPlotBuilder(_az, chart_detail::ChartKind::MultiBoxPlot, std::move(path), std::move(title)); }
        BarChartBuilder    multiBarChart(std::string path, std::string title)
        { return BarChartBuilder(_az, chart_detail::ChartKind::MultiBarChart, std::move(path), std::move(title)); }
        HeatmapBuilder     rankHeatmap(std::string path, std::string title)
        { return HeatmapBuilder(_az, chart_detail::ChartKind::RankHeatmap, std::move(path), std::move(title)); }
        TableBuilder       table(std::string path, std::string title)
        { return TableBuilder(_az, chart_detail::ChartKind::Table, std::move(path), std::move(title)); }
        WinTieLoseBuilder  winTieLose(std::string path, std::string title)
        { return WinTieLoseBuilder(_az, chart_detail::ChartKind::WinTieLose, std::move(path), std::move(title)); }
        RankBarBuilder     rankBar(std::string path, std::string title)
        { return RankBarBuilder(_az, chart_detail::ChartKind::RankBar, std::move(path), std::move(title)); }
        CdDiagramBuilder   cdDiagram(std::string path, std::string title)
        { return CdDiagramBuilder(_az, chart_detail::ChartKind::CdDiagram, std::move(path), std::move(title)); }

    private:
        ECAnalyzer* _az;
    };
}
