# 12 · Result Analysis with ECAnalyzer

[English](12-analysis.md) | [简体中文](../手册/12-使用-结果分析.md) | [Manual index](README.md)

Combine results from multiple algorithms and problems into performance tables with descriptive statistics, significance tests, ranks, win/tie/loss counts, and best-value highlighting. Export TXT, CSV, and LaTeX. See `tutorial/4.log-analysis.cpp`.

The analysis layer is not included by `evoflow.h`; additionally include `ec-analyzer.h`.

## 1. Input: `.rslt` logs

Analysis reads saved results rather than rerunning optimization. Enable result logging during experiments:

```cpp
cb.name("psoA").tag("sphere").logging(true);
Optimizer* opt = cb.buildOptimizer();
opt->setProblem(&p);
opt->exe(20, seed);
```

The layout is `_log/<optimizer>/<problem>/<optimizer>_<problem>(<tag>)_<run>.rslt`. `addOptimizer(name,tag,runs)` must match the recorded experiment. Always use an explicit shared tag for comparisons; an omitted tag can become a timestamp and separate invocations into different batches. Fresh results may begin at zero; repeated runs can continue numbering according to the execution entry point.

Default log root is `_log`. Before adding participants, use `az.logRoot("D:/experiments/run-01")` for another root; both `.rslt` and `.prcs` use that layout. Empty paths fall back to `_log`. CLI/GUI report facades pass their selected root to this API.

## 2. Three stages

| Stage | Method | Meaning |
|---|---|---|
| Reduction | One `reduceBy(...)` | Turn a run's solution set into one scalar |
| Aggregation | Multiple `addStatistic(...)` | Aggregate run scalars into table columns |
| Comparison | `addComparison(...)` | Compare algorithms: tests, ranks, wins/losses, best highlighting |

Then `run()` computes and `report(format,path)` exports. Configuration methods return the analyzer for chaining.

## 3. Enumerations

| Enum | Members | Stage |
|---|---|---|
| `Stat` | Mean, Variance, Std, Median, Mode, Smallest, Largest, Q1, Q3, IQR, Range, SEM, CV, GeometricMean, HarmonicMean | Reduction and aggregation |
| `Indicator` | HyperVolume, GD, IGD, PeakRatio, PeakDistance, PeakInaccuracy | Reduction; requires suitable references, and decision data for niching |
| `Compare` | Significance, Rank, WinTieLose, Best, KruskalWallis, WilcoxonSignedRank, Friedman, Nemenyi, Dunn | Comparison |
| `Direction` | MinIsBetter, MaxIsBetter | Comparison direction |
| `Format` | Txt, Csv, Latex available; Excel, Word, Png deferred for table `report()` | Table output |

Parameterized percentiles use `reduceByPercentile(p)` or `addPercentile(p)`. The same `Stat` enum has different scope: `reduceBy(Mean)` averages a run's solutions, while `addStatistic(Mean)` averages runs. Chart PNG export is separately available through `ChartFormat::Png`.

## 4. API example

```cpp
ECAnalyzer az;
az.logRoot("_log")
  .addOptimizer("psoA", "sphere", 20)
  .addOptimizer("psoB", "sphere", 20)
  .addProblem("sphere");
az.reduceBy(ECAnalyzer::Stat::Mean, 0);
az.addStatistic(ECAnalyzer::Stat::Mean)
  .addStatistic(ECAnalyzer::Stat::Std)
  .addStatistic(ECAnalyzer::Stat::Smallest);
az.addComparison(ECAnalyzer::Compare::Significance)
  .addComparison(ECAnalyzer::Compare::Rank, ECAnalyzer::Stat::Mean,
                 ECAnalyzer::Direction::MinIsBetter)
  .addComparison(ECAnalyzer::Compare::WinTieLose, ECAnalyzer::Stat::Mean,
                 ECAnalyzer::Direction::MinIsBetter)
  .addComparison(ECAnalyzer::Compare::Best, ECAnalyzer::Stat::Mean,
                 ECAnalyzer::Direction::MinIsBetter);
az.run().report(ECAnalyzer::Format::Txt, "result")
        .report(ECAnalyzer::Format::Csv, "result")
        .report(ECAnalyzer::Format::Latex, "result");
```

