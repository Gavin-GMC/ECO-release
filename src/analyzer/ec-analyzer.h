// ===========================================================================
//  ec-analyzer.h  --  performance analysis / evaluation layer of ECFlow
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
//    Format     output format (Txt / Csv / Latex functional; Excel/Word/Png deferred)
// ===========================================================================
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
#include "metriclib.h"
#include "logger.hpp"
#include "out-txt.h"
#include "out-latex.h"

namespace ECFlow
{
    class ECAnalyzer
    {
    public:
        // cross-stage scalar operator (reduce over solutions OR aggregate over repeats)
        enum class Stat { Mean, Variance, Std, Median, Mode, Smallest, Largest };
        // reduce-only set-quality indicator (needs reference point / front)
        enum class Indicator { HyperVolume, GD, IGD, PeakRatio };
        // cross-optimizer comparison
        enum class Compare { Significance, Rank, WinTieLose, Best };
        // objective sense for comparisons
        enum class Direction { MinIsBetter, MaxIsBetter };
        // output format
        enum class Format { Txt, Csv, Latex, Excel, Word, Png };

    private:
        // internal unified quantity tag (Stat + Indicator), drives the machinery
        enum class _Q { mean, variance, std_, median, mode, smallest, largest,
                        hyper_volume, gd, igd, peak_ratio, none };

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
            case Indicator::PeakRatio:   return _Q::peak_ratio;
            }
            return _Q::none;
        }

        struct PartiInfo
        {
            std::string optimizer;
            std::string tag;
            int repeats;
            PartiInfo(std::string o, std::string t, int r) : optimizer(o), tag(t), repeats(r) {}
        };

        // --- stage 1: reduction ---
        _Q _reduce = _Q::none;
        bool _reduce_is_indicator = false;
        int _obj_index = 0;                              // objective column (descriptive reduce)
        std::vector<std::vector<double>> _ref;           // per-problem reference point/front (indicator)

        // --- stage 2: aggregation columns ---
        std::vector<_Q> _analysis;

        // --- stage 3: comparisons ---
        bool _significance_test = false;
        bool _rank_stat = false;   _Q _q_rank = _Q::none;  bool _min_rank = true;
        bool _wl_stat = false;     _Q _q_wl = _Q::none;    bool _min_wl = true;
        bool _best_stat = false;   _Q _q_best = _Q::none;  bool _min_best = true;

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
        std::string buildPath(std::string problem, std::string optimizer, std::string tag, int repeat_id)
        {
            return "_log/" + optimizer + "/" + problem + "/" + optimizer + "_" + problem
                + "(" + tag + ")_" + std::to_string(repeat_id) + ".rslt";
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
            const std::string optimizer, const std::string problem, const std::string tag, const int repeats)
        {
            for (int i = 0; i < repeats; i++)
            {
                std::vector<std::vector<double>> one, oneDec;
                if (loadResult(one, oneDec, buildPath(problem, optimizer, tag, i)))
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
                        _infos[o].optimizer, _problems[p], _infos[o].tag, _infos[o].repeats);
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
            case _Q::hyper_volume: return "HV";
            case _Q::gd:           return "GD";
            case _Q::igd:          return "IGD";
            case _Q::peak_ratio:   return "peak_ratio";
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
                            _result_buffer[p][o][re] = _applyStat(_reduce, buf.data(), sol);
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
                                // standard CEC niching peak ratio (metriclib overload ②, coordinate-free):
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
            _analysis_buffer.assign(P, std::vector<std::vector<double>>(O, std::vector<double>(_analysis.size() + 15, 0.0)));

            for (size_t m = 0; m < _analysis.size(); m++)
            {
                for (int p = 0; p < P; p++)
                {
                    for (int o = 0; o < O; o++)
                    {
                        if (_result_buffer[p][o].empty())
                            _analysis_buffer[p][o][m] = NAN;
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
            int col = _analysis.size();      // p-value stored right after the stat columns
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

        void _wl_statistic()
        {
            int col = _analysis.size() + 1;
            int comp = _columnOf(_q_wl);
            bool is_larger;

            if (_significance_test)
            {
                for (size_t o = 1; o < _infos.size(); o++)
                {
                    int win = 0, lose = 0, eq = 0;
                    for (size_t p = 0; p < _problems.size(); p++)
                    {
                        if (std::abs(_analysis_buffer[p][o][_analysis.size()]) > 5e-4)   // not significant
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
            int col = _analysis.size() + 4;
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
            int ranked_id = _analysis.size() + 5;
            int average_rank_id = _analysis.size() + 6;
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
            for (auto q : _analysis) subheader.push_back(getMetricName(q));
            if (_significance_test) subheader.push_back("p-value");

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
                            : _number2string(_analysis_buffer[p][o][_analysis.size()]));
                }
            }

            if (_rank_stat)
            {
                datastrings.push_back({});
                int index = datastrings.size() - 1;
                rows.push_back("Rank");
                int ar_id = _analysis.size() + 6;
                for (size_t o = 0; o < _infos.size(); o++)
                    datastrings[index].push_back(_number2string(_analysis_buffer[0][o][ar_id])
                        + "/" + std::to_string(_infos.size()));
            }
            if (_wl_stat)
            {
                datastrings.push_back({});
                int index = datastrings.size() - 1;
                datastrings[index].push_back("-");
                int wl_id = _analysis.size() + 1;
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
            if (_best_stat)
            {
                bolds.assign(_problems.size(), {});
                int comp = _columnOf(_q_best);
                int be_id = _analysis.size() + 4;
                for (size_t p = 0; p < _problems.size(); p++)
                    for (size_t o = 0; o < _infos.size(); o++)
                        if (_analysis_buffer[p][o][be_id])
                            bolds[p].push_back(o * _analysis.size() + comp);
            }
        }

        std::string _defaultPath(std::string path, const char* ext)
        {
            dir_create("_analysis");
            if (path.empty())
                return "_analysis/" + getMetricName(_reduce) + "(" + std::to_string(time(NULL)) + ")." + ext;
            if (path.find('/') == std::string::npos && path.find('\\') == std::string::npos)
                return "_analysis/" + path + "." + ext;
            return path;
        }

    public:
        ECAnalyzer() {}
        ~ECAnalyzer() { clearAll(); }

        // ---- participants ----
        ECAnalyzer& addOptimizer(std::string optimizer, std::string tag, int repeats)
        {
            _infos.push_back(PartiInfo(optimizer, tag, repeats));
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
        // set-quality indicator; refPerProblem[p] interpretation depends on `ind`:
        //   HyperVolume : reference point            (length = objectives)
        //   GD / IGD    : row-major reference front   (length = refSize * objectives)
        //   PeakRatio   : { refer_fitness f*, peak_count, accuracy_f (ε), accuracy_d (niche radius r) }
        //                 (standard CEC niching; needs decision vars => run with logging(full result))
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
            if (_rank_stat)         _rank_statistic();
            if (_wl_stat)           _wl_statistic();
            if (_best_stat)         _best_solution_statistic();
            return *this;
        }
        ECAnalyzer& cal() { return run(); }   // backward-compatible alias

        // ---- output ----
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
            case Format::Png:
                sys_logger.warning("ECAnalyzer::report: Excel/Word/Png output is deferred (ANALYSIS-OUT); "
                                   "use Txt/Csv/Latex.");
                break;
            }
            return *this;
        }

        // ---- housekeeping ----
        ECAnalyzer& clearMetric()
        {
            _reduce = _Q::none; _reduce_is_indicator = false; _obj_index = 0; _ref.clear();
            _analysis.clear();
            _significance_test = false;
            _rank_stat = false; _q_rank = _Q::none; _min_rank = true;
            _wl_stat = false;   _q_wl = _Q::none;   _min_wl = true;
            _best_stat = false; _q_best = _Q::none; _min_best = true;
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
}
