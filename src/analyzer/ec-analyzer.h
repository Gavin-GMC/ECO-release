// ===========================================================================
//  ec-analyzer.h  --  performance analysis / evaluation layer of EvoFlow
//  Migrated & refactored in v4 from the stable ECAnalyzer.
//
//  Purpose: aggregate the result logs (.rslt) produced by Optimizer::logResult
//  into a comparison table over  (optimizer x problem)  cells, each cell the
//  summary of `repeats` independent runs.
//
//  Three-stage pipeline (see docs 手册/2X-开发-分析评估.md):
//    (1) reduce    : one run's solution set  -> one scalar per run
//                    - descriptive  reduceBy(Stat, objIndex)   (over solutions)
//                    - set-quality  reduceBy(Indicator, refs)  (HV / GD / IGD)
//    (2) aggregate : the `repeats` scalars   -> table columns
//                    - addStatistic(Stat)     (mean / std / best ...)
//    (3) compare   : across optimizers        -> extra columns / rows
//                    - addComparison(Compare, on, Direction)
//  then  run()  and  report(Format, path).
//
//  Five public enums (behaviour-named, decoupled from any one algorithm):
//    Stat       cross-stage scalar op, used by BOTH reduceBy and addStatistic
//    Indicator  reduce-only set-quality metric (HV / GD / IGD / PeakRatio)
//    Compare    cross-optimizer statistic (Significance / Rank / WinTieLose / Best)
//    Direction  MinIsBetter / MaxIsBetter (objective sense for Compare)
//    Format     output format (Txt / Csv / Latex functional; Excel/Word deferred)
// ===========================================================================

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <cmath>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <limits>
#include <algorithm>
#include <map>
#include <tuple>
#include <cctype>
#include <utility>
#include "metriclib.h"
#include "logger.hpp"
#include "prcs-reader.h"
#include "out-txt.h"
#include "out-latex.h"
#include "out-chart.h"
#include "chart-builder.h"

namespace EvoFlow
{
    class ECAnalyzer
    {
    public:
        // cross-stage scalar operator (reduce over solutions OR aggregate over repeats)
        //   v2.2.2.6:定义下沉到 analysis-enums.h(chart-builder 需在 ECAnalyzer 完整前命名它);
        //   此别名令既有 `ECAnalyzer::Stat::Median` 写法全部不受影响。
        using Stat = EvoFlow::Stat;
        // reduce-only set-quality indicator (needs reference point / front)
        // reduce-only set-quality metrics. HV/GD/IGD 多目标;PeakRatio/Distance/Inaccuracy 多峰(需 logging(full result))。
        enum class Indicator { HyperVolume, GD, IGD, PeakRatio, PeakDistance, PeakInaccuracy };
        // cross-optimizer comparison
        enum class Compare { Significance, Rank, WinTieLose, Best,
                             KruskalWallis, WilcoxonSignedRank, Friedman, Nemenyi, Dunn };   // 非参统计检验套件(v2.2.1.1)
        // objective sense for comparisons
        enum class Direction { MinIsBetter, MaxIsBetter };
        // output format
        //   v2.2.3:去掉 Png —— 表格出位图已由图表侧 `chart().table(...).format(ChartFormat::Png)` 覆盖,
        //   此处再留一个空桩即纯冗余。Excel/Word 仍未实现(ANALYSIS-OUT)。
        enum class Format { Txt, Csv, Latex, Excel, Word };
        // 图表类型(SVG 输出,v2.2.2)。样式经 chartStyle() 设一次全批共用;标题为每图参数。
        //   逐问题类(BoxPlot/BarChart):默认【一图一问题】,不指定问题则每问题各出一张,
        //     文件名自动加问题后缀;经 ChartSpec::problem(s) 收窄。
        //   跨问题类(MultiBoxPlot/MultiBarChart/CdDiagram):一图涵盖全部问题。
        //   Table/WinTieLose/RankHeatmap 亦为跨问题类,且【只吃 _analysis_buffer】,零新统计。
        //   Convergence 属【逐问题】类(默认一图一问题),吃 .prcs 轨迹,需 logging(_, true)。
    private:
        // v2.2.2.6:以下三者【降为内部实现细节】—— 公开面由 chart-builder.h 的流式接口取代。
        //   (未用嵌套 private 而是配合 chart_detail 命名空间,见 chart-builder.h 的循环依赖说明)
        //   ParetoFront 为逐问题类(需 ≥2 目标);RankBar 为跨问题类(需 Compare::Rank 或 Nemenyi)。
        enum class Chart { CdDiagram, BoxPlot, MultiBoxPlot, BarChart, MultiBarChart,
                           Table, WinTieLose, RankHeatmap, Convergence, ParetoFront, RankBar };

        // 展示加工口径(用户定:默认【原始数据】,一切加工均为可选)。
        //   变换【在 analyzer 内做】、结果落 _chart_scratch,SVG 层只收算好的数。
        enum class Norm {
            None,          // 默认:原样,不做任何变换
            MinMax,        // 组内按该问题跨算法 min/max 压到 [0,1](柱状图下最优项会退化为 0 高度)
            PooledRank,    // 组内把 K*R 个 run 混合排秩 1..K*R(并列取平均秩)
            RatioToBest    // 组内除以该问题最优值(>=1,最优=1);柱状图跨问题时最实用
        };

        // 每图的【取数】参数(与跨图共享的 ChartStyle 正交)。
        //   注:logScale 归 ChartStyle —— 它只改轴刻度、不动数据,与 norm 分属展示/数据两侧。
        class ChartSpec
        {
        public:
            ChartSpec& norm(Norm n)            { _norm = n; return *this; }
            ChartSpec& jitter(bool on)         { _jit  = on; return *this; }   // 默认关(用户定)
            ChartSpec& problem(int idx)        { _probs.assign(1, idx); return *this; }
            ChartSpec& problems(std::vector<int> v) { _probs = std::move(v); return *this; }
            ChartSpec& statistic(Stat s)       { _stat = s; _hasStat = true; return *this; }
            ChartSpec& band(Stat lo, Stat hi)  { _lo = lo; _hi = hi; _hasBand = true; return *this; }
            ChartSpec& noBand()                { _hasBand = false; return *this; }
            ChartSpec& grid(int n)             { _grid = (n > 1 ? n : 2); return *this; }  // 收敛曲线 FES 网格点数
            int  getGrid() const { return _grid; }
            ChartSpec& swarm(std::string id)   { _swarm = std::move(id); return *this; }  // v2.5.3:收敛取源级别(空=种群级/全局)
            const std::string& getSwarm() const { return _swarm; }
            ChartSpec& format(int f) { _format = f; return *this; }
            ChartSpec& pngFont(const FileOut::PngFontConfig& f) { _pngFont = f; return *this; }
            int  getFormat() const { return _format; }
            const FileOut::PngFontConfig& getPngFont() const { return _pngFont; }
            // —— 以下三项仅 Pareto 前沿用 ——
            ChartSpec& objectives(int xi, int yi) { _ox = xi; _oy = yi; return *this; }   // 目标数 >2 时选两个目标轴
            ChartSpec& run(int i)              { _run = i; return *this; }               // 收窄到单次运行;-1 = 全部叠加
            ChartSpec& connect(bool on)        { _conn = on; return *this; }             // 连线,默认关
            int  objX() const { return _ox; }
            int  objY() const { return _oy; }
            int  getRun() const { return _run; }
            bool hasConnect() const { return _conn; }
            Norm getNorm()  const { return _norm; }
            bool hasJitter()const { return _jit;  }
            const std::vector<int>& getProblems() const { return _probs; }   // 空 = 全部
            bool hasStat()  const { return _hasStat; }
            Stat getStat()  const { return _stat; }
            bool hasBand()  const { return _hasBand; }
            Stat bandLo()   const { return _lo; }
            Stat bandHi()   const { return _hi; }
        private:
            Norm _norm = Norm::None;      // 默认原始数据
            bool _jit  = false;           // 默认不叠散点层
            std::vector<int> _probs;      // 空 = 全部问题
            bool _hasStat = false; Stat _stat = Stat::Median;
            std::string _swarm = "";      // v2.5.3:收敛轨迹取源级别(空=种群级/全局 _<run>.prcs;非空=子群级)
            // 误差棒默认关(用户定,与"一切展示加工均为可选"一致);band(lo,hi) 开,noBand() 显式关
            bool _hasBand = false; Stat _lo = Stat::Q1, _hi = Stat::Q3;
            int  _grid = 200;             // 网格越密越平滑,内存 O(网格 × run)
            int  _format = 0;             // v2.2.3:0=Svg 1=Png
            FileOut::PngFontConfig _pngFont;
            int  _ox = 0, _oy = 1;        // Pareto:默认取前两个目标
            int  _run = -1;               // Pareto:-1 = 全部 run 叠加
            bool _conn = false;           // Pareto:默认只画点(连线会造出前沿上不存在的中间段)
        };
    public:

    private:
        // internal unified quantity tag (Stat + Indicator), drives the machinery
        enum class _Q { mean, variance, std_, median, mode, smallest, largest,
                        q1, q3, iqr, range_, sem, cv, geo_mean, harm_mean, percentile,
                        hyper_volume, gd, igd, peak_ratio, peak_distance, peak_inaccuracy, none };

        // 跨算法统计(Compare)在 _analysis_buffer 每格 stat 列【之后】的【具名额外槽】——取代散落魔数偏移 `+N`。
        // 用法:_analysis_buffer[p][o][_analysis.size() + EX_XXX]。EX_COUNT = 额外槽总数(取代旧 `+15` 硬上限)。
        enum ExtraCol {
            EX_SIG_P = 0,                          // Significance p 值(层1·成对 vs 基线)
            EX_WL_WIN, EX_WL_LOSE, EX_WL_TIE,      // WinTieLose 胜/负/平(相对 EX_WL_WIN 的 +0/+1/+2)
            EX_BEST,                               // Best 最优标记
            EX_RANK, EX_AVGRANK,                   // 名次 / 平均秩
            EX_KW_P,                               // Kruskal-Wallis p(层1·每问题综合,存 [p][0])
            EX_WSR_P,                              // Wilcoxon 符号秩 p(层2·汇总,存 [0][o])
            EX_FRIEDMAN_CHI, EX_FRIEDMAN_P,        // Friedman χ² / p(层2·汇总,存 [0][0])
            EX_NEMENYI_CD, EX_NEMENYI_AVGRANK,     // Nemenyi CD([0][0]) / 平均秩([0][o])
            EX_DUNN_P,                             // Dunn p(层1·后验,存 [p][o])
            EX_COUNT                               // 额外槽总数(_analysis_buffer 尾部预留 = 此值)
        };

        static _Q _toQ(Stat s)
        {
            switch (s)
            {
            case Stat::Mean:     return _Q::mean;
            case Stat::Variance: return _Q::variance;
            case Stat::Std:      return _Q::std_;
            case Stat::Median:   return _Q::median;
            case Stat::Mode:     return _Q::mode;
            case Stat::Smallest: return _Q::smallest;
            case Stat::Largest:  return _Q::largest;
            case Stat::Q1:            return _Q::q1;
            case Stat::Q3:            return _Q::q3;
            case Stat::IQR:           return _Q::iqr;
            case Stat::Range:         return _Q::range_;
            case Stat::SEM:           return _Q::sem;
            case Stat::CV:            return _Q::cv;
            case Stat::GeometricMean: return _Q::geo_mean;
            case Stat::HarmonicMean:  return _Q::harm_mean;
            }
            return _Q::none;
        }
        static _Q _toQ(Indicator i)
        {
            switch (i)
            {
            case Indicator::HyperVolume: return _Q::hyper_volume;
            case Indicator::GD:          return _Q::gd;
            case Indicator::IGD:         return _Q::igd;
            case Indicator::PeakRatio:      return _Q::peak_ratio;
            case Indicator::PeakDistance:   return _Q::peak_distance;
            case Indicator::PeakInaccuracy: return _Q::peak_inaccuracy;
            }
            return _Q::none;
        }