`addProblem` also accepts a list. The comparison `on` statistic chooses a representative aggregation column; Significance ignores it and compares reduced run samples directly. Default direction is smaller-is-better.

A bare output name becomes `_analysis/<name>.<extension>`; an empty name generates a reduction-name/timestamp filename. Missing extensions are added; conflicting suffixes are corrected with a warning. `_analysis` is created automatically, but callers must prepare parent directories for explicit paths. Reports contain per-problem statistics, baseline p-values, average-rank rows, win/tie/loss counts, and LaTeX `\textbf` best-value marking.

## 5. Quality indicators

Reduce an entire Pareto solution set to one scalar with per-problem references:

| Indicator | Meaning | `refPerProblem[p]` |
|---|---|---|
| HyperVolume | Dominated volume; larger is better | One reference point with one poor/upper value per objective |
| GD | Generational distance; smaller is better | Reference front flattened row-major: points × objectives |
| IGD | Inverted generational distance; convergence and coverage | Same layout as GD |
| PeakRatio | Fraction of known global optima found; larger is better | `{f_star, peak_count, epsilon, radius}` |

```cpp
az.reduceBy(ECAnalyzer::Indicator::HyperVolume, {{5.0, 5.0}});
std::vector<double> front = { /* f1,f2 for each reference point */ };
az.reduceBy(ECAnalyzer::Indicator::IGD, {front});
```

Use a `Multiobject` archive to record the whole front. Aggregate indicator values across runs with `addStatistic`, for example Mean.

### 5.1 PeakRatio

Niching metrics work in decision space: retain solutions whose fitness is within epsilon of `f_star`, deduplicate by niche radius, then divide distinct hits by known peak count.

```cpp
az.reduceBy(ECAnalyzer::Indicator::PeakRatio,
            {{f_star, (double)peaks, 1e-1, rho}});
```

The external benchmark/problem definition supplies optimum fitness, peak count, and radius; the caller chooses epsilon. Full result logging is required so `.rslt` v1.1 contains decision coordinates; without them PeakRatio returns empty. `logging(true)` enables the documented full result path. Use `Multimodal` archives to retain multiple peaks. Ratios are clamped to `[0,1]` before across-run aggregation. See the external-benchmark tutorial and synthetic two-peak tests.

## 6. Nonparametric tests

| Samples | Two-group test | K-group omnibus | Post-hoc |
|---|---|---|---|
| Independent run distributions, per problem | Significance (Mann–Whitney U) | KruskalWallis | Dunn |
| Paired representative values, blocked by problem | WilcoxonSignedRank | Friedman | Nemenyi |

Independent tests use reduced run samples. Paired tests use one aggregation value per algorithm/problem, selected by `on=Stat`. An omnibus test detects an overall difference; post-hoc comparisons identify which pairs differ.

Per-problem results appear as table columns such as `KW_p`/`Dunn_p`. Cross-problem tests produce summary rows: Friedman χ²/p, Nemenyi critical difference/average ranks, and Wilcoxon versus baseline.

```cpp
az.reduceBy(ECAnalyzer::Stat::Smallest)
  .addStatistic(ECAnalyzer::Stat::Median)
  .addComparison(ECAnalyzer::Compare::KruskalWallis)
  .addComparison(ECAnalyzer::Compare::Dunn)
  .addComparison(ECAnalyzer::Compare::Friedman, ECAnalyzer::Stat::Median)
  .addComparison(ECAnalyzer::Compare::Nemenyi, ECAnalyzer::Stat::Median)
  .addComparison(ECAnalyzer::Compare::WilcoxonSignedRank, ECAnalyzer::Stat::Median);
```

