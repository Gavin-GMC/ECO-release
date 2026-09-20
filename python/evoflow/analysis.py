"""Persisted result analysis backed by EvoFlow's native analyzer."""

from __future__ import annotations

import os
from dataclasses import dataclass
from typing import Any, Callable, Iterable, Optional, Sequence, Tuple, Union

from . import _core


PathInput = Union[str, os.PathLike]

Statistic = _core.Statistic
Indicator = _core.Indicator
Comparison = _core.Comparison
Direction = _core.Direction
ReportFormat = _core.ReportFormat
ChartKind = _core.ChartKind
ChartFormat = _core.ChartFormat
Normalization = _core.Normalization
Palette = _core.Palette
ChartLanguage = _core.ChartLanguage

ExperimentBatch = _core.ExperimentBatch
ChartArtifact = _core.ChartArtifact
ProcessHeader = _core.ProcessHeader
ProcessRecord = _core.ProcessRecord
ProcessLog = _core.ProcessLog
ProcessScanResult = _core.ProcessScanResult


_STATISTIC_IDS = {
    Statistic.MEAN: "mean",
    Statistic.VARIANCE: "variance",
    Statistic.STD: "std",
    Statistic.MEDIAN: "median",
    Statistic.MODE: "mode",
    Statistic.SMALLEST: "smallest",
    Statistic.LARGEST: "largest",
    Statistic.Q1: "q1",
    Statistic.Q3: "q3",
    Statistic.IQR: "iqr",
    Statistic.RANGE: "range",
    Statistic.SEM: "sem",
    Statistic.CV: "cv",
    Statistic.GEOMETRIC_MEAN: "geometric_mean",
    Statistic.HARMONIC_MEAN: "harmonic_mean",
}

_INDICATOR_IDS = {
    Indicator.HYPERVOLUME: "hypervolume",
    Indicator.GD: "gd",
    Indicator.IGD: "igd",
    Indicator.PEAK_RATIO: "peak_ratio",
    Indicator.PEAK_DISTANCE: "peak_distance",
    Indicator.PEAK_INACCURACY: "peak_inaccuracy",
}

_COMPARISON_IDS = {
    Comparison.SIGNIFICANCE: "significance",
    Comparison.RANK: "rank",
    Comparison.WIN_TIE_LOSE: "win_tie_lose",
    Comparison.BEST: "best",
    Comparison.KRUSKAL_WALLIS: "kruskal_wallis",
    Comparison.WILCOXON_SIGNED_RANK: "wilcoxon_signed_rank",
    Comparison.FRIEDMAN: "friedman",
    Comparison.NEMENYI: "nemenyi",
    Comparison.DUNN: "dunn",
}

_CHART_IDS = {
    ChartKind.BOX: "box",
    ChartKind.MULTI_BOX: "multi_box",
    ChartKind.BAR: "bar",
    ChartKind.MULTI_BAR: "multi_bar",
    ChartKind.CONVERGENCE: "convergence",
    ChartKind.PARETO: "pareto",
    ChartKind.TABLE: "table",
    ChartKind.WIN_TIE_LOSE: "win_tie_lose",
    ChartKind.RANK_HEATMAP: "rank_heatmap",
    ChartKind.RANK_BAR: "rank_bar",
    ChartKind.CD_DIAGRAM: "cd_diagram",
}

_NORMALIZATION_IDS = {
    Normalization.NONE: "none",
    Normalization.MIN_MAX: "min_max",
    Normalization.POOLED_RANK: "pooled_rank",
    Normalization.RATIO_TO_BEST: "ratio_to_best",
}


def _path(value: PathInput, argument: str) -> str:
    try:
        return os.fsdecode(os.fspath(value))
    except TypeError as error:
        raise TypeError(f"{argument} must be a path-like object") from error


def _enum_id(value: Any, values: dict, argument: str) -> str:
    try:
        return values[value]
    except (KeyError, TypeError) as error:
        raise TypeError(f"{argument} has the wrong enum type") from error


def _percentile(value: Any, argument: str) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise TypeError(f"{argument} must be a number")
    result = float(value)
    if not 0.0 <= result <= 1.0:
        raise ValueError(f"{argument} must be between 0 and 1")
    return result