        struct PartiInfo
        {
            std::string optimizer;
            std::string tag;
            int repeats;
            std::vector<int> runs;
            PartiInfo(std::string o, std::string t, int r)
                : optimizer(std::move(o)), tag(std::move(t)), repeats(r)
            {
                for (int i = 0; i < r; ++i) runs.push_back(i);
            }
            PartiInfo(std::string o, std::string t, std::vector<int> selectedRuns)
                : optimizer(std::move(o)), tag(std::move(t)),
                  repeats(static_cast<int>(selectedRuns.size())),
                  runs(std::move(selectedRuns)) {}
        };

        // ============================================================================
        // 指标↔参数 绑定:有意保留【两套并存】的参数通道,因两类参数语义不同——不强行统一
        // (统一为 params[条目][问题] 会把"逐问题"与"逐条目"两种语义混进一个结构,反致不清晰):
        //   ① _ref            —— 逐【问题】(_ref[p]):Indicator 的参考数据,随问题变
        //                        (HV 参考点 / GD·IGD 参考前沿 / PeakRatio {f*,峰数,ε,r} / Peak* 峰坐标)。
        //                        仅归约维、单指标(reduceBy(Indicator,refPerProblem) 由调用方逐问题给)。
        //   ② _reduce_params  —— 归约维【单指标】的参数组,问题无关(如 reduceByPercentile 的 {p})。
        //   ③ _analysis_params—— 逐【条目】(_analysis_params[m]):聚合维每个指标列的参数组,问题无关
        //                        (与 _analysis 位置对齐;percentile 列={p},无参列={})。
        // 归纳:参考数据按"问题"分(_ref)、配置参数按"指标条目"分(_reduce_params/_analysis_params);
        //      每个"参数组"本身是 vector<double>,故单个指标可含多参数。
        // ============================================================================

        // --- stage 1: reduction ---
        _Q _reduce = _Q::none;
        std::vector<double> _reduce_params;              // ② 归约指标参数组(问题无关;如 percentile={p})
        bool _reduce_is_indicator = false;
        int _obj_index = 0;                              // objective column (descriptive reduce)
        std::string _trace_swarm = "";                   // v2.5.3:收敛轨迹取源级别(空=种群级全局;非空=该子群级)
        std::vector<std::vector<double>> _ref;           // ① 逐问题参考数据(indicator;_ref[p])

        // --- stage 2: aggregation columns ---
        std::vector<_Q> _analysis;
        std::vector<std::vector<double>> _analysis_params;   // ③ 与 _analysis 平行:每列指标参数组(percentile 列={p};无参列={})

        // --- stage 3: comparisons ---
        bool _significance_test = false;
        bool _rank_stat = false;   _Q _q_rank = _Q::none;  bool _min_rank = true;
        bool _wl_stat = false;     _Q _q_wl = _Q::none;    bool _min_wl = true;
        bool _best_stat = false;   _Q _q_best = _Q::none;  bool _min_best = true;
        bool _kw_test = false;                             // Kruskal-Wallis(层1·独立·每问题综合,→ M+7 on [p][0])
        bool _wsr_test = false;    _Q _q_wsr = _Q::none;   // Wilcoxon 符号秩(层2·配对·跨问题,vs 基线,→ M+8 on [0][o])
        bool _friedman_test = false; _Q _q_friedman = _Q::none;   // Friedman(层2·K 算法综合排名,→ χ² M+9 / p M+10 on [0][0])
        bool _nemenyi_test = false;  _Q _q_nemenyi = _Q::none;    // Nemenyi(层2·Friedman 后验,→ CD M+11 on [0][0] / 平均秩 M+12 on [0][o])
        bool _dunn_test = false;                                  // Dunn(层1·KW 后验,每问题 baseline vs 各,→ M+13 on [p][o])

        // --- 图表(SVG)样式:设一次,本 analyzer 所有图共用 ⇒ 同批跨图色表一致、同算法同色 ---
        FileOut::ChartStyle _chart_style;
        std::string _chart_ext = "svg";                                  // v2.2.3:本次出图的扩展名
        FileOut::ChartFormat _chart_fmt = FileOut::ChartFormat::Svg;     //   与之配套的格式(显式透传给 draw*)
        std::string _log_root = "_log";                                  // 结果/过程日志根目录;默认兼容既有布局

        // --- 图表【按需补算】通道(用户定的边界) ---
        //   图需要的统计量未被用户 addStatistic 声明时,不能不算,也【不能污染用户的统计空间】:
        //   → 用与 addStatistic 完全相同的底层算法(_applyStat)从 _result_buffer 现算,
        //     结果只进本 scratch(额外内存、画完即清),【绝不写 _analysis_buffer】;
        //   → SVG 层自身不做任何计算,只接收算好的数。
        //   key = (problem, optimizer, _Q)
        mutable std::map<std::tuple<int, int, int>, double> _chart_scratch;

        std::vector<std::string> _problems;
        std::vector<PartiInfo> _infos;

        // [problem][optimizer][repeat][solution][objective]
        std::vector<std::vector<std::vector<std::vector<std::vector<double>>>>> _data_buffer;
        // [problem][optimizer][repeat][solution][decision-dim] (captured from .rslt ver 1.1,
        //   i.e. logging(full result); needed by decision-space indicators like PeakRatio)
        std::vector<std::vector<std::vector<std::vector<std::vector<double>>>>> _decision_buffer;
        // [problem][optimizer][repeat] : per-run reduced scalar
        std::vector<std::vector<std::vector<double>>> _result_buffer;
        // [problem][optimizer][column] : aggregated / comparison values
        std::vector<std::vector<std::vector<double>>> _analysis_buffer;

        void clearResult() { _result_buffer.clear(); _analysis_buffer.clear(); }
        void clearData() { _data_buffer.clear(); _decision_buffer.clear(); clearResult(); }

        // ---- result-file path (matches Optimizer/Logger layout, see logger.hpp) ----
        std::string buildPath(std::string problem, std::string optimizer, std::string tag, int repeat_id,
                              const char* ext = "rslt")
        {
            const std::string separator =
                (!_log_root.empty() && (_log_root.back() == '/' || _log_root.back() == '\\')) ? "" : "/";
            return _log_root + separator + optimizer + "/" + problem + "/" + optimizer + "_" + problem
                + "(" + tag + ")_" + std::to_string(repeat_id) + "." + ext;
        }

        // ===== 轨迹(.prcs)→ FES 网格,供收敛曲线用(v2.2.2.4) =====
        //   【流式】两遍扫描:①只求 FES 上界,②边读边重采样。峰值内存 O(网格点 × run 数),
        //   与代数无关 —— 轨迹量 = 代数 × 种群规模,全量驻留会炸。
        //   【不做任何"修正"】不 cummin/cummax、不假定优化方向:best 的单调方向由问题的
        //   比较器决定(addObjective 的 min_is_better;外部 benchmark 也可声明最大化),方向属消费方的事。

        // 一条记录的 best 集 → 单标量(复用归约维已声明的算子;nbest=1 时任何 Stat 同值)
        double _traceScalar(const std::vector<std::vector<double>>& best) const
        {
            std::vector<double> v;
            for (size_t i = 0; i < best.size(); i++)
                if (_obj_index >= 0 && _obj_index < (int)best[i].size()) v.push_back(best[i][_obj_index]);
            if (v.empty()) return NAN;
            if (_reduce == _Q::none) return v[0];
            return _applyStat(_reduce, v.data(), (int)v.size());
        }

        // v2.5.3 LOG-PRCS-MULTIPOP:收敛轨迹文件路径。_trace_swarm 空=种群级(全局 _<run>.prcs);
        //   非空=子群级(_<run>_<id>.prcs,把 _<id> 插在 .prcs 前)。级别由收敛图 `.swarm(id)` 显式指定。
        std::string _tracePath(int p, int o, int r)
        {
            std::string base = buildPath(_problems[p], _infos[o].optimizer,
                _infos[o].tag, _infos[o].runs[r], "prcs");
            if (_trace_swarm.empty()) return base;
            return base.substr(0, base.size() - 5) + "_" + _trace_swarm + ".prcs";   // 去尾 ".prcs" 再插 _<id>
        }

        // 第①遍:该问题下跨所有算法所有 run 的 FES 上界(建公共网格 ⇒ 曲线可比)
        long long _traceMaxFES(int p, std::string& err)
        {
            long long hi = 0;
            for (size_t o = 0; o < _infos.size(); o++)
                for (int r = 0; r < _infos[o].repeats; r++)
                {
                    PrcsReader::Header h; std::string e;
                    long long localHi = 0;
                    if (!PrcsReader::read(_tracePath(p, o, r),
                                          h, [&](const PrcsReader::Record& rec) { localHi = rec.fes; }, e))
                    { if (err.empty()) err = e; continue; }
                    if (localHi > hi) hi = localHi;
                }
            return hi;
        }

        // 第②遍:把 (p,o) 的每个 run 重采样到 grid → curve[run][网格点]
        //   阶梯保持(取最近前值):best-so-far 单调 ⇒ 这是唯一不引入伪信息的插值。
        //   首条记录之前 → NaN(不画)。
        bool _resampleRuns(int p, int o, const std::vector<double>& grid,
                           std::vector<std::vector<double>>& curve, std::string& err)
        {
            int G = (int)grid.size(), R = _infos[o].repeats;
            curve.assign(R, std::vector<double>(G, NAN));
            bool any = false;
            for (int r = 0; r < R; r++)
            {
                PrcsReader::Header h; std::string e;
                int k = 0; double last = NAN;
                bool ok = PrcsReader::read(_tracePath(p, o, r),
                    h, [&](const PrcsReader::Record& rec) {
                        while (k < G && grid[k] < (double)rec.fes) { curve[r][k] = last; k++; }
                        last = _traceScalar(rec.best);
                    }, e);
                if (!ok) { if (err.empty()) err = e; continue; }
                while (k < G) { curve[r][k] = last; k++; }
                any = true;
            }
            return any;
        }

        // Parse one .rslt file into buffer[solution][objective]. Format written by
        // Optimizer::logResult: "<ver= 1.x >" / Exe_Time / ---- / Object_Number \t N
        // \t Solution_Number \t M / then M lines "i:\t <name value>*N [full-soln (1.1)]".
        // `buffer`   <- [solution][objective]     (always)
        // `decision` <- [solution][decision-dim]  (ver 1.1 only; empty rows for ver 1.0)
        bool loadResult(std::vector<std::vector<double>>& buffer,
            std::vector<std::vector<double>>& decision, std::string file_path)
        {
            std::ifstream data_file(file_path);
            std::string read_buffer, version_tag;
            int object_number, solution_number;

            buffer.clear();
            decision.clear();
            if (!data_file.is_open())
            {
                sys_logger.error("The result data is missing or damaged, when processing " + file_path);
                return false;
            }

            data_file >> read_buffer >> version_tag >> read_buffer;   // <ver= 1.x >
            if (version_tag != "1.0" && version_tag != "1.1")
            {
                sys_logger.error("Unsupported result version '" + version_tag + "' in " + file_path);
                return false;
            }

            std::getline(data_file, read_buffer);   // rest of ver line
            std::getline(data_file, read_buffer);   // Exe_Time line
            std::getline(data_file, read_buffer);   // separator

            data_file >> read_buffer >> object_number >> read_buffer >> solution_number;
            buffer.resize(solution_number);
            decision.resize(solution_number);
            for (int i = 0; i < solution_number; i++)
            {
                buffer[i].resize(object_number);
                data_file >> read_buffer;            // "i:"
                for (int j = 0; j < object_number; j++)
                    data_file >> read_buffer >> buffer[i][j];   // <name> <value>

                if (version_tag == "1.1")            // capture full-solution (decision) block until EOS
                {
                    // block tokens: "vK-"(var header) / "name:"(var name) / <numbers...> / "EOS"
                    while (data_file >> read_buffer)
                    {
                        if (read_buffer == "EOS")
                            break;
                        if (read_buffer.empty() || read_buffer.back() == '-' || read_buffer.back() == ':')
                            continue;                 // skip var header / var name
                        try { decision[i].push_back(std::stod(read_buffer)); }
                        catch (...) { /* non-numeric token, skip */ }
                    }
                }
            }
            return true;
        }