The implementation's documented reference conventions are SciPy for exact small-sample MW/Wilcoxon distributions, χ² survival functions for KW/Friedman, and large-sample normal approximations; Demšar 2006 for Nemenyi critical differences; scikit-posthocs conventions for Dunn.

## 7. Charts

SVG is a vector output written directly by `out-svg.h`, without a rendering-library dependency. Typed builders expose only options valid for each chart.

### 7.1 Fluent chart builders

```cpp
az.chartStyle(FileOut::ChartStyle()
    .palette(FileOut::Palette::ColorblindSafe)
    .lang(FileOut::Lang::EN).caption(true));
az.chart().boxPlot("box", "Problem distributions").jitter(true).draw();
az.chart().convergence("conv", "Convergence").grid(400)
    .band(ECAnalyzer::Stat::Q1, ECAnalyzer::Stat::Q3).draw();
az.chart().paretoFront("pf", "Pareto front").objectives(0,1).connect(true).draw();
az.chart().rankBar("rb", "Average ranks").draw();
```

| Method | Specific options | Prerequisite |
|---|---|---|
| `boxPlot()` | `jitter`, `norm` | Per-problem data |
| `barChart()` | `norm`, `statistic`, `band/noBand` | Per-problem data |
| `convergence()` | `grid`, `statistic`, `band`, `swarm` | `.prcs` history from `logging(_,true)` |
| `paretoFront()` | `objectives`, `run`, `connect` | At least two objectives |
| `multiBoxPlot()` / `multiBarChart()` | Corresponding options above | Cross-problem data |
| `cdDiagram()` | — | Nemenyi comparison |
| `winTieLose()` | — | WinTieLose comparison |
| `rankHeatmap()` | `statistic` | Rank comparison or an explicit statistic |
| `rankBar()` | — | Rank or Nemenyi comparison |
| `table()` | — | Computed report |

Per-problem builders default to one chart per problem, adding problem suffixes, unless `problem(i)`/`problems({...})` selects a subset. Cross-problem builders create one combined figure. Shared methods are `path`, `title`, `style`, `format`, and `pngFont`, in any order. `ChartFormat::Svg` is default. Missing prerequisites log an error and refuse output rather than create an empty chart.

`draw()` returns the builder, allowing redraws:

```cpp
auto b = az.chart().multiBoxPlot("box", "Raw data");
b.draw();
b.norm(chart_detail::Norm::MinMax).path("box_mm").title("Normalized").draw();
```

Drawing is explicit, not a destructor side effect. Builders hold an `ECAnalyzer*`, not a pointer to the temporary `chart()` facade, so they can safely span statements while the analyzer remains alive.

Invalid options fail to compile: box plots have no `grid`, rank bars no `norm`, cross-problem tables no `problem`, convergence no `jitter`, and win/tie/loss charts no `objectives`.

Style is shared across the analyzer; titles are per figure. Built-in palettes are ColorblindSafe (default, sixteen colors from Okabe–Ito and Tol muted), Categorical, Grayscale, and Vibrant. Color follows algorithm insertion index, preserving identity across charts. Framework labels default to English and support Chinese; user algorithm names are not translated. Primitive bounds beyond the canvas generate warnings.

Default charts show raw data: normalization, log scaling, Y reversal, jitter, and bands are off. Statistics needed only by a chart are computed into temporary buffers without adding columns to `report()`.

**Convergence:** the X-axis is FES, not generation. Enable `cb.logging(true,true)`. A common per-problem FES grid supports comparisons, with logarithmic sampling when requested. Resampling uses step hold; before the first record values are NaN and lines remain broken. Two scans bound memory by grid points × runs, independent of generation count. Direction follows the problem comparator, including maximization. Only `.prcs` v1.1/v1.2 are supported; older formats require rerunning.