def _index(value: Any, argument: str, *, minimum: int = 0) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise TypeError(f"{argument} must be an integer")
    if value < minimum:
        raise ValueError(f"{argument} must be at least {minimum}")
    return value


def _references(values: Sequence[Sequence[float]]) -> Tuple[Tuple[float, ...], ...]:
    try:
        result = tuple(tuple(float(item) for item in row) for row in values)
    except (TypeError, ValueError) as error:
        raise TypeError("references must be a sequence of numeric sequences") from error
    if not result or any(not row for row in result):
        raise ValueError("references cannot be empty")
    return result


@dataclass(frozen=True)
class AnalysisTable:
    """A native report table; ``bolds`` is available on advanced Analyzer tables."""

    header: Tuple[str, ...]
    subheader: Tuple[str, ...]
    rows: Tuple[str, ...]
    cells: Tuple[Tuple[str, ...], ...]
    bolds: Optional[Tuple[Tuple[int, ...], ...]] = None


@dataclass(frozen=True)
class AnalysisResult:
    warnings: Tuple[str, ...]
    batches: Tuple[ExperimentBatch, ...]
    chosen: ExperimentBatch
    table: AnalysisTable
    chart_files: Tuple[str, ...]
    charts: Tuple[ChartArtifact, ...]


class AnalysisError(RuntimeError):
    """A native batch or report validation failure."""

    def __init__(self, message: str, batches: Iterable[ExperimentBatch] = ()) -> None:
        super().__init__(message)
        self.batches = tuple(batches)


@dataclass(frozen=True)
class Reducer:
    """Reduction of one run's solution set to one scalar."""

    kind: str
    metric: Any = None
    objective: int = 0
    value: float = 0.5
    references: Tuple[Tuple[float, ...], ...] = ()

    @classmethod
    def statistic(cls, metric: Statistic, *, objective: int = 0) -> "Reducer":
        _enum_id(metric, _STATISTIC_IDS, "metric")
        return cls("statistic", metric, _index(objective, "objective"))

    @classmethod
    def percentile(cls, value: float, *, objective: int = 0) -> "Reducer":
        return cls(
            "percentile",
            None,
            _index(objective, "objective"),
            _percentile(value, "percentile"),
        )

    @classmethod
    def indicator(
        cls, metric: Indicator, references: Sequence[Sequence[float]]
    ) -> "Reducer":
        _enum_id(metric, _INDICATOR_IDS, "metric")
        return cls("indicator", metric, references=_references(references))


@dataclass(frozen=True)
class Aggregation:
    """One cross-run report column."""

    kind: str
    metric: Any = None
    value: float = 0.5

    @classmethod
    def statistic(cls, metric: Statistic) -> "Aggregation":
        _enum_id(metric, _STATISTIC_IDS, "metric")
        return cls("statistic", metric)

    @classmethod
    def percentile(cls, value: float) -> "Aggregation":
        return cls("percentile", value=_percentile(value, "percentile"))


@dataclass(frozen=True)
class ComparisonSpec:
    comparison: Comparison
    on: Statistic = Statistic.MEDIAN
    direction: Direction = Direction.MINIMIZE

    def __post_init__(self) -> None:
        _enum_id(self.comparison, _COMPARISON_IDS, "comparison")
        _enum_id(self.on, _STATISTIC_IDS, "on")
        if self.direction not in (Direction.MINIMIZE, Direction.MAXIMIZE):
            raise TypeError("direction has the wrong enum type")


@dataclass(frozen=True)
class ChartSpec:
    """A chart request for the safe batch-oriented ``analyze`` facade."""

    id: str
    kind: ChartKind
    problem: int = 0
    statistic: Statistic = Statistic.MEDIAN
    band: Optional[Tuple[Statistic, Statistic]] = None
    normalization: Normalization = Normalization.NONE
    jitter: bool = False
    grid: int = 200
    objective_x: int = 0
    objective_y: int = 1
    run: int = -1
    connect: bool = False

    def __post_init__(self) -> None:
        if not isinstance(self.id, str) or not self.id:
            raise ValueError("chart id cannot be empty")
        _enum_id(self.kind, _CHART_IDS, "kind")
        _enum_id(self.statistic, _STATISTIC_IDS, "statistic")
        _enum_id(self.normalization, _NORMALIZATION_IDS, "normalization")
        _index(self.problem, "problem")
        _index(self.grid, "grid", minimum=2)
        _index(self.objective_x, "objective_x")
        _index(self.objective_y, "objective_y")
        if isinstance(self.run, bool) or not isinstance(self.run, int) or self.run < -1:
            raise ValueError("run must be -1 or a non-negative integer")
        if self.band is not None:
            if len(self.band) != 2:
                raise ValueError("band must contain two statistics")
            _enum_id(self.band[0], _STATISTIC_IDS, "band[0]")
            _enum_id(self.band[1], _STATISTIC_IDS, "band[1]")