        void loadBatch(std::vector<std::vector<std::vector<double>>>& buffer,
            std::vector<std::vector<std::vector<double>>>& decision,
            const std::string optimizer, const std::string problem, const std::string tag,
            const std::vector<int>& runs)
        {
            for (int run : runs)
            {
                std::vector<std::vector<double>> one, oneDec;
                if (loadResult(one, oneDec, buildPath(problem, optimizer, tag, run)))
                {
                    buffer.push_back(one);
                    decision.push_back(oneDec);
                }
            }
        }

        void loadData()
        {
            clearData();
            _data_buffer.resize(_problems.size());
            _decision_buffer.resize(_problems.size());
            for (size_t p = 0; p < _problems.size(); p++)
            {
                _data_buffer[p].resize(_infos.size());
                _decision_buffer[p].resize(_infos.size());
            }

            for (size_t p = 0; p < _problems.size(); p++)
                for (size_t o = 0; o < _infos.size(); o++)
                    loadBatch(_data_buffer[p][o], _decision_buffer[p][o],
                        _infos[o].optimizer, _problems[p], _infos[o].tag, _infos[o].runs);
        }

        std::string getMetricName(_Q q)
        {
            switch (q)
            {
            case _Q::mean:         return "mean";
            case _Q::variance:     return "variance";
            case _Q::std_:         return "std";
            case _Q::median:       return "median";
            case _Q::mode:         return "mode";
            case _Q::smallest:     return "smallest";
            case _Q::largest:      return "largest";
            case _Q::q1:           return "Q1";
            case _Q::q3:           return "Q3";
            case _Q::iqr:          return "IQR";
            case _Q::range_:       return "range";
            case _Q::sem:          return "SEM";
            case _Q::cv:           return "CV";
            case _Q::geo_mean:     return "geo_mean";
            case _Q::harm_mean:    return "harm_mean";
            case _Q::percentile:   return "percentile";
            case _Q::hyper_volume: return "HV";
            case _Q::gd:           return "GD";
            case _Q::igd:          return "IGD";
            case _Q::peak_ratio:      return "peak_ratio";
            case _Q::peak_distance:   return "peak_distance";
            case _Q::peak_inaccuracy: return "peak_inaccuracy";
            default:               return "";
            }
        }

        std::string _number2string(double input)
        {
            std::stringstream ss;
            if (std::abs(input) >= 1000.0 || std::abs(input) <= 0.0001)
            {
                ss << std::scientific << std::setprecision(4) << input;
                return ss.str();
            }
            int integerPart = static_cast<int>(std::abs(input));
            int int_size = (input == 0) ? 1 : static_cast<int>(std::log10(integerPart)) + 1;
            ss << std::fixed << std::setprecision(5 - int_size) << input;
            return ss.str();
        }

        // apply a descriptive Stat over a raw double array
        static double _applyStat(_Q q, const double* d, int n)
        {
            switch (q)
            {
            case _Q::mean:     return MetricLib::mean(d, n);
            case _Q::variance: return MetricLib::variance(d, n);
            case _Q::std_:     return MetricLib::standardDeviation(d, n);
            case _Q::median:   return MetricLib::median(d, n);
            case _Q::mode:     return MetricLib::mode(d, n);
            case _Q::smallest: return MetricLib::smallest(d, n);
            case _Q::largest:  return MetricLib::largest(d, n);
            case _Q::q1:       return MetricLib::interQuartileRange(d, n).Q1;
            case _Q::q3:       return MetricLib::interQuartileRange(d, n).Q3;
            case _Q::iqr:      return MetricLib::interQuartileRange(d, n).IQR;
            case _Q::range_:   return MetricLib::range(d, n);
            case _Q::sem:      return MetricLib::standardDeviation(d, n) / std::sqrt((double)n);   // 标准误 = std/√n
            case _Q::cv:       { double m = MetricLib::mean(d, n); return (std::abs(m) < 1e-12) ? NAN : MetricLib::standardDeviation(d, n) / m; }   // 变异系数 = std/mean
            case _Q::geo_mean: return MetricLib::geometricMean(d, n);
            case _Q::harm_mean:return MetricLib::harmonicMean(d, n);
            default:           return NAN;
            }
        }

        // ---- stage 1: reduce each run's solution set to one scalar ----
        void _metric_cal()
        {
            int P = _problems.size(), O = _infos.size();
            _result_buffer.assign(P, std::vector<std::vector<double>>(O));
            for (int p = 0; p < P; p++)
                for (int o = 0; o < O; o++)
                    _result_buffer[p][o].resize(_data_buffer[p][o].size());

            for (int p = 0; p < P; p++)
            {
                for (int o = 0; o < O; o++)
                {
                    for (size_t re = 0; re < _data_buffer[p][o].size(); re++)
                    {
                        auto& run = _data_buffer[p][o][re];        // [solution][objective]
                        int sol = run.size();
                        if (sol == 0) { _result_buffer[p][o][re] = NAN; continue; }
                        int obj = run[0].size();

                        if (!_reduce_is_indicator)
                        {
                            // descriptive: reduce over solutions, at objective column _obj_index
                            int col = (_obj_index >= 0 && _obj_index < obj) ? _obj_index : 0;
                            std::vector<double> buf(sol);
                            for (int i = 0; i < sol; i++) buf[i] = run[i][col];
                            _result_buffer[p][o][re] = (_reduce == _Q::percentile)
                                ? MetricLib::quantile(buf.data(), sol, _reduce_params[0])   // reduceByPercentile:参数组[0]=p
                                : _applyStat(_reduce, buf.data(), sol);
                        }
                        else
                        {
                            // set-quality indicator: build double** view of the solution set
                            std::vector<double*> pts(sol);
                            for (int i = 0; i < sol; i++) pts[i] = run[i].data();
                            const std::vector<double>* ref = (p < (int)_ref.size()) ? &_ref[p] : nullptr;

                            if (_reduce == _Q::hyper_volume)
                            {
                                if (!ref || (int)ref->size() != obj)
                                    _result_buffer[p][o][re] = NAN;
                                else
                                    _result_buffer[p][o][re] = MetricLib::hv_math(
                                        pts.data(), const_cast<double*>(ref->data()), sol, obj);
                            }
                            else if (_reduce == _Q::gd || _reduce == _Q::igd)
                            {
                                if (!ref || obj == 0 || ref->size() % obj != 0 || ref->empty())
                                    _result_buffer[p][o][re] = NAN;
                                else
                                {
                                    int rsize = ref->size() / obj;
                                    std::vector<double*> front(rsize);
                                    for (int k = 0; k < rsize; k++)
                                        front[k] = const_cast<double*>(ref->data()) + k * obj;
                                    _result_buffer[p][o][re] = (_reduce == _Q::gd)
                                        ? MetricLib::gd(pts.data(), front.data(), sol, rsize, obj)
                                        : MetricLib::igd(pts.data(), front.data(), sol, rsize, obj);
                                }
                            }
                            else if (_reduce == _Q::peak_ratio)
                            {
                                // coordinate-free niching peak ratio (metriclib overload ②):
                                //   _ref[p] = { refer_fitness f*, peak_count, accuracy_f (ε), accuracy_d (r) }
                                //   needs decision variables -> requires logging(full result) => .rslt ver 1.1
                                auto& dec = _decision_buffer[p][o][re];   // [solution][decision-dim]
                                int decdim = dec.empty() ? 0 : (int)dec[0].size();
                                if (!ref || ref->size() < 4 || decdim == 0 || sol != (int)dec.size())
                                    _result_buffer[p][o][re] = NAN;
                                else
                                {
                                    double f_star = (*ref)[0];
                                    int    peak_n = (int)(*ref)[1];
                                    double acc_f  = (*ref)[2];
                                    double acc_d  = (*ref)[3];
                                    if (peak_n <= 0)
                                        _result_buffer[p][o][re] = NAN;
                                    else
                                    {
                                        std::vector<double> fit(sol);
                                        for (int i = 0; i < sol; i++) fit[i] = run[i][_obj_index];
                                        std::vector<const double*> dpts(sol);
                                        for (int i = 0; i < sol; i++) dpts[i] = dec[i].data();
                                        // overload ② ignores refer_point coords (only refer_size counts);
                                        // pass dpts as a non-null placeholder to satisfy its null-check.
                                        double pr = MetricLib::peakRatio(fit.data(), dpts.data(), sol,
                                            f_star, dpts.data(), peak_n, decdim, acc_f, acc_d);
                                        _result_buffer[p][o][re] = std::min(1.0, std::max(0.0, pr));  // clamp [0,1]
                                    }
                                }
                            }
                            else if (_reduce == _Q::peak_distance)
                            {
                                // 解到最近参考峰的平均欧氏距离(metriclib peakDistance)。
                                //   _ref[p] = 参考峰坐标 flat(refer_size × decdim,行主);需 logging(full result) 供解坐标。
                                auto& dec = _decision_buffer[p][o][re];
                                int decdim = dec.empty() ? 0 : (int)dec[0].size();
                                if (!ref || decdim == 0 || sol != (int)dec.size() || (int)ref->size() < decdim || ref->size() % decdim != 0)
                                    _result_buffer[p][o][re] = NAN;
                                else
                                {
                                    int rsize = (int)ref->size() / decdim;
                                    std::vector<const double*> dpts(sol), rpts(rsize);
                                    for (int i = 0; i < sol; i++)   dpts[i] = dec[i].data();
                                    for (int k = 0; k < rsize; k++) rpts[k] = ref->data() + (size_t)k * decdim;
                                    _result_buffer[p][o][re] = MetricLib::peakDistance(dpts.data(), rpts.data(), sol, rsize, decdim);
                                }
                            }
                            else if (_reduce == _Q::peak_inaccuracy)
                            {
                                // 解适应度与最近参考峰适应度的平均绝对差(metriclib peakInaccuracy)。
                                //   _ref[p] = 每峰 { fitness, coord_0..coord_{D-1} } flat(stride=1+decdim);需 logging(full result)。
                                auto& dec = _decision_buffer[p][o][re];
                                int decdim = dec.empty() ? 0 : (int)dec[0].size();
                                int stride = decdim + 1;
                                if (!ref || decdim == 0 || sol != (int)dec.size() || (int)ref->size() < stride || ref->size() % stride != 0)
                                    _result_buffer[p][o][re] = NAN;
                                else
                                {
                                    int rsize = (int)ref->size() / stride;
                                    std::vector<double> ifit(sol), rfit(rsize);
                                    std::vector<const double*> idec(sol), rdec(rsize);
                                    for (int i = 0; i < sol; i++)   { ifit[i] = run[i][_obj_index]; idec[i] = dec[i].data(); }
                                    for (int k = 0; k < rsize; k++) { rfit[k] = (*ref)[(size_t)k * stride]; rdec[k] = ref->data() + (size_t)k * stride + 1; }
                                    _result_buffer[p][o][re] = MetricLib::peakInaccuracy(ifit.data(), idec.data(), rfit.data(), rdec.data(), sol, rsize, decdim);
                                }
                            }
                            else
                            {
                                _result_buffer[p][o][re] = NAN;   // _Q::none
                            }
                        }
                    }
                }
            }
        }