Population-level history `_<run>.prcs` records epochs and unified FES; subpopulation history `_<run>_<id>.prcs` records generations. Default convergence reads population history; `.swarm("s0")` selects finer subpopulation data. A single-epoch configuration where local budget equals global budget may have only one global point.

**Pareto fronts:** points come directly from the archive. A Multiobject archive is already nondominated, so the chart does not resort it. Connecting lines default off because disconnected/discrete fronts have no real intermediate solutions. When enabled, captions identify lines as visual aids.

### 7.2 Viewing SVG

Open an SVG in a browser/viewer or embed it as an image. It remains sharp under zoom. GUI integrations may add hover/zoom controls without recomputing chart data.

### 7.3 LaTeX

pdfLaTeX does not directly consume SVG through `includegraphics`. Convert to vector PDF, for example with Inkscape, then include it:

```sh
inkscape chart.svg --export-type=pdf
```

```latex
\usepackage{graphicx}
\includegraphics[width=\linewidth]{chart.pdf}
```

Alternatively, the `svg` package uses installed Inkscape with shell escape enabled:

```latex
\usepackage{svg}
\includesvg[width=\linewidth]{chart}
```

SVG-to-TikZ conversion is another option; fidelity depends on complexity. Vector PDF is appropriate for scalable publication figures; raster PNG serves bitmap-only destinations. Table LaTeX output and chart PDF can be used together in one document.

### 7.4 PNG

```cpp
az.chart().boxPlot("box", "Distribution").format(ChartFormat::Png).draw();
auto b = az.chart().rankBar("rb", "Average rank");
b.draw(); // SVG
b.format(ChartFormat::Png).draw(); // PNG
```

Format selection produces one format per draw, not both. SVG and PNG replay the same `ChartCanvas` primitive list; PNG does not parse SVG. `stb_truetype` renders text, choosing the Latin font below code point `0x80` and the CJK font otherwise. `PngFontConfig::scale` defaults to 2.0; `stb_image_write` handles compressed output. The documented example outputs were approximately 20–74 KB, depending on content.

Default Windows paths are `C:/Windows/Fonts/times.ttf` and `C:/Windows/Fonts/simsun.ttc`. Fonts are loaded from the user's system and are not redistributed. Systems without these files require explicit readable font paths:

```cpp
FileOut::PngFontConfig f;
f.latin = "/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf";
f.cjk = "/usr/share/fonts/opentype/noto/NotoSerifCJK-Regular.ttc";
az.chart().boxPlot("box", "Distribution")
    .format(ChartFormat::Png).pngFont(f).draw();
```

Linux English PNG output was validated with explicitly supplied DejaVu fonts on Ubuntu WSL2/CPython 3.12.3. Missing fonts produce `no usable font: ...` and no image rather than silent text omission. Automatic font discovery/substitution and macOS remain outside current validation.

## 8. Available features and limits

Available: descriptive statistics and percentiles; all listed comparisons; HV/GD/IGD/PeakRatio/PeakDistance/PeakInaccuracy; TXT/CSV/LaTeX reports; SVG/PNG charts. `Format::Excel/Word/Png` table-report modes are deferred and issue a warning; do not confuse these with chart PNG support.

The metriclib dependency contains non-inline definitions in headers. Include `ec-analyzer.h` in only one translation unit per executable to avoid multiple definitions.

## 9. Internals

`src/analyzer/ec-analyzer.h` loads `.rslt`, reduces via `_metric_cal`, aggregates via `_metric_analysis`, performs comparisons, builds `_analysis_table`, and exports through `report`. Comparison methods include `_significant`, `_rank`, `_wl`, `_best`, `_kruskal`, `_wilcoxon`, `_friedman`, `_nemenyi`, and `_dunn_statistic`. Named `ExtraCol` values replace magic offsets.

`out/out-txt.h` handles TXT/CSV and `out-latex.h` handles LaTeX through `FileOut::writeTableToXxx`, consuming a string table and bold flags. Statistical mathematics is supplied by metriclib in namespace `MetricLib`.