@dataclass(frozen=True)
class ChartStyle:
    palette: Palette = Palette.COLORBLIND_SAFE
    language: ChartLanguage = ChartLanguage.EN
    caption: bool = True
    log_scale: bool = False
    reverse_y: bool = False

    def __post_init__(self) -> None:
        if self.palette not in (
            Palette.COLORBLIND_SAFE,
            Palette.CATEGORICAL,
            Palette.GRAYSCALE,
            Palette.VIBRANT,
        ):
            raise TypeError("palette has the wrong enum type")
        if self.language not in (ChartLanguage.EN, ChartLanguage.ZH):
            raise TypeError("language has the wrong enum type")


@dataclass(frozen=True)
class PngFont:
    latin: str = "C:/Windows/Fonts/times.ttf"
    cjk: str = "C:/Windows/Fonts/simsun.ttc"
    cjk_index: int = 0
    scale: float = 2.0

    def __post_init__(self) -> None:
        _index(self.cjk_index, "cjk_index")
        if isinstance(self.scale, bool) or not isinstance(self.scale, (int, float)):
            raise TypeError("scale must be a number")
        if float(self.scale) <= 0.0:
            raise ValueError("scale must be positive")


def _table(value: Any, *, bolds: bool) -> AnalysisTable:
    return AnalysisTable(
        tuple(value.header),
        tuple(value.subheader),
        tuple(value.rows),
        tuple(tuple(row) for row in value.cells),
        tuple(tuple(row) for row in value.bolds) if bolds else None,
    )


def _native_reducer(value: Reducer) -> Any:
    if not isinstance(value, Reducer):
        raise TypeError("reducer must be a Reducer")
    result = _core._ReportReducerSpec()
    result.kind = value.kind
    result.objective = _index(value.objective, "reducer.objective")
    if value.kind == "statistic":
        result.metric = _enum_id(value.metric, _STATISTIC_IDS, "reducer.metric")
    elif value.kind == "percentile":
        result.metric = "percentile"
        result.percentile = _percentile(value.value, "reducer.value")
    elif value.kind == "indicator":
        result.metric = _enum_id(value.metric, _INDICATOR_IDS, "reducer.metric")
        result.references = [list(row) for row in value.references]
    else:
        raise ValueError("unknown reducer kind")
    return result


def _native_aggregation(value: Union[Statistic, Aggregation]) -> Any:
    if value in _STATISTIC_IDS:
        value = Aggregation.statistic(value)
    if not isinstance(value, Aggregation):
        raise TypeError("statistics entries must be Statistic or Aggregation")
    result = _core._ReportStatisticSpec()
    if value.kind == "statistic":
        result.metric = _enum_id(value.metric, _STATISTIC_IDS, "aggregation.metric")
    elif value.kind == "percentile":
        result.metric = "percentile"
        result.percentile = _percentile(value.value, "aggregation.value")
    else:
        raise ValueError("unknown aggregation kind")
    return result


def _native_comparison(value: Union[Comparison, ComparisonSpec]) -> Any:
    if value in _COMPARISON_IDS:
        value = ComparisonSpec(value)
    if not isinstance(value, ComparisonSpec):
        raise TypeError("comparisons entries must be Comparison or ComparisonSpec")
    result = _core._ReportComparisonSpec()
    result.metric = _enum_id(value.comparison, _COMPARISON_IDS, "comparison")
    result.on = _enum_id(value.on, _STATISTIC_IDS, "comparison.on")
    result.minimize = value.direction == Direction.MINIMIZE
    return result