        // ---- stage 2: aggregate the per-run scalars into table columns ----
        void _metric_analysis()
        {
            int P = _problems.size(), O = _infos.size();
            _analysis_buffer.assign(P, std::vector<std::vector<double>>(O, std::vector<double>(_analysis.size() + EX_COUNT, 0.0)));

            for (size_t m = 0; m < _analysis.size(); m++)
            {
                for (int p = 0; p < P; p++)
                {
                    for (int o = 0; o < O; o++)
                    {
                        if (_result_buffer[p][o].empty())
                            _analysis_buffer[p][o][m] = NAN;
                        else if (_analysis[m] == _Q::percentile)
                            _analysis_buffer[p][o][m] = MetricLib::quantile(
                                _result_buffer[p][o].data(), (int)_result_buffer[p][o].size(), _analysis_params[m][0]);
                        else
                            _analysis_buffer[p][o][m] = _applyStat(_analysis[m],
                                _result_buffer[p][o].data(), (int)_result_buffer[p][o].size());
                    }
                }
            }
        }

        // find the aggregation column index whose Stat == q (default 0 if absent)
        int _columnOf(_Q q)
        {
            for (size_t i = 0; i < _analysis.size(); i++)
                if (_analysis[i] == q) return (int)i;
            return 0;
        }

        void _significant_statistic()
        {
            int col = _analysis.size() + EX_SIG_P;   // Significance p 值槽
            int P = _problems.size(), O = _infos.size();
            for (int p = 0; p < P; p++)
                for (int o = 1; o < O; o++)
                {
                    auto& a = _result_buffer[p][0];
                    auto& b = _result_buffer[p][o];
                    if (a.empty() || b.empty())
                        _analysis_buffer[p][o][col] = NAN;
                    else
                        _analysis_buffer[p][o][col] = MetricLib::mann_whitney_u_test(
                            a.data(), (int)a.size(), b.data(), (int)b.size()).second;
                }
        }

        // 层1·独立·每问题综合:聚齐所有 O 组运行分布 → Kruskal-Wallis 综合 p 值 → M+7 存 [p][0](o=0 行)
        void _kruskal_statistic()
        {
            int col = _analysis.size() + EX_KW_P;
            int P = _problems.size(), O = _infos.size();
            for (int p = 0; p < P; p++)
            {
                std::vector<double*> samples; std::vector<int> lens;
                for (int o = 0; o < O; o++)
                    if (!_result_buffer[p][o].empty())
                    { samples.push_back(_result_buffer[p][o].data()); lens.push_back((int)_result_buffer[p][o].size()); }
                double kp = NAN;
                if (samples.size() >= 2)   // KW 需 ≥2 组(意义在 >2;=2 时 ≈U 检验)
                    try { kp = MetricLib::kruskal_wallis_h_test(samples.data(), lens.data(), (int)samples.size()).second; }
                    catch (const std::exception&) { kp = NAN; }   // metriclib 边界(空组等)→ 优雅降级
                _analysis_buffer[p][0][col] = kp;
            }
        }

        // 层2·配对·跨问题:每非基线算法 o vs 基线,按问题配对代表值(_q_wsr 列) → Wilcoxon 符号秩 p → M+8 存 [0][o]
        void _wilcoxon_statistic()
        {
            int col = _analysis.size() + EX_WSR_P;
            int comp = _columnOf(_q_wsr);
            int P = _problems.size(), O = _infos.size();
            for (int o = 1; o < O; o++)
            {
                std::vector<double> base, cur;
                for (int p = 0; p < P; p++)
                {
                    double b = _analysis_buffer[p][0][comp], c = _analysis_buffer[p][o][comp];
                    if (!std::isnan(b) && !std::isnan(c)) { base.push_back(b); cur.push_back(c); }
                }
                double wp = NAN;
                if (base.size() >= 1)
                    try { wp = MetricLib::wilcoxon_signed_rank_test(base.data(), cur.data(), (int)base.size()).second; }
                    catch (const std::exception&) { wp = NAN; }   // 全零差值等 → 无法检验 → NaN
                _analysis_buffer[0][o][col] = wp;
            }
        }

        // 层2·K 算法跨问题综合:从代表值列(_q_friedman)建 K×N 矩阵(处理=算法 o,区块=问题 p)→ Friedman χ²/p → M+9/M+10 存 [0][0]
        void _friedman_statistic()
        {
            int chi_col = _analysis.size() + EX_FRIEDMAN_CHI, p_col = _analysis.size() + EX_FRIEDMAN_P;
            int comp = _columnOf(_q_friedman);
            int P = _problems.size(), O = _infos.size();
            std::vector<std::vector<double>> mat(O);   // mat[o]=算法 o 在各"有效"问题的代表值(全算法非 NaN 的问题才纳入)
            for (int p = 0; p < P; p++)
            {
                bool ok = true;
                for (int o = 0; o < O; o++) if (std::isnan(_analysis_buffer[p][o][comp])) { ok = false; break; }
                if (!ok) continue;
                for (int o = 0; o < O; o++) mat[o].push_back(_analysis_buffer[p][o][comp]);
            }
            int n = mat.empty() ? 0 : (int)mat[0].size();
            double chi = NAN, pv = NAN;
            if (O >= 2 && n >= 1)
            {
                std::vector<double*> rows(O);
                for (int o = 0; o < O; o++) rows[o] = mat[o].data();
                try { auto r = MetricLib::friedman_test(rows.data(), O, n); chi = r.first; pv = r.second; }
                catch (const std::exception&) { chi = NAN; pv = NAN; }
            }
            _analysis_buffer[0][0][chi_col] = chi;
            _analysis_buffer[0][0][p_col] = pv;
        }

        // 层2·Friedman 后验:块内秩→各算法平均秩(M+12 on [0][o]) + 临界差 CD(M+11 on [0][0])。|秩差|>CD 即成对显著。
        void _nemenyi_statistic()
        {
            int cd_col = _analysis.size() + EX_NEMENYI_CD, ar_col = _analysis.size() + EX_NEMENYI_AVGRANK;
            int comp = _columnOf(_q_nemenyi);
            int P = _problems.size(), O = _infos.size();
            std::vector<double> Rsum(O, 0.0); int nblk = 0;
            for (int p = 0; p < P; p++)
            {
                bool ok = true;
                for (int o = 0; o < O; o++) if (std::isnan(_analysis_buffer[p][o][comp])) { ok = false; break; }
                if (!ok) continue;
                std::vector<double> vals(O);
                for (int o = 0; o < O; o++) vals[o] = _analysis_buffer[p][o][comp];
                double* r = MetricLib::rankElements(vals.data(), O);   // 块内 1 基秩(升序,含结平均)
                for (int o = 0; o < O; o++) Rsum[o] += r[o];
                delete[] r;
                nblk++;
            }
            _analysis_buffer[0][0][cd_col] = (O >= 2 && nblk >= 1) ? MetricLib::nemenyiCriticalDifference(O, nblk) : NAN;
            for (int o = 0; o < O; o++) _analysis_buffer[0][o][ar_col] = (nblk >= 1) ? Rsum[o] / nblk : NAN;
        }

        // 层1·KW 后验:每问题 pooled 秩,baseline(o=0) vs 各组 → Dunn Bonferroni p → M+13 on [p][o]
        void _dunn_statistic()
        {
            int col = _analysis.size() + EX_DUNN_P;
            int P = _problems.size(), O = _infos.size();
            for (int p = 0; p < P; p++)
            {
                std::vector<double*> samples; std::vector<int> lens, gmap;
                for (int o = 0; o < O; o++)
                    if (!_result_buffer[p][o].empty())
                    { samples.push_back(_result_buffer[p][o].data()); lens.push_back((int)_result_buffer[p][o].size()); gmap.push_back(o); }
                for (int o = 0; o < O; o++) _analysis_buffer[p][o][col] = NAN;
                if (samples.size() >= 2 && gmap[0] == 0)   // baseline 须为 o=0 且有数据
                {
                    auto padj = MetricLib::dunn_baseline_bonferroni(samples.data(), lens.data(), (int)samples.size());
                    for (size_t g = 1; g < padj.size(); g++) _analysis_buffer[p][gmap[g]][col] = padj[g];
                }
            }
        }

        void _wl_statistic()
        {
            int col = _analysis.size() + EX_WL_WIN;
            int comp = _columnOf(_q_wl);
            bool is_larger;

            if (_significance_test)
            {
                for (size_t o = 1; o < _infos.size(); o++)
                {
                    int win = 0, lose = 0, eq = 0;
                    for (size_t p = 0; p < _problems.size(); p++)
                    {
                        if (std::abs(_analysis_buffer[p][o][_analysis.size() + EX_SIG_P]) > 5e-4)   // not significant
                            eq++;
                        else
                        {
                            is_larger = _analysis_buffer[p][0][comp] > _analysis_buffer[p][o][comp];
                            (_min_wl ^ is_larger) ? win++ : lose++;
                        }
                    }
                    _analysis_buffer[0][o][col] = win;
                    _analysis_buffer[0][o][col + 1] = lose;
                    _analysis_buffer[0][o][col + 2] = eq;
                }
            }
            else
            {
                for (size_t o = 1; o < _infos.size(); o++)
                {
                    int win = 0, lose = 0;
                    for (size_t p = 0; p < _problems.size(); p++)
                    {
                        is_larger = _analysis_buffer[p][0][comp] > _analysis_buffer[p][o][comp];
                        (_min_wl ^ is_larger) ? win++ : lose++;
                    }
                    _analysis_buffer[0][o][col] = win;
                    _analysis_buffer[0][o][col + 1] = lose;
                }
            }
        }

        void _best_solution_statistic()
        {
            int col = _analysis.size() + EX_BEST;
            int comp = _columnOf(_q_best);

            for (size_t p = 0; p < _problems.size(); p++)
            {
                int best_id = 0;
                double best_val = _analysis_buffer[p][0][comp];
                for (size_t o = 1; o < _infos.size(); o++)
                {
                    bool is_larger = best_val > _analysis_buffer[p][o][comp];
                    if (!(_min_best ^ is_larger))
                    {
                        best_id = (int)o;
                        best_val = _analysis_buffer[p][o][comp];
                    }
                }
                for (size_t o = 0; o < _infos.size(); o++)
                    _analysis_buffer[p][o][col] = (o == (size_t)best_id) ? 1 : 0;
            }
        }

        void _rank_statistic()
        {
            int ranked_id = _analysis.size() + EX_RANK;
            int average_rank_id = _analysis.size() + EX_AVGRANK;
            int comp = _columnOf(_q_rank);
            const double BIG = (std::numeric_limits<double>::max)();

            std::vector<sortHelper<int, double>> sb(_infos.size());
            int factor = _min_rank ? 1 : -1;
            for (size_t p = 0; p < _problems.size(); p++)
            {
                for (size_t o = 0; o < _infos.size(); o++)
                {
                    sb[o].id = (int)o;
                    double v = _analysis_buffer[p][o][comp];
                    sb[o].value = std::isnan(v) ? BIG : v * factor;
                }
                std::sort(sb.begin(), sb.end());

                // average ranks for ties (1-based)
                size_t start = 0;
                for (size_t o = 1; o < _infos.size(); o++)
                {
                    if (sb[o].value == sb[start].value) continue;
                    double avg = double(start + o - 1) / 2 + 1;
                    for (size_t k = start; k < o; k++)
                        _analysis_buffer[p][sb[k].id][ranked_id] = avg;
                    start = o;
                }
                double avg = double(start + _infos.size() - 1) / 2 + 1;
                for (size_t k = start; k < _infos.size(); k++)
                    _analysis_buffer[p][sb[k].id][ranked_id] = avg;
            }

            for (size_t o = 0; o < _infos.size(); o++)
            {
                double total = 0;
                for (size_t p = 0; p < _problems.size(); p++)
                    total += _analysis_buffer[p][o][ranked_id];
                _analysis_buffer[0][o][average_rank_id] = total / _problems.size();
            }
        }