def _native_chart(value: ChartSpec) -> Any:
    if not isinstance(value, ChartSpec):
        raise TypeError("charts entries must be ChartSpec")
    result = _core._ReportChartSpec()
    result.id = value.id
    result.kind = _enum_id(value.kind, _CHART_IDS, "chart.kind")
    result.problem = value.problem
    result.statistic = _enum_id(value.statistic, _STATISTIC_IDS, "chart.statistic")
    if value.band is not None:
        result.band_low = _enum_id(value.band[0], _STATISTIC_IDS, "chart.band[0]")
        result.band_high = _enum_id(value.band[1], _STATISTIC_IDS, "chart.band[1]")
    result.normalization = _enum_id(
        value.normalization, _NORMALIZATION_IDS, "chart.normalization"
    )
    result.jitter = value.jitter
    result.grid = value.grid
    result.objective_x = value.objective_x
    result.objective_y = value.objective_y
    result.run = value.run
    result.connect = value.connect
    return result


def discover_batches(log_path: PathInput = "_log") -> Tuple[ExperimentBatch, ...]:
    """Discover persisted experiment batches without mixing tags."""

    result = _core._discover_batches(_path(log_path, "log_path"))
    if not result.ok:
        raise AnalysisError(result.error, result.batches)
    return tuple(result.batches)


def analyze(
    log_path: PathInput = "_log",
    *,
    tag: str,
    optimizers: Sequence[str] = (),
    problems: Sequence[str] = (),
    reducer: Optional[Reducer] = None,
    statistics: Optional[Sequence[Union[Statistic, Aggregation]]] = None,
    comparisons: Optional[Sequence[Union[Comparison, ComparisonSpec]]] = None,
    baseline: Optional[str] = None,
    charts: Optional[Sequence[ChartSpec]] = None,
    charts_dir: Optional[PathInput] = None,
    chart_format: ChartFormat = ChartFormat.SVG,
    png_font: Optional[PngFont] = None,
) -> AnalysisResult:
    """Build one validated report through the same App facade as CLI and UI."""

    if not isinstance(tag, str) or not tag:
        raise ValueError("tag cannot be empty; use discover_batches() to choose one")
    if baseline is not None and not isinstance(baseline, str):
        raise TypeError("baseline must be a string or None")
    if chart_format not in (ChartFormat.SVG, ChartFormat.PNG):
        raise TypeError("chart_format has the wrong enum type")
    if charts is not None and len(charts) > 0 and charts_dir is None:
        raise ValueError("charts_dir is required when charts are requested")

    options = _core._ReportOptions()
    options.log_path = _path(log_path, "log_path")
    options.tag = tag
    options.optimizers = list(optimizers)
    options.problems = list(problems)
    options.baseline_optimizer = baseline or ""
    if charts_dir is not None:
        options.charts_dir = _path(charts_dir, "charts_dir")
    options.charts_png = chart_format == ChartFormat.PNG
    if png_font is not None:
        if not isinstance(png_font, PngFont):
            raise TypeError("png_font must be a PngFont or None")
        options.png_latin_font = png_font.latin
        options.png_cjk_font = png_font.cjk
        options.png_cjk_index = png_font.cjk_index
        options.png_scale = float(png_font.scale)

    custom = any(
        value is not None for value in (reducer, statistics, comparisons, charts)
    )
    options.custom_analysis = custom
    if custom:
        selected_reducer = reducer or Reducer.statistic(Statistic.SMALLEST)
        selected_statistics = (
            (Statistic.MEAN, Statistic.STD, Statistic.MEDIAN)
            if statistics is None
            else statistics
        )
        if comparisons is None:
            selected_comparisons = (
                (
                    ComparisonSpec(Comparison.BEST),
                    ComparisonSpec(Comparison.RANK),
                )
                if statistics is None
                else ()
            )
        else:
            selected_comparisons = comparisons
        if charts is None:
            if charts_dir is None:
                selected_charts = ()
            elif comparisons is None and statistics is None:
                selected_charts = (
                    ChartSpec("table", ChartKind.TABLE),
                    ChartSpec("dist", ChartKind.MULTI_BOX),
                    ChartSpec("rank", ChartKind.RANK_BAR),
                )
            else:
                selected_charts = (
                    ChartSpec("table", ChartKind.TABLE),
                    ChartSpec("dist", ChartKind.MULTI_BOX),
                )
        else:
            selected_charts = charts
        options.reducer = _native_reducer(selected_reducer)
        options.statistics = [_native_aggregation(item) for item in selected_statistics]
        options.comparisons = [_native_comparison(item) for item in selected_comparisons]
        options.charts = [_native_chart(item) for item in selected_charts]

    native = _core._build_report(options)
    if not native.ok:
        raise AnalysisError(native.error, native.batches)
    table = AnalysisTable(
        tuple(native.header),
        tuple(native.subheader),
        tuple(native.rows),
        tuple(tuple(row) for row in native.cells),
        None,
    )
    return AnalysisResult(
        tuple(native.warnings),
        tuple(native.batches),
        native.chosen,
        table,
        tuple(native.chart_files),
        tuple(native.chart_artifacts),
    )