        // build the printable string table (header/subheader/rows/data/bolds)
        void _analysis_table(std::vector<std::string>& header, std::vector<std::string>& subheader,
            std::vector<std::string>& rows, std::vector<std::vector<std::string>>& datastrings,
            std::vector<std::vector<int>>& bolds)
        {
            header.clear();
            for (auto& info : _infos) header.push_back(info.optimizer);

            subheader.clear();
            for (size_t i = 0; i < _analysis.size(); i++)
                subheader.push_back(_analysis[i] == _Q::percentile
                    ? ("P" + _number2string(_analysis_params[i][0] * 100.0))
                    : getMetricName(_analysis[i]));
            if (_significance_test) subheader.push_back("p-value");
            if (_kw_test)           subheader.push_back("KW_p");
            if (_dunn_test)         subheader.push_back("Dunn_p");

            rows.clear();
            for (auto& pr : _problems) rows.push_back(pr);

            datastrings.assign(_problems.size(), {});
            for (size_t p = 0; p < _problems.size(); p++)
            {
                for (size_t o = 0; o < _infos.size(); o++)
                {
                    for (size_t m = 0; m < _analysis.size(); m++)
                        datastrings[p].push_back(std::isnan(_analysis_buffer[p][o][m]) ? "-"
                            : _number2string(_analysis_buffer[p][o][m]));
                    if (_significance_test)
                        datastrings[p].push_back(o == 0 ? "-"
                            : _number2string(_analysis_buffer[p][o][_analysis.size() + EX_SIG_P]));
                    if (_kw_test)   // 每问题综合 p:仅 o=0 行填(存 [p][0][M+7]),其余 "-"
                        datastrings[p].push_back((o == 0 && !std::isnan(_analysis_buffer[p][0][_analysis.size() + EX_KW_P]))
                            ? _number2string(_analysis_buffer[p][0][_analysis.size() + EX_KW_P]) : "-");
                    if (_dunn_test)   // KW 后验:baseline(o=0) "-",各算法 Bonferroni p(存 [p][o][M+13])
                        datastrings[p].push_back((o == 0 || std::isnan(_analysis_buffer[p][o][_analysis.size() + EX_DUNN_P])) ? "-"
                            : _number2string(_analysis_buffer[p][o][_analysis.size() + EX_DUNN_P]));
                }
            }

            if (_rank_stat)
            {
                datastrings.push_back({});
                int index = datastrings.size() - 1;
                rows.push_back("Rank");
                int ar_id = _analysis.size() + EX_AVGRANK;
                for (size_t o = 0; o < _infos.size(); o++)
                    datastrings[index].push_back(_number2string(_analysis_buffer[0][o][ar_id])
                        + "/" + std::to_string(_infos.size()));
            }
            if (_wl_stat)
            {
                datastrings.push_back({});
                int index = datastrings.size() - 1;
                datastrings[index].push_back("-");
                int wl_id = _analysis.size() + EX_WL_WIN;
                if (_significance_test)
                {
                    rows.push_back("+/=/-");
                    for (size_t o = 1; o < _infos.size(); o++)
                    {
                        std::stringstream ss;
                        ss << int(_analysis_buffer[0][o][wl_id]) << "/" << int(_analysis_buffer[0][o][wl_id + 2])
                           << "/" << int(_analysis_buffer[0][o][wl_id + 1]);
                        datastrings[index].push_back(ss.str());
                    }
                }
                else
                {
                    rows.push_back("+/-");
                    for (size_t o = 1; o < _infos.size(); o++)
                    {
                        std::stringstream ss;
                        ss << int(_analysis_buffer[0][o][wl_id]) << "/" << int(_analysis_buffer[0][o][wl_id + 1]);
                        datastrings[index].push_back(ss.str());
                    }
                }
            }
            if (_wsr_test)   // 层2 汇总行:各非基线算法 vs 基线的 Wilcoxon 符号秩 p(跨问题配对)
            {
                datastrings.push_back({});
                int index = datastrings.size() - 1;
                rows.push_back("Wilcoxon(vs base)");
                int wsr_id = _analysis.size() + EX_WSR_P;
                datastrings[index].push_back("-");   // baseline o=0
                for (size_t o = 1; o < _infos.size(); o++)
                    datastrings[index].push_back(std::isnan(_analysis_buffer[0][o][wsr_id]) ? "-"
                        : _number2string(_analysis_buffer[0][o][wsr_id]));
            }
            if (_friedman_test)   // 层2 汇总行:全套件 Friedman χ²/p(单值,首格显示)
            {
                datastrings.push_back({});
                int index = datastrings.size() - 1;
                rows.push_back("Friedman");
                double chi = _analysis_buffer[0][0][_analysis.size() + EX_FRIEDMAN_CHI], pv = _analysis_buffer[0][0][_analysis.size() + EX_FRIEDMAN_P];
                std::stringstream ss;
                if (std::isnan(chi)) ss << "-"; else ss << "chi2=" << _number2string(chi) << " p=" << _number2string(pv);
                datastrings[index].push_back(ss.str());
                for (size_t o = 1; o < _infos.size(); o++) datastrings[index].push_back("-");
            }
            if (_nemenyi_test)   // 层2 汇总行:各算法平均秩(+"*"=显著差于最优秩控制);CD 在行标签
            {
                datastrings.push_back({});
                int index = datastrings.size() - 1;
                double cd = _analysis_buffer[0][0][_analysis.size() + EX_NEMENYI_CD];
                int ar_col = _analysis.size() + EX_NEMENYI_AVGRANK;
                rows.push_back(std::isnan(cd) ? "Nemenyi" : ("Nemenyi(CD=" + _number2string(cd) + ")"));
                double best = 1e300;
                for (size_t o = 0; o < _infos.size(); o++) { double a = _analysis_buffer[0][o][ar_col]; if (!std::isnan(a) && a < best) best = a; }
                for (size_t o = 0; o < _infos.size(); o++) {
                    double a = _analysis_buffer[0][o][ar_col];
                    datastrings[index].push_back(std::isnan(a) ? "-"
                        : (_number2string(a) + ((!std::isnan(cd) && a - best > cd) ? "*" : "")));
                }
            }
            if (_best_stat)
            {
                bolds.assign(_problems.size(), {});
                int comp = _columnOf(_q_best);
                int be_id = _analysis.size() + EX_BEST;
                for (size_t p = 0; p < _problems.size(); p++)
                    for (size_t o = 0; o < _infos.size(); o++)
                        if (_analysis_buffer[p][o][be_id])
                            bolds[p].push_back(o * _analysis.size() + comp);
            }
        }

        // ===== 图表按需补算:命中用户声明的统计列则直读,否则现算到 scratch(不写 _analysis_buffer) =====
        double _chartStatQ(int p, int o, _Q q) const
        {
            for (size_t m = 0; m < _analysis.size(); m++)      // ① 用户已声明 → 零额外计算
                if (_analysis[m] == q) return _analysis_buffer[p][o][m];
            auto key = std::make_tuple(p, o, (int)q);          // ② 未声明 → scratch 补算(带记忆)
            auto it = _chart_scratch.find(key);
            if (it != _chart_scratch.end()) return it->second;
            double v = _result_buffer[p][o].empty() ? NAN
                     : _applyStat(q, _result_buffer[p][o].data(), (int)_result_buffer[p][o].size());
            _chart_scratch[key] = v;
            return v;
        }
        double _chartStat(int p, int o, Stat s) const { return _chartStatQ(p, o, _toQ(s)); }

        // 组内混合秩(并列取平均秩,1 基),与 metriclib::rankElements 同口径。越小越优。
        static std::vector<std::vector<double>> _pooledRank(const std::vector<std::vector<double>>& byOpt)
        {
            std::vector<std::pair<double, std::pair<int, int>>> flat;
            for (int i = 0; i < (int)byOpt.size(); i++)
                for (int r = 0; r < (int)byOpt[i].size(); r++) flat.push_back({ byOpt[i][r], { i, r } });
            std::sort(flat.begin(), flat.end(),
                      [](const std::pair<double, std::pair<int, int>>& a,
                         const std::pair<double, std::pair<int, int>>& b) { return a.first < b.first; });
            std::vector<std::vector<double>> out(byOpt.size());
            for (size_t i = 0; i < byOpt.size(); i++) out[i].resize(byOpt[i].size());
            size_t i = 0;
            while (i < flat.size()) {
                size_t j = i;
                while (j + 1 < flat.size() && flat[j + 1].first == flat[i].first) j++;
                double avg = (double)(i + j) / 2.0 + 1.0;
                for (size_t t = i; t <= j; t++) out[flat[t].second.first][flat[t].second.second] = avg;
                i = j + 1;
            }
            return out;
        }

        // 按 norm 变换问题 p 下各算法的 run 向量(结果是【局部副本】,不回写 _result_buffer)。
        //   norm 口径按【越小越优】解释(与分析器其它默认一致)。
        //   RatioToBest 在最优值 <= 0 时无定义 → 退回 None 并置 warn。
        std::vector<std::vector<double>> _normRuns(int p, Norm n, std::string& warn) const
        {
            std::vector<std::vector<double>> src;
            for (size_t o = 0; o < _infos.size(); o++) src.push_back(_result_buffer[p][o]);
            if (n == Norm::None) return src;
            if (n == Norm::PooledRank) return _pooledRank(src);

            double lo = std::numeric_limits<double>::infinity();
            double hi = -std::numeric_limits<double>::infinity();
            for (auto& v : src) for (double x : v) { lo = (std::min)(lo, x); hi = (std::max)(hi, x); }
            if (!(lo <= hi)) return src;                              // 该问题无数据

            if (n == Norm::RatioToBest) {
                if (!(lo > 0)) { warn = "RatioToBest undefined (best <= 0), fell back to raw"; return src; }
                for (auto& v : src) for (double& x : v) x /= lo;
                return src;
            }
            double sp = (hi - lo) > 0 ? (hi - lo) : 1.0;              // MinMax
            for (auto& v : src) for (double& x : v) x = (x - lo) / sp;
            return src;
        }

        static const char* _normYKey(Norm n)
        {
            switch (n) {
            case Norm::MinMax:     return "y_minmax";
            case Norm::PooledRank: return "y_rank";
            case Norm::RatioToBest:return "y_ratio";
            default:               return "y_raw";
            }
        }
        static const char* _normCapKey(Norm n)
        {
            switch (n) {
            case Norm::MinMax:     return "n_minmax";
            case Norm::PooledRank: return "n_rank";
            case Norm::RatioToBest:return "n_ratio";
            default:               return "n_none";
            }
        }
        // 文件名安全化:问题名可能含 '(' '/' 等,做逐问题批量出图时用作后缀
        static std::string _slug(const std::string& s)
        {
            std::string r;
            for (char c : s) r += (std::isalnum((unsigned char)c) || c == '-' || c == '_') ? c : '_';
            return r;
        }

        // ===== 箱线图/柱状图族的取数与出图(逐问题类走批量,跨问题类一张) =====
        void _chartBoxBar(Chart type, const std::string& path, const std::string& title,
                          const ChartSpec& spec, const FileOut::ChartStyle& styleIn)
        {
            bool isBox   = (type == Chart::BoxPlot  || type == Chart::MultiBoxPlot);
            bool isMulti = (type == Chart::MultiBoxPlot || type == Chart::MultiBarChart);
            int O = (int)_infos.size(), P = (int)_problems.size();

            std::vector<int> sel;
            const std::vector<int>& req = spec.getProblems();
            if (req.empty()) { for (int p = 0; p < P; p++) sel.push_back(p); }
            else for (size_t i = 0; i < req.size(); i++) if (req[i] >= 0 && req[i] < P) sel.push_back(req[i]);
            if (sel.empty()) { sys_logger.error("ECAnalyzer::chart: 问题选择为空或越界"); return; }

            // 【守卫】norm=MinMax 含 0(最优项)、PooledRank 是等距整数秩 —— 二者取对数无意义 → 降级为线性轴 + 告警
            FileOut::ChartStyle style = styleIn;
            Norm nm = spec.getNorm();
            if (style.hasLogScale() && (nm == Norm::MinMax || nm == Norm::PooledRank))
            {
                style.logScale(false);
                sys_logger.error("ECAnalyzer::chart: logScale 与 norm(MinMax/PooledRank) 不可共用"
                                 "(前者需取对数,后者含 0 或等距整数秩)——已降级为线性轴");
            }
            FileOut::Lang lg = style.getLang();

            std::vector<std::string> algos;
            for (int o = 0; o < O; o++) algos.push_back(_infos[o].optimizer);

            // 逐问题类 = 每问题一张(默认一图一问题);跨问题类 = 全部选中问题一张
            std::vector<std::vector<int>> batches;
            if (isMulti) batches.push_back(sel);
            else for (size_t i = 0; i < sel.size(); i++) batches.push_back(std::vector<int>(1, sel[i]));

            for (size_t b = 0; b < batches.size(); b++)
            {
                const std::vector<int>& gs = batches[b];
                std::vector<std::string> groups;
                for (size_t i = 0; i < gs.size(); i++) groups.push_back(_problems[gs[i]]);

                std::string yLabel = FileOut::t(lg, _normYKey(nm)), caption;
                if (style.hasCaption())
                {
                    caption  = isBox ? FileOut::t(lg, spec.hasJitter() ? "box_caption_pt" : "box_caption")
                                     : FileOut::t(lg, "bar_caption");
                    caption += FileOut::t(lg, _normCapKey(nm));
                    if (style.hasLogScale())              caption += FileOut::t(lg, "logaxis");
                    if (isMulti && nm == Norm::None)      caption += FileOut::t(lg, "cross_raw");
                }
                std::string file = _defaultPath(
                    batches.size() > 1 ? path + "_" + _slug(_problems[gs[0]]) : path, _chart_ext.c_str());

                std::string warnNorm, warn;
                if (isBox)
                {
                    std::vector<std::vector<FileOut::BoxCell>> data;
                    for (size_t g = 0; g < gs.size(); g++)
                    {
                        int p = gs[g];
                        std::vector<std::vector<double>> runs = _normRuns(p, nm, warnNorm);
                        std::vector<FileOut::BoxCell> row;
                        for (int o = 0; o < O; o++)
                        {
                            FileOut::BoxCell cell;
                            const std::vector<double>& v = runs[o];
                            if (v.empty()) { row.push_back(cell); continue; }
                            if (nm == Norm::None)     // 原始值:走按需补算(命中已声明统计列 ⇒ 零额外计算)
                            {
                                cell.lo  = _chartStat(p, o, Stat::Smallest);
                                cell.q1  = _chartStat(p, o, Stat::Q1);
                                cell.med = _chartStat(p, o, Stat::Median);
                                cell.q3  = _chartStat(p, o, Stat::Q3);
                                cell.hi  = _chartStat(p, o, Stat::Largest);
                            }
                            else                      // 变换后:同一套底层算法作用于变换后的 run 向量
                            {
                                int n = (int)v.size();
                                cell.lo  = _applyStat(_Q::smallest, v.data(), n);
                                cell.q1  = _applyStat(_Q::q1,       v.data(), n);
                                cell.med = _applyStat(_Q::median,   v.data(), n);
                                cell.q3  = _applyStat(_Q::q3,       v.data(), n);
                                cell.hi  = _applyStat(_Q::largest,  v.data(), n);
                            }
                            if (spec.hasJitter()) cell.points = v;
                            row.push_back(cell);
                        }
                        data.push_back(row);
                    }
                    warn = FileOut::drawBoxChart(algos, groups, data, yLabel, caption, title, file, style, _chart_fmt);
                }
                else
                {
                    _Q qv = spec.hasStat() ? _toQ(spec.getStat())
                                           : (!_analysis.empty() ? _analysis[0] : _Q::median);
                    std::vector<std::vector<FileOut::BarCell>> data;
                    for (size_t g = 0; g < gs.size(); g++)
                    {
                        int p = gs[g];
                        std::vector<std::vector<double>> runs = _normRuns(p, nm, warnNorm);
                        std::vector<FileOut::BarCell> row;
                        for (int o = 0; o < O; o++)
                        {
                            FileOut::BarCell cell;
                            const std::vector<double>& v = runs[o];
                            if (v.empty()) { cell.value = NAN; row.push_back(cell); continue; }
                            int n = (int)v.size();
                            if (nm == Norm::None)
                            {
                                cell.value = _chartStatQ(p, o, qv);
                                if (spec.hasBand())
                                { cell.lo = _chartStat(p, o, spec.bandLo()); cell.hi = _chartStat(p, o, spec.bandHi()); }
                            }
                            else
                            {
                                cell.value = _applyStat(qv, v.data(), n);
                                if (spec.hasBand())
                                {
                                    cell.lo = _applyStat(_toQ(spec.bandLo()), v.data(), n);
                                    cell.hi = _applyStat(_toQ(spec.bandHi()), v.data(), n);
                                }
                            }
                            row.push_back(cell);
                        }
                        data.push_back(row);
                    }
                    warn = FileOut::drawBarChart(algos, groups, data, yLabel, caption, title, file, style, _chart_fmt);
                }
                if (!warnNorm.empty()) sys_logger.error("ECAnalyzer::chart norm: " + warnNorm);
                if (!warn.empty())     sys_logger.error("ECAnalyzer::chart 画布告警: " + warn);
            }
        }

        // ===== Pareto 前沿:逐问题一图,目标空间 2D 散点 =====
        //   数据直取 _data_buffer —— .rslt 存的是 getBest()=档案内容,配 Multiobject 档案即非支配集,
        //   故【此处不做非支配排序】(另:MOUtil::fastNonDominatedSort 形参是 Individual**,
        //   本就无法直接作用于 _data_buffer 的裸数组)。
        void _chartPareto(const std::string& path, const std::string& title,
                          const ChartSpec& spec, const FileOut::ChartStyle& style)
        {
            int O = (int)_infos.size(), P = (int)_problems.size();
            if (_data_buffer.empty())
            { sys_logger.error("ECAnalyzer::chart(ParetoFront): 无解集数据,需先 run()"); return; }

            std::vector<int> sel;
            const std::vector<int>& req = spec.getProblems();
            if (req.empty()) { for (int p = 0; p < P; p++) sel.push_back(p); }
            else for (size_t i = 0; i < req.size(); i++) if (req[i] >= 0 && req[i] < P) sel.push_back(req[i]);
            if (sel.empty()) { sys_logger.error("ECAnalyzer::chart(ParetoFront): 问题选择为空或越界"); return; }

            FileOut::Lang lg = style.getLang();
            int ox = spec.objX(), oy = spec.objY();

            for (size_t s = 0; s < sel.size(); s++)
            {
                int p = sel[s];
                // 目标数校验:单目标画不出前沿 → 拒绝出图(不产空图)
                int nobj = 0;
                for (int o = 0; o < O && nobj == 0; o++)
                    for (size_t r = 0; r < _data_buffer[p][o].size() && nobj == 0; r++)
                        if (!_data_buffer[p][o][r].empty()) nobj = (int)_data_buffer[p][o][r][0].size();
                if (nobj < 2)
                {
                    sys_logger.error("ECAnalyzer::chart(ParetoFront): 问题 '" + _problems[p]
                        + "' 只有 " + std::to_string(nobj) + " 个目标,前沿至少需要 2 个");
                    continue;
                }
                if (ox < 0 || ox >= nobj || oy < 0 || oy >= nobj || ox == oy)
                {
                    sys_logger.error("ECAnalyzer::chart(ParetoFront): objectives(" + std::to_string(ox)
                        + "," + std::to_string(oy) + ") 越界或重复(该问题有 " + std::to_string(nobj) + " 个目标)");
                    continue;
                }

                std::vector<FileOut::ScatterSeries> series;
                for (int o = 0; o < O; o++)
                {
                    FileOut::ScatterSeries ss; ss.name = _infos[o].optimizer;
                    int R = (int)_data_buffer[p][o].size();
                    int r0 = 0, r1 = R;
                    if (spec.getRun() >= 0)                 // 收窄到单次运行
                    { if (spec.getRun() >= R) continue; r0 = spec.getRun(); r1 = r0 + 1; }
                    for (int r = r0; r < r1; r++)
                        for (size_t i = 0; i < _data_buffer[p][o][r].size(); i++)
                        {
                            const std::vector<double>& sol = _data_buffer[p][o][r][i];
                            if ((int)sol.size() > (std::max)(ox, oy))
                                ss.pts.push_back(std::make_pair(sol[ox], sol[oy]));
                        }
                    if (!ss.pts.empty()) series.push_back(ss);
                }
                if (series.empty())
                { sys_logger.error("ECAnalyzer::chart(ParetoFront): 无可用解集"); continue; }

                std::string caption;
                if (style.hasCaption())
                {
                    caption  = FileOut::t(lg, "pf_caption");
                    caption += FileOut::t(lg, spec.getRun() >= 0 ? "pf_run1" : "pf_runs");
                    if (spec.hasConnect()) caption += FileOut::t(lg, "pf_connect");
                }
                std::string file = _defaultPath(
                    sel.size() > 1 ? path + "_" + _slug(_problems[p]) : path, _chart_ext.c_str());
                std::string warn = FileOut::drawScatter2D(series,
                    "f" + std::to_string(ox + 1), "f" + std::to_string(oy + 1),
                    caption, spec.hasConnect(), title, file, style, _chart_fmt);
                if (!warn.empty()) sys_logger.error("ECAnalyzer::chart(ParetoFront) 画布告警: " + warn);
            }
        }

        // ===== 平均秩条:跨问题一图 =====
        //   两个平均秩槽口径不同:EX_AVGRANK 来自 Compare::Rank(逐问题名次求平均);
        //   EX_NEMENYI_AVGRANK 来自 Friedman 块内秩(仅纳入所有算法都有数据的问题)。
        //   并存时取 Rank(更基础),说明文字注明来源,避免与 CD 图的秩混为一谈。
        void _chartRankBar(const std::string& path, const std::string& title,
                           const ChartSpec& spec, const FileOut::ChartStyle& style)
        {
            (void)spec;
            int O = (int)_infos.size(), M = (int)_analysis.size();
            bool useRank = _rank_stat, useNem = (!_rank_stat && _nemenyi_test);
            if (!useRank && !useNem)
            { sys_logger.error("ECAnalyzer::chart(RankBar): 需先 addComparison(Compare::Rank) 或 Compare::Nemenyi"); return; }

            int col = M + (useRank ? EX_AVGRANK : EX_NEMENYI_AVGRANK);
            std::vector<std::string> algos; std::vector<double> ranks;
            for (int o = 0; o < O; o++)
            { algos.push_back(_infos[o].optimizer); ranks.push_back(_analysis_buffer[0][o][col]); }

            std::string caption;
            if (style.hasCaption())
                caption = FileOut::t(style.getLang(), "rb_caption")
                        + FileOut::t(style.getLang(), useRank ? "rb_src_rank" : "rb_src_nem");
            std::string warn = FileOut::drawRankBar(algos, ranks, caption, title,
                                                    _defaultPath(path, _chart_ext.c_str()), style, _chart_fmt);
            if (!warn.empty()) sys_logger.error("ECAnalyzer::chart(RankBar) 画布告警: " + warn);
        }