class Analyzer:
    """Advanced fluent facade over the documented native ``ECAnalyzer``."""

    def __init__(self, log_root: PathInput = "_log") -> None:
        self._native = _core._Analyzer(_path(log_root, "log_root"))

    def add_optimizer(self, name: str, tag: str, repeats: int) -> "Analyzer":
        _index(repeats, "repeats", minimum=1)
        self._native.add_optimizer(name, tag, repeats)
        return self

    def add_problem(self, name: str) -> "Analyzer":
        self._native.add_problem(name)
        return self

    def add_problems(self, names: Iterable[str]) -> "Analyzer":
        for name in names:
            self.add_problem(name)
        return self

    def reduce_by(
        self,
        metric: Union[Statistic, Indicator],
        *,
        objective: int = 0,
        references: Sequence[Sequence[float]] = (),
    ) -> "Analyzer":
        if metric in _STATISTIC_IDS:
            self._native.reduce_statistic(metric, _index(objective, "objective"))
        elif metric in _INDICATOR_IDS:
            self._native.reduce_indicator(metric, [list(row) for row in _references(references)])
        else:
            raise TypeError("metric must be a Statistic or Indicator")
        return self

    def reduce_percentile(self, value: float, *, objective: int = 0) -> "Analyzer":
        self._native.reduce_percentile(
            _percentile(value, "value"), _index(objective, "objective")
        )
        return self

    def add_statistic(self, statistic: Statistic) -> "Analyzer":
        _enum_id(statistic, _STATISTIC_IDS, "statistic")
        self._native.add_statistic(statistic)
        return self

    def add_percentile(self, value: float) -> "Analyzer":
        self._native.add_percentile(_percentile(value, "value"))
        return self

    def add_comparison(
        self,
        comparison: Comparison,
        *,
        on: Statistic = Statistic.MEDIAN,
        direction: Direction = Direction.MINIMIZE,
    ) -> "Analyzer":
        _enum_id(comparison, _COMPARISON_IDS, "comparison")
        _enum_id(on, _STATISTIC_IDS, "on")
        if direction not in (Direction.MINIMIZE, Direction.MAXIMIZE):
            raise TypeError("direction has the wrong enum type")
        self._native.add_comparison(comparison, on, direction)
        return self

    def run(self) -> AnalysisTable:
        return _table(self._native.run(), bolds=True)

    def table(self) -> AnalysisTable:
        return _table(self._native.table(), bolds=True)

    def report(self, format: ReportFormat, path: PathInput) -> str:
        if format not in (ReportFormat.TXT, ReportFormat.CSV, ReportFormat.LATEX):
            raise TypeError("format has the wrong enum type")
        return self._native.report(format, _path(path, "path"))

    def set_chart_style(self, style: ChartStyle) -> "Analyzer":
        if not isinstance(style, ChartStyle):
            raise TypeError("style must be a ChartStyle")
        self._native.set_chart_style(
            style.palette,
            style.language,
            style.caption,
            style.log_scale,
            style.reverse_y,
        )
        return self

    def chart(
        self,
        kind: ChartKind,
        path: PathInput,
        *,
        title: str = "",
        problems: Optional[Union[int, Iterable[int]]] = None,
        normalization: Normalization = Normalization.NONE,
        jitter: bool = False,
        statistic: Optional[Statistic] = None,
        band: Optional[Tuple[Statistic, Statistic]] = None,
        grid: int = 200,
        objective_x: int = 0,
        objective_y: int = 1,
        run: int = -1,
        swarm: str = "",
        connect: bool = False,
        format: ChartFormat = ChartFormat.SVG,
        style: Optional[ChartStyle] = None,
        png_font: Optional[PngFont] = None,
    ) -> Tuple[str, ...]:
        _enum_id(kind, _CHART_IDS, "kind")
        _enum_id(normalization, _NORMALIZATION_IDS, "normalization")
        if format not in (ChartFormat.SVG, ChartFormat.PNG):
            raise TypeError("format has the wrong enum type")
        options = _core._ChartOptions()
        options.kind = kind
        options.path = _path(path, "path")
        options.title = title
        if problems is None:
            options.problems = []
        elif isinstance(problems, int) and not isinstance(problems, bool):
            options.problems = [problems]
        else:
            try:
                options.problems = list(problems)
            except TypeError as error:
                raise TypeError("problems must be an integer, iterable, or None") from error
        options.normalization = normalization
        options.jitter = bool(jitter)
        if statistic is not None:
            _enum_id(statistic, _STATISTIC_IDS, "statistic")
            options.has_statistic = True
            options.statistic = statistic
        if band is not None:
            if len(band) != 2:
                raise ValueError("band must contain two statistics")
            _enum_id(band[0], _STATISTIC_IDS, "band[0]")
            _enum_id(band[1], _STATISTIC_IDS, "band[1]")
            options.has_band = True
            options.band_low = band[0]
            options.band_high = band[1]
        options.grid = _index(grid, "grid", minimum=2)
        options.objective_x = _index(objective_x, "objective_x")
        options.objective_y = _index(objective_y, "objective_y")
        if isinstance(run, bool) or not isinstance(run, int) or run < -1:
            raise ValueError("run must be -1 or a non-negative integer")
        options.run = run
        options.swarm = swarm
        options.connect = bool(connect)
        options.format = format
        if style is not None:
            if not isinstance(style, ChartStyle):
                raise TypeError("style must be a ChartStyle or None")
            options.has_style = True
            options.palette = style.palette
            options.language = style.language
            options.caption = style.caption
            options.log_scale = style.log_scale
            options.reverse_y = style.reverse_y
        if png_font is not None:
            if not isinstance(png_font, PngFont):
                raise TypeError("png_font must be a PngFont or None")
            native_font = _core._PngFontConfig()
            native_font.latin = png_font.latin
            native_font.cjk = png_font.cjk
            native_font.cjk_index = png_font.cjk_index
            native_font.scale = float(png_font.scale)
            options.png_font = native_font
        return tuple(self._native.chart(options))

    def clear_metrics(self) -> "Analyzer":
        self._native.clear_metrics()
        return self

    def clear_all(self) -> "Analyzer":
        self._native.clear_all()
        return self


def read_process(path: PathInput) -> ProcessLog:
    """Read a v1.1/v1.2 process trace into memory through ``PrcsReader``."""

    return _core._read_process(_path(path, "path"))


def scan_process(path: PathInput, callback: Callable[[ProcessRecord], Any]) -> ProcessScanResult:
    """Stream process records through the native reader without retaining the file."""

    if not callable(callback):
        raise TypeError("callback must be callable")
    return _core._scan_process(_path(path, "path"), callback)


__all__ = [
    "Aggregation",
    "AnalysisError",
    "AnalysisResult",
    "AnalysisTable",
    "Analyzer",
    "ChartArtifact",
    "ChartFormat",
    "ChartKind",
    "ChartLanguage",
    "ChartSpec",
    "ChartStyle",
    "Comparison",
    "ComparisonSpec",
    "Direction",
    "ExperimentBatch",
    "Indicator",
    "Normalization",
    "Palette",
    "PngFont",
    "ProcessHeader",
    "ProcessLog",
    "ProcessRecord",
    "ProcessScanResult",
    "Reducer",
    "ReportFormat",
    "Statistic",
    "analyze",
    "discover_batches",
    "read_process",
    "scan_process",
]