        // ===== 收敛曲线:逐问题一图,x=FES =====
        void _chartConvergence(const std::string& path, const std::string& title,
                               const ChartSpec& spec, const FileOut::ChartStyle& style)
        {
            int O = (int)_infos.size(), P = (int)_problems.size();
            std::vector<int> sel;
            const std::vector<int>& req = spec.getProblems();
            if (req.empty()) { for (int p = 0; p < P; p++) sel.push_back(p); }
            else for (size_t i = 0; i < req.size(); i++) if (req[i] >= 0 && req[i] < P) sel.push_back(req[i]);
            if (sel.empty()) { sys_logger.error("ECAnalyzer::chart(Convergence): 问题选择为空或越界"); return; }

            _trace_swarm = spec.getSwarm();   // v2.5.3:取源级别(空=种群级/全局;非空=子群级);_tracePath 据此拼路径

            FileOut::Lang lg = style.getLang();
            int G = spec.getGrid();

            for (size_t s = 0; s < sel.size(); s++)
            {
                int p = sel[s];
                std::string err;
                long long hi = _traceMaxFES(p, err);
                if (hi <= 0)
                {
                    sys_logger.error("ECAnalyzer::chart(Convergence): 无可用轨迹(需 logging(_, true) 产出 .prcs)"
                                     + std::string(err.empty() ? "" : "; " + err));
                    continue;
                }
                // 公共 FES 网格(跨算法同一把尺 ⇒ 曲线可比);logScale 时按对数取点
                std::vector<double> grid(G);
                bool lg10 = style.hasLogScale();
                double lo = lg10 ? (std::max)(1.0, (double)hi / 1e4) : 0.0;
                for (int i = 0; i < G; i++)
                    grid[i] = lg10
                        ? std::pow(10, std::log10(lo) + (std::log10((double)hi) - std::log10(lo)) * i / (G - 1.0))
                        : lo + ((double)hi - lo) * i / (G - 1.0);

                std::vector<FileOut::CurveSeries> series;
                for (int o = 0; o < O; o++)
                {
                    std::vector<std::vector<double>> curve;      // [run][网格点]
                    if (!_resampleRuns(p, o, grid, curve, err)) continue;

                    FileOut::CurveSeries cs; cs.name = _infos[o].optimizer;
                    cs.line.assign(G, NAN);
                    if (spec.hasBand()) { cs.lo.assign(G, NAN); cs.hi.assign(G, NAN); }
                    // 聚合维:每个网格点上跨 run 施加【已声明的】统计量;未声明则用中位数
                    _Q qMain = spec.hasStat() ? _toQ(spec.getStat())
                                              : (!_analysis.empty() ? _analysis[0] : _Q::median);
                    std::vector<double> col;
                    for (int g = 0; g < G; g++)
                    {
                        col.clear();
                        for (size_t r = 0; r < curve.size(); r++)
                            if (!std::isnan(curve[r][g])) col.push_back(curve[r][g]);
                        if (col.empty()) continue;
                        int n = (int)col.size();
                        cs.line[g] = _applyStat(qMain, col.data(), n);
                        if (spec.hasBand())
                        {
                            cs.lo[g] = _applyStat(_toQ(spec.bandLo()), col.data(), n);
                            cs.hi[g] = _applyStat(_toQ(spec.bandHi()), col.data(), n);
                        }
                    }
                    series.push_back(cs);
                }
                if (series.empty())
                { sys_logger.error("ECAnalyzer::chart(Convergence): 所有算法均无可用轨迹"); continue; }

                std::string yLabel = FileOut::t(lg, "y_raw");
                std::string caption;
                if (style.hasCaption())
                {
                    caption = FileOut::t(lg, "conv_caption");
                    if (style.hasLogScale()) caption += FileOut::t(lg, "logaxis");
                }
                std::string file = _defaultPath(
                    sel.size() > 1 ? path + "_" + _slug(_problems[p]) : path, _chart_ext.c_str());
                std::string warn = FileOut::drawConvergence(grid, series, yLabel, caption, title, file, style, _chart_fmt);
                if (!warn.empty()) sys_logger.error("ECAnalyzer::chart(Convergence) 画布告警: " + warn);
                if (!err.empty())  sys_logger.error("ECAnalyzer::chart(Convergence) 轨迹告警: " + err);
            }
        }

        std::string _defaultPath(std::string path, const char* ext)
        {
            dir_create("_analysis");
            if (path.empty())
                return "_analysis/" + getMetricName(_reduce) + "(" + std::to_string(time(NULL)) + ")." + ext;
            if (path.find('/') == std::string::npos && path.find('\\') == std::string::npos)
                path = "_analysis/" + path;

            const size_t slash = path.find_last_of("/\\");
            const size_t fileBegin = (slash == std::string::npos) ? 0 : slash + 1;
            const size_t dot = path.find_last_of('.');
            const std::string wanted = std::string(".") + ext;
            if (dot == std::string::npos || dot < fileBegin || dot == fileBegin)
                return path + wanted;

            std::string current = path.substr(dot);
            std::string lowerCurrent = current;
            std::string lowerWanted = wanted;
            std::transform(lowerCurrent.begin(), lowerCurrent.end(), lowerCurrent.begin(),
                           [](unsigned char c) { return (char)std::tolower(c); });
            std::transform(lowerWanted.begin(), lowerWanted.end(), lowerWanted.begin(),
                           [](unsigned char c) { return (char)std::tolower(c); });
            if (lowerCurrent == lowerWanted) return path;

            const std::string corrected = path.substr(0, dot) + wanted;
            sys_logger.warning("ECAnalyzer: chart/report path extension '" + current
                               + "' conflicts with requested format; using '" + corrected + "'");
            return corrected;
        }

    public:
        ECAnalyzer() {}
        ~ECAnalyzer() { clearAll(); }

        // ---- participants ----
        // 改写结果/过程日志根目录。空串回落到既有默认 `_log`;
        // CLI/UI 报告门面用它确保“发现批次”和“实际读取”来自同一目录。
        ECAnalyzer& logRoot(std::string path)
        {
            _log_root = path.empty() ? "_log" : std::move(path);
            return *this;
        }
        ECAnalyzer& addOptimizer(std::string optimizer, std::string tag, int repeats)
        {
            _infos.push_back(PartiInfo(optimizer, tag, repeats));
            return *this;
        }
        ECAnalyzer& addOptimizer(std::string optimizer, std::string tag,
                                 const std::vector<int>& runs)
        {
            _infos.push_back(PartiInfo(std::move(optimizer), std::move(tag), runs));
            return *this;
        }
        ECAnalyzer& addProblem(std::string problem_name)
        {
            _problems.push_back(problem_name);
            return *this;
        }
        ECAnalyzer& addProblem(const std::vector<std::string>& names)
        {
            for (auto& n : names) _problems.push_back(n);
            return *this;
        }

        // ---- stage 1: per-run reduction (single choice) ----
        // descriptive reduction over the solution set, at objective column objIndex
        ECAnalyzer& reduceBy(Stat s, int objIndex = 0)
        {
            _reduce = _toQ(s);
            _reduce_is_indicator = false;
            _obj_index = objIndex;
            _ref.clear();
            return *this;
        }
        // 参数化分位数归约(p∈[0,1]):把每次运行的解集在目标列 objIndex 上取 p 分位为标量。与 addPercentile 对称。
        ECAnalyzer& reduceByPercentile(double p, int objIndex = 0)
        {
            _reduce = _Q::percentile;
            _reduce_params = { p };
            _reduce_is_indicator = false;
            _obj_index = objIndex;
            _ref.clear();
            return *this;
        }
        // set-quality indicator; refPerProblem[p] interpretation depends on `ind`:
        //   HyperVolume : reference point            (length = objectives)
        //   GD / IGD    : row-major reference front   (length = refSize * objectives)
        //   PeakRatio      : { refer_fitness f*, peak_count, accuracy_f (ε), accuracy_d (niche radius r) }
        //                    (coordinate-free niching overload ②)
        //   PeakDistance   : row-major reference-peak coordinates   (length = refSize * decisionDim)
        //   PeakInaccuracy : per peak { fitness, coord_0..coord_{D-1} } flat (stride = 1 + decisionDim)
        //   (PeakRatio/Distance/Inaccuracy all need decision vars => run with logging(full result))
        ECAnalyzer& reduceBy(Indicator ind, const std::vector<std::vector<double>>& refPerProblem = {})
        {
            _reduce = _toQ(ind);
            _reduce_is_indicator = true;
            _ref = refPerProblem;
            return *this;
        }

        // ---- stage 2: aggregation columns (multiple) ----
        ECAnalyzer& addStatistic(Stat s)
        {
            _analysis.push_back(_toQ(s));
            _analysis_params.push_back({});        // 无参列:空参数组(保持与 _analysis 平行)
            return *this;
        }

        // 参数化分位数列(p∈[0,1],如 0.25=P25=Q1)。与 addStatistic 同为表列,经 metriclib quantile 计算。
        ECAnalyzer& addPercentile(double p)
        {
            _analysis.push_back(_Q::percentile);
            _analysis_params.push_back({ p });     // percentile 列:参数组={p}
            return *this;
        }

        // ---- stage 3: cross-optimizer comparison ----
        // `on` = which aggregation column to compare (ignored for Significance);
        // `dir` = objective sense.
        ECAnalyzer& addComparison(Compare c, Stat on = Stat::Mean, Direction dir = Direction::MinIsBetter)
        {
            bool min_is_better = (dir == Direction::MinIsBetter);
            switch (c)
            {
            case Compare::Significance: _significance_test = true; break;
            case Compare::Rank:       _rank_stat = true; _q_rank = _toQ(on); _min_rank = min_is_better; break;
            case Compare::WinTieLose: _wl_stat = true;   _q_wl = _toQ(on);   _min_wl = min_is_better;   break;
            case Compare::Best:       _best_stat = true; _q_best = _toQ(on); _min_best = min_is_better; break;
            case Compare::KruskalWallis:     _kw_test = true; break;                    // 层1(on 忽略,用运行分布)
            case Compare::WilcoxonSignedRank:_wsr_test = true; _q_wsr = _toQ(on); break; // 层2(on=代表值列,如 Median)
            case Compare::Friedman:          _friedman_test = true; _q_friedman = _toQ(on); break; // 层2(on=代表值列)
            case Compare::Nemenyi:           _nemenyi_test = true;  _q_nemenyi = _toQ(on); break;  // 层2(Friedman 后验)
            case Compare::Dunn:              _dunn_test = true; break;                              // 层1(KW 后验,on 忽略)
            }
            return *this;
        }

        // ---- compute ----
        ECAnalyzer& run()
        {
            loadData();
            _metric_cal();
            _metric_analysis();
            if (_significance_test) _significant_statistic();
            if (_kw_test)           _kruskal_statistic();      // 层1(消费 _result_buffer)
            if (_wsr_test)          _wilcoxon_statistic();     // 层2(消费 _metric_analysis 后的 _analysis_buffer)
            if (_friedman_test)     _friedman_statistic();     // 层2
            if (_nemenyi_test)      _nemenyi_statistic();      // 层2(Friedman 后验)
            if (_dunn_test)         _dunn_statistic();         // 层1(KW 后验)
            if (_rank_stat)         _rank_statistic();
            if (_wl_stat)           _wl_statistic();
            if (_best_stat)         _best_solution_statistic();
            return *this;
        }
        ECAnalyzer& cal() { return run(); }   // backward-compatible alias

        // ---- output ----
        // 设本 analyzer 的图表样式(色表/语言/是否带说明):设一次,后续所有 chart() 共用
        //   ⇒ 同批结果跨图色表一致、相同算法同色(颜色按 optimizer 添加序取)。
        ECAnalyzer& chartStyle(const FileOut::ChartStyle& s) { _chart_style = s; return *this; }

        // ===== 流式图表接口(v2.2.2.6)=====
        //   每图种自暴露方法,返回该图种【专属构建器】,只带对它有效的选项 ⇒ 传错参数编译不过:
        //     az.chart().boxPlot("box","分布").jitter(true).draw();
        //     az.chart().convergence("conv","收敛").grid(400).draw();
        //   产物与 report() 同路径约定:裸名 → _analysis/<path>.svg(目录自动建)。
        ChartMaker chart() { return ChartMaker(this); }

        // 内部:供 chart-builder 的 draw() 调用(用户不直接用)。把构建器收集的选项
        //   翻译成内部 Chart/ChartSpec 后走既有分派 —— 出图逻辑零重写。
        ECAnalyzer& _drawChart(chart_detail::ChartKind kind, const chart_detail::ChartSpec& cs,
                               const std::string& path, const std::string& title,
                               const FileOut::ChartStyle* style)
        {
            Chart type = Chart::CdDiagram;
            switch (kind) {   // 显式映射,不靠"两个枚举顺序恰好一致"的隐式约定
            case chart_detail::ChartKind::CdDiagram:     type = Chart::CdDiagram;     break;
            case chart_detail::ChartKind::BoxPlot:       type = Chart::BoxPlot;       break;
            case chart_detail::ChartKind::MultiBoxPlot:  type = Chart::MultiBoxPlot;  break;
            case chart_detail::ChartKind::BarChart:      type = Chart::BarChart;      break;
            case chart_detail::ChartKind::MultiBarChart: type = Chart::MultiBarChart; break;
            case chart_detail::ChartKind::Table:         type = Chart::Table;         break;
            case chart_detail::ChartKind::WinTieLose:    type = Chart::WinTieLose;    break;
            case chart_detail::ChartKind::RankHeatmap:   type = Chart::RankHeatmap;   break;
            case chart_detail::ChartKind::Convergence:   type = Chart::Convergence;   break;
            case chart_detail::ChartKind::ParetoFront:   type = Chart::ParetoFront;   break;
            case chart_detail::ChartKind::RankBar:       type = Chart::RankBar;       break;
            }
            ChartSpec spec;
            switch (cs.norm) {
            case chart_detail::Norm::MinMax:      spec.norm(Norm::MinMax);      break;
            case chart_detail::Norm::PooledRank:  spec.norm(Norm::PooledRank);  break;
            case chart_detail::Norm::RatioToBest: spec.norm(Norm::RatioToBest); break;
            default:                              spec.norm(Norm::None);        break;
            }
            spec.jitter(cs.jitter).grid(cs.grid).run(cs.run).connect(cs.connect)
                .objectives(cs.objX, cs.objY).swarm(cs.swarm);   // v2.5.3:搬运收敛取源级别
            spec.format(cs.format).pngFont(cs.pngFont);
            if (!cs.problems.empty()) spec.problems(cs.problems);
            if (cs.hasStat) spec.statistic((Stat)cs.statQ);
            if (cs.hasBand) spec.band((Stat)cs.bandLoQ, (Stat)cs.bandHiQ);
            else            spec.noBand();
            return _chartDispatch(type, path, title, spec, style ? *style : _chart_style);
        }

    private:
        ECAnalyzer& _chartDispatch(Chart type, std::string path, std::string title,
                          const ChartSpec& spec, const FileOut::ChartStyle& style)
        {
            if (_analysis_buffer.empty() || _infos.empty())
            { sys_logger.error("ECAnalyzer::chart: 需先 run();无分析数据"); return *this; }
            _chart_scratch.clear();                       // 每次出图独立,画完即清
            // v2.2.3:格式决定扩展名(ChartCanvas::save 按扩展名分派后端);字体经全局配置传入
            _chart_fmt = (spec.getFormat() == 1) ? FileOut::ChartFormat::Png : FileOut::ChartFormat::Svg;
            _chart_ext = (spec.getFormat() == 1) ? "png" : "svg";
            if (spec.getFormat() == 1) FileOut::currentPngFont() = spec.getPngFont();

            switch (type)
            {
            case Chart::BoxPlot:
            case Chart::MultiBoxPlot:
            case Chart::BarChart:
            case Chart::MultiBarChart:
                _chartBoxBar(type, path, title, spec, style);
                break;

            case Chart::Convergence:
                _chartConvergence(path, title, spec, style);
                break;

            case Chart::ParetoFront:
                _chartPareto(path, title, spec, style);
                break;

            case Chart::RankBar:
                _chartRankBar(path, title, spec, style);
                break;

            case Chart::Table:
            {
                // 与 report() 同源:复用同一个 _analysis_table(),一格数都不重算
                std::vector<std::string> header, subheader, rows;
                std::vector<std::vector<std::string>> datastrings;
                std::vector<std::vector<int>> bolds;
                _analysis_table(header, subheader, rows, datastrings, bolds);
                std::string warn = FileOut::drawTableChart(header, subheader, rows, datastrings, bolds,
                                                           title, _defaultPath(path, _chart_ext.c_str()), style, _chart_fmt);
                if (!warn.empty()) sys_logger.error("ECAnalyzer::chart(Table) 画布告警: " + warn);
                break;
            }

            case Chart::WinTieLose:
            {
                if (!_wl_stat)
                { sys_logger.error("ECAnalyzer::chart(WinTieLose): 需先 addComparison(Compare::WinTieLose)"); return *this; }
                int O = (int)_infos.size(), M = (int)_analysis.size();
                std::vector<std::string> algos; std::vector<int> w, t_, l;
                for (int o = 0; o < O; o++)      // 对象索引 = optimizer 添加序
                {
                    algos.push_back(_infos[o].optimizer);
                    w.push_back ((int)std::lround(_analysis_buffer[0][o][M + EX_WL_WIN]));
                    l.push_back ((int)std::lround(_analysis_buffer[0][o][M + EX_WL_LOSE]));
                    t_.push_back((int)std::lround(_analysis_buffer[0][o][M + EX_WL_TIE]));
                }
                std::string warn = FileOut::drawWinTieLose(algos, w, t_, l, title,
                                                           _defaultPath(path, _chart_ext.c_str()), style, _chart_fmt);
                if (!warn.empty()) sys_logger.error("ECAnalyzer::chart(WinTieLose) 画布告警: " + warn);
                break;
            }

            case Chart::RankHeatmap:
            {
                int O = (int)_infos.size(), P = (int)_problems.size(), M = (int)_analysis.size();
                bool useRank = !spec.hasStat();
                if (useRank && !_rank_stat)
                { sys_logger.error("ECAnalyzer::chart(RankHeatmap): 需先 addComparison(Compare::Rank),或用 ChartSpec::statistic() 指定统计量"); return *this; }
                std::vector<std::string> algos;
                for (int o = 0; o < O; o++) algos.push_back(_infos[o].optimizer);
                std::vector<std::vector<double>> gridv(P, std::vector<double>(O, NAN));
                for (int p = 0; p < P; p++)
                    for (int o = 0; o < O; o++)
                        gridv[p][o] = useRank ? _analysis_buffer[p][o][M + EX_RANK]
                                              : _chartStat(p, o, spec.getStat());   // 未声明则按需补算
                std::string vlabel = useRank ? (style.getLang() == FileOut::Lang::ZH ? "名次" : "Rank")
                                             : (style.getLang() == FileOut::Lang::ZH ? "统计量" : "Statistic");
                bool lowBetter = useRank ? true : true;   // 名次与目标值同为"越小越优"
                std::string warn = FileOut::drawHeatmap(algos, _problems, gridv, lowBetter, vlabel,
                                                        title, _defaultPath(path, _chart_ext.c_str()), style, _chart_fmt);
                if (!warn.empty()) sys_logger.error("ECAnalyzer::chart(RankHeatmap) 画布告警: " + warn);
                break;
            }
            case Chart::CdDiagram:
            {
                if (!_nemenyi_test)
                { sys_logger.error("ECAnalyzer::chart(CdDiagram): 需先 addComparison(Compare::Nemenyi)"); return *this; }
                int O = (int)_infos.size();
                int ar = (int)_analysis.size() + EX_NEMENYI_AVGRANK;
                double CD = _analysis_buffer[0][0][(int)_analysis.size() + EX_NEMENYI_CD];
                std::vector<std::string> algos; std::vector<double> ranks;
                for (int o = 0; o < O; o++)                      // 对象索引 = optimizer 添加序(颜色键)
                { algos.push_back(_infos[o].optimizer); ranks.push_back(_analysis_buffer[0][o][ar]); }
                if (std::isnan(CD))
                { sys_logger.error("ECAnalyzer::chart(CdDiagram): CD 为 NaN(区块不足?)"); return *this; }
                std::string warn = FileOut::drawCdDiagram(algos, ranks, CD, title,
                                                          _defaultPath(path, _chart_ext.c_str()), style, _chart_fmt);
                if (!warn.empty()) sys_logger.error("ECAnalyzer::chart(CdDiagram) 画布告警: " + warn);
                break;
            }
            }
            _chart_scratch.clear();
            return *this;
        }
    public:

        // v2.3.1.5:**取出报告表格本身**(不渲染、不落盘)。供 `evoflow report` 与 UI 的统计分析页
        //   自行渲染成 txt/csv/latex/json/界面 —— 与 report(Format) **同源**(同一个 _analysis_table),
        //   故两条出口的数值必然一致。纯追加,既有 report() 不受影响。
        ECAnalyzer& tableForReport(std::vector<std::string>& header, std::vector<std::string>& subheader,
                                   std::vector<std::string>& rows,
                                   std::vector<std::vector<std::string>>& cells,
                                   std::vector<std::vector<int>>& bolds)
        {
            _analysis_table(header, subheader, rows, cells, bolds);
            return *this;
        }

        ECAnalyzer& report(Format fmt, std::string path = "")
        {
            std::vector<std::string> header, subheader, rows;
            std::vector<std::vector<std::string>> datastrings;
            std::vector<std::vector<int>> bolds;

            switch (fmt)
            {
            case Format::Txt:
                _analysis_table(header, subheader, rows, datastrings, bolds);
                FileOut::writeTableToTxt(header, subheader, rows, datastrings, _defaultPath(path, "txt"));
                break;
            case Format::Csv:
                _analysis_table(header, subheader, rows, datastrings, bolds);
                FileOut::writeTableToCSV(header, subheader, rows, datastrings, _defaultPath(path, "csv"));
                break;
            case Format::Latex:
                _analysis_table(header, subheader, rows, datastrings, bolds);
                FileOut::writeTableToLaTeX(header, subheader, rows, datastrings, _defaultPath(path, "tex"), bolds, true);
                break;
            case Format::Excel:
            case Format::Word:
                sys_logger.warning("ECAnalyzer::report: Excel/Word output is deferred (ANALYSIS-OUT); "
                                   "use Txt/Csv/Latex, or chart().table(...).format(ChartFormat::Png) for a bitmap table.");
                break;
            }
            return *this;
        }

        // ---- housekeeping ----
        ECAnalyzer& clearMetric()
        {
            _reduce = _Q::none; _reduce_params.clear(); _reduce_is_indicator = false; _obj_index = 0; _ref.clear();
            _analysis.clear(); _analysis_params.clear();
            _significance_test = false;
            _rank_stat = false; _q_rank = _Q::none; _min_rank = true;
            _wl_stat = false;   _q_wl = _Q::none;   _min_wl = true;
            _best_stat = false; _q_best = _Q::none; _min_best = true;
            _kw_test = false;   _wsr_test = false;  _q_wsr = _Q::none;
            _friedman_test = false; _q_friedman = _Q::none;
            _nemenyi_test = false; _q_nemenyi = _Q::none; _dunn_test = false;
            clearResult();
            return *this;
        }
        ECAnalyzer& clearAll()
        {
            _problems.clear();
            _infos.clear();
            clearData();
            return *this;
        }
    };

    // ---- chart-builder 的 draw():须在 ECAnalyzer 完整之后定义 ----
    //   一份模板定义覆盖全部 9 个构建器(CRTP)。
    template <class Self>
    Self& ChartBase<Self>::draw()
    {
        _az->_drawChart(_kind, _spec, _path, _title, _hasStyle ? &_style : nullptr);
        return self();
    }
}
