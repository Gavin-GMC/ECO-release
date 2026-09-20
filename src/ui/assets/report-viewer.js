(() => {
  "use strict";

  const ui = window.EvoFlowUi;
  if (!ui) return;

  const byId = (id) => document.getElementById(id);
  const element = (tag, className = "", text = "") => {
    const node = document.createElement(tag);
    if (className) node.className = className;
    if (text) node.textContent = text;
    return node;
  };
  const option = (value, label) => {
    const node = document.createElement("option");
    node.value = value;
    node.textContent = label;
    return node;
  };
  const fillSelect = (select, items, selected) => {
    select.replaceChildren(...items.map((item) => option(item.value, item.label)));
    if (items.some((item) => item.value === selected)) select.value = selected;
  };

  const STATISTICS = [
    ["mean", "Mean · 均值"],
    ["variance", "Variance · 方差"],
    ["std", "Std · 标准差"],
    ["median", "Median · 中位数"],
    ["mode", "Mode · 众数"],
    ["smallest", "Smallest · 最小值"],
    ["largest", "Largest · 最大值"],
    ["q1", "Q1 · 下四分位"],
    ["q3", "Q3 · 上四分位"],
    ["iqr", "IQR · 四分位距"],
    ["range", "Range · 极差"],
    ["sem", "SEM · 均值标准误"],
    ["cv", "CV · 变异系数"],
    ["geometric_mean", "Geometric mean · 几何均值"],
    ["harmonic_mean", "Harmonic mean · 调和均值"]
  ].map(([value, label]) => ({ value, label }));
  const INDICATORS = [
    ["hypervolume", "HyperVolume"],
    ["gd", "GD"],
    ["igd", "IGD"],
    ["peak_ratio", "Peak Ratio"],
    ["peak_distance", "Peak Distance"],
    ["peak_inaccuracy", "Peak Inaccuracy"]
  ].map(([value, label]) => ({ value, label }));
  const COMPARISONS = [
    ["best", "Best · 最优标记"],
    ["rank", "Rank · 排名"],
    ["win_tie_lose", "Win / Tie / Lose"],
    ["significance", "Significance · 显著性"],
    ["kruskal_wallis", "Kruskal–Wallis"],
    ["wilcoxon_signed_rank", "Wilcoxon signed-rank"],
    ["friedman", "Friedman"],
    ["nemenyi", "Nemenyi"],
    ["dunn", "Dunn"]
  ].map(([value, label]) => ({ value, label }));
  const CHARTS = [
    ["table", "统计表图"],
    ["box", "单问题箱线图"],
    ["multi_box", "跨问题箱线图"],
    ["bar", "单问题统计柱状图"],
    ["multi_bar", "跨问题统计柱状图"],
    ["rank_bar", "平均秩柱状图"],
    ["rank_heatmap", "排名热力图"],
    ["win_tie_lose", "胜 / 平 / 负图"],
    ["cd_diagram", "Nemenyi 临界差异图"],
    ["convergence", "收敛曲线"],
    ["pareto", "Pareto 前沿"]
  ].map(([value, label]) => ({ value, label }));
  const NORMALIZATIONS = [
    { value: "none", label: "原始值" },
    { value: "min_max", label: "Min–max" },
    { value: "pooled_rank", label: "合并秩" },
    { value: "ratio_to_best", label: "相对最优比" }
  ];
  const BANDS = [
    { value: "", label: "不显示区间" },
    { value: "q1|q3", label: "Q1 – Q3" },
    { value: "smallest|largest", label: "最小值 – 最大值" }
  ];

  const labelOf = (catalog, value) =>
    catalog.find((item) => item.value === value)?.label || value;
  const comparisonUsesSource = (metric) =>
    !["significance", "kruskal_wallis", "dunn"].includes(metric);
  const comparisonUsesDirection = (metric) =>
    ["best", "rank", "win_tie_lose"].includes(metric);
  const namedStatistics = () =>
    state.analysis.statistics
      .filter((item) => item.metric !== "percentile")
      .map((item) => ({
        value: item.metric,
        label: labelOf(STATISTICS, item.metric)
      }));
  const selectedValues = (containerId) =>
    Array.from(byId(containerId).querySelectorAll('input[type="checkbox"]:checked'))
      .map((input) => input.value);

  const defaultAnalysis = () => ({
    reducer: { kind: "statistic", metric: "smallest", objective: 0, percentile: 0.5 },
    statistics: [
      { metric: "mean", percentile: 0.5 },
      { metric: "std", percentile: 0.5 },
      { metric: "median", percentile: 0.5 }
    ],
    comparisons: [
      { metric: "best", on: "median", direction: "min" },
      { metric: "rank", on: "median", direction: "min" }
    ],
    charts: [
      newChart("table", "table"),
      newChart("multi_box", "dist"),
      newChart("rank_bar", "rank")
    ],
    references: {},
    nextChartId: 1
  });

  function newChart(kind, id = "") {
    return {
      id,
      kind,
      problem: "",
      statistic: "median",
      normalization: "none",
      band: "",
      jitter: false,
      grid: 200,
      objectiveX: 0,
      objectiveY: 1,
      run: -1,
      connect: false
    };
  }

  const state = {
    batches: [],
    batch: null,
    path: "",
    scanned: false,
    report: null,
    analysis: null
  };
  state.analysis = defaultAnalysis();

  const message = byId("report-message");
  const pathInput = byId("report-log-path");
  const scanButton = byId("report-scan");
  const generateButton = byId("report-generate");
  const formHeaders = { "Content-Type": "application/x-www-form-urlencoded;charset=UTF-8" };

  const showMessage = (text, kind = "info") => {
    message.hidden = false;
    message.className = `message${kind === "error" ? " error" : ""}`;
    message.textContent = text;
  };
  const hideMessage = () => {
    message.hidden = true;
    message.textContent = "";
  };
  const setBusy = (button, busy, busyText, readyText) => {
    button.disabled = busy;
    button.textContent = busy ? busyText : readyText;
  };

  const selectedProblems = () => selectedValues("report-problem-options");
  const selectedOptimizers = () => selectedValues("report-optimizer-options");

  const appendAnalysisForm = (form) => {
    const plan = state.analysis;
    form.set("analysis", "custom");
    form.set("reducerKind", plan.reducer.kind);
    form.set("reducerMetric", plan.reducer.metric);
    form.set("reducerObjective", String(plan.reducer.objective));
    form.set("reducerPercentile", String(plan.reducer.percentile));
    if (plan.reducer.kind === "indicator")
      selectedProblems().forEach((problem) =>
        form.append("reference", plan.references[problem] || ""));
    plan.statistics.forEach((statistic) => {
      const value = statistic.metric === "percentile"
        ? `percentile:${statistic.percentile}` : statistic.metric;
      form.append("aggregate", value);
    });
    plan.comparisons.forEach((comparison) =>
      form.append(
        "comparison",
        `${comparison.metric}|${comparison.on}|${comparison.direction}`
      ));
    const problems = selectedProblems();
    plan.charts.forEach((chart) => {
      const problemIndex = Math.max(0, problems.indexOf(chart.problem));
      const [bandLow = "", bandHigh = ""] = chart.band.split("|");
      form.append("chart", [
        chart.id,
        chart.kind,
        problemIndex,
        chart.statistic,
        chart.normalization,
        bandLow,
        bandHigh,
        chart.jitter ? 1 : 0,
        chart.grid,
        chart.objectiveX,
        chart.objectiveY,
        chart.run,
        chart.connect ? 1 : 0
      ].join("|"));
    });
  };

  const buildForm = (extra = {}) => {
    const form = new URLSearchParams();
    form.set("path", state.path);
    if (state.batch) form.set("tag", state.batch.tag);
    selectedOptimizers().forEach((value) => form.append("optimizer", value));
    selectedProblems().forEach((value) => form.append("problem", value));
    const baseline = byId("report-baseline").value;
    if (baseline) form.set("baseline", baseline);
    form.set("latinFont", byId("report-latin-font").value.trim());
    form.set("cjkFont", byId("report-cjk-font").value.trim());
    form.set("scale", byId("report-png-scale").value);
    appendAnalysisForm(form);
    Object.entries(extra).forEach(([key, value]) => form.set(key, value));
    return form;
  };

  const renderBatchList = () => {
    const host = byId("report-batches");
    host.replaceChildren();
    byId("report-batch-count").textContent = state.batches.length;
    if (!state.batches.length) {
      host.appendChild(element("div", "empty-state compact", "没有发现 .rslt 结果文件。"));
      return;
    }
    state.batches.forEach((batch) => {
      const button = element("button", "batch-card");
      button.type = "button";
      button.dataset.tag = batch.tag;
      button.setAttribute("aria-pressed", state.batch?.tag === batch.tag ? "true" : "false");
      const top = element("div", "batch-card-top");
      top.append(element("strong", "", batch.tag));
      if (batch.ragged) top.append(element("span", "status-badge warning", "数据不齐"));
      const meta = element("div", "batch-meta");
      meta.append(
        element("span", "", `${batch.optimizers.length} 优化器`),
        element("span", "", `${batch.problems.length} 问题`),
        element("span", "", `${batch.repeats} 次共同运行`),
        element("span", "", `${batch.rsltCount} 文件`)
      );
      button.append(top, meta);
      if ((batch.runDiagnostics || []).length) {
        button.append(element(
          "div", "batch-run-diagnostics", batch.runDiagnostics.join(" · ")
        ));
      }
      button.addEventListener("click", () => selectBatch(batch));
      host.appendChild(button);
    });
  };

  const renderChoices = (containerId, values, kind) => {
    const host = byId(containerId);
    host.replaceChildren();
    values.forEach((value, index) => {
      const label = element("label", "choice-item");
      const input = document.createElement("input");
      input.type = "checkbox";
      input.name = `report-${kind}`;
      input.value = value;
      input.checked = true;
      input.addEventListener("change", () => {
        if (kind === "problem") {
          normalizeChartProblems();
          renderReducer();
          renderChartsSettings();
        } else {
          renderBaseline();
        }
        updateGenerateState();
      });
      label.append(input, element("span", "", value));
      if (index === 0) label.classList.add("first-choice");
      host.appendChild(label);
    });
  };

  const renderBaseline = () => {
    const select = byId("report-baseline");
    const previous = select.value;
    const items = selectedOptimizers().map((value) => ({ value, label: value }));
    fillSelect(select, items, previous || items[0]?.value || "");
  };

  const normalizeChartProblems = () => {
    const problems = selectedProblems();
    state.analysis.charts.forEach((chart) => {
      if (!problems.includes(chart.problem)) chart.problem = problems[0] || "";
    });
  };

  const renderReducer = () => {
    const plan = state.analysis;
    const kind = byId("report-reducer-kind");
    kind.value = plan.reducer.kind;
    const metricField = byId("report-reducer-metric-field");
    const objectiveField = byId("report-reducer-objective-field");
    const percentileField = byId("report-reducer-percentile-field");
    const metric = byId("report-reducer-metric");
    metricField.hidden = plan.reducer.kind === "percentile";
    objectiveField.hidden = plan.reducer.kind === "indicator";
    percentileField.hidden = plan.reducer.kind !== "percentile";
    if (plan.reducer.kind !== "percentile") {
      const catalog = plan.reducer.kind === "indicator" ? INDICATORS : STATISTICS;
      if (!catalog.some((item) => item.value === plan.reducer.metric))
        plan.reducer.metric = catalog[0].value;
      fillSelect(metric, catalog, plan.reducer.metric);
    }
    byId("report-reducer-objective").value = plan.reducer.objective;
    byId("report-reducer-percentile").value = plan.reducer.percentile;

    const references = byId("report-reference-fields");
    references.hidden = plan.reducer.kind !== "indicator";
    references.replaceChildren();
    if (plan.reducer.kind === "indicator") {
      references.appendChild(element(
        "div", "analysis-empty",
        "按所选问题填写参考点、参考前沿或峰值数据；可用空格、逗号或换行分隔。"
      ));
      selectedProblems().forEach((problem) => {
        const label = element("label", "reference-field", problem);
        const textarea = document.createElement("textarea");
        textarea.value = plan.references[problem] || "";
        textarea.placeholder = "输入该问题的参考数据";
        textarea.addEventListener("input", () => {
          plan.references[problem] = textarea.value;
          updateGenerateState();
        });
        label.appendChild(textarea);
        references.appendChild(label);
      });
    }
  };

  const removeButton = (handler) => {
    const button = element("button", "remove-analysis-item", "×");
    button.type = "button";
    button.title = "移除";
    button.addEventListener("click", handler);
    return button;
  };

  const renderStatistics = () => {
    const host = byId("report-statistics");
    host.replaceChildren();
    state.analysis.statistics.forEach((statistic, index) => {
      const row = element("div", "analysis-item");
      const title = statistic.metric === "percentile"
        ? `Percentile · P${Math.round(statistic.percentile * 100)}`
        : labelOf(STATISTICS, statistic.metric);
      row.appendChild(element("strong", "", title));
      const controls = element("div", "analysis-item-controls");
      if (statistic.metric === "percentile") {
        const label = element("label", "field", "分位点");
        const input = document.createElement("input");
        input.type = "number";
        input.min = "0";
        input.max = "1";
        input.step = "0.01";
        input.value = statistic.percentile;
        input.addEventListener("input", () => {
          statistic.percentile = Number(input.value);
          renderComparisons();
          renderChartsSettings();
          updateGenerateState();
        });
        label.appendChild(input);
        controls.appendChild(label);
      }
      controls.appendChild(removeButton(() => {
        state.analysis.statistics.splice(index, 1);
        const available = namedStatistics();
        state.analysis.comparisons = state.analysis.comparisons.filter((comparison) =>
          !comparisonUsesSource(comparison.metric)
          || available.some((item) => item.value === comparison.on));
        state.analysis.charts.forEach((chart) => {
          if (!available.some((item) => item.value === chart.statistic))
            chart.statistic = available[0]?.value || "";
        });
        renderStatistics();
        renderComparisons();
        renderChartsSettings();
        updateGenerateState();
      }));
      row.appendChild(controls);
      host.appendChild(row);
    });
    if (!state.analysis.statistics.length)
      host.appendChild(element("div", "analysis-empty", "至少添加一个跨轮统计指标。"));
  };

  const renderComparisons = () => {
    const host = byId("report-comparisons");
    host.replaceChildren();
    const sources = namedStatistics();
    state.analysis.comparisons.forEach((comparison, index) => {
      if (comparisonUsesSource(comparison.metric)
        && !sources.some((item) => item.value === comparison.on))
        comparison.on = sources[0]?.value || "";
      const row = element("div", "analysis-item");
      row.appendChild(element("strong", "", labelOf(COMPARISONS, comparison.metric)));
      const controls = element("div", "analysis-item-controls");
      if (comparisonUsesSource(comparison.metric)) {
        const sourceLabel = element("label", "field", "依据");
        const source = document.createElement("select");
        fillSelect(source, sources, comparison.on);
        source.addEventListener("change", () => { comparison.on = source.value; });
        sourceLabel.appendChild(source);
        controls.appendChild(sourceLabel);
      }
      if (comparisonUsesDirection(comparison.metric)) {
        const directionLabel = element("label", "field", "方向");
        const direction = document.createElement("select");
        fillSelect(direction, [
          { value: "min", label: "越小越优" },
          { value: "max", label: "越大越优" }
        ], comparison.direction);
        direction.addEventListener("change", () => { comparison.direction = direction.value; });
        directionLabel.appendChild(direction);
        controls.appendChild(directionLabel);
      }
      controls.appendChild(removeButton(() => {
        state.analysis.comparisons.splice(index, 1);
        renderComparisons();
        updateGenerateState();
      }));
      row.appendChild(controls);
      host.appendChild(row);
    });
    if (!state.analysis.comparisons.length)
      host.appendChild(element("div", "analysis-empty", "未添加优化器比较指标；报告仍可生成。"));
  };

  const chartControl = (labelText, items, selected, onChange, className = "") => {
    const label = element("label", `field ${className}`.trim(), labelText);
    const select = document.createElement("select");
    fillSelect(select, items, selected);
    select.addEventListener("change", () => onChange(select.value));
    label.appendChild(select);
    return label;
  };

  const numberControl = (labelText, value, min, onChange) => {
    const label = element("label", "field", labelText);
    const input = document.createElement("input");
    input.type = "number";
    input.min = String(min);
    input.step = "1";
    input.value = value;
    input.addEventListener("input", () => onChange(Number(input.value)));
    label.appendChild(input);
    return label;
  };

  const toggleControl = (labelText, checked, onChange) => {
    const label = element("label", "field toggle-control");
    const input = document.createElement("input");
    input.type = "checkbox";
    input.checked = checked;
    input.addEventListener("change", () => onChange(input.checked));
    label.append(input, document.createTextNode(labelText));
    return label;
  };

  const renderChartsSettings = () => {
    normalizeChartProblems();
    const host = byId("report-chart-settings");
    host.replaceChildren();
    const problems = selectedProblems().map((value) => ({ value, label: value }));
    const sources = namedStatistics();
    state.analysis.charts.forEach((chart, index) => {
      if (!sources.some((item) => item.value === chart.statistic))
        chart.statistic = sources[0]?.value || "";
      const row = element("div", "analysis-item chart-setting");
      row.appendChild(element("strong", "", labelOf(CHARTS, chart.kind)));
      const controls = element("div", "analysis-item-controls");
      if (["box", "bar", "convergence", "pareto"].includes(chart.kind))
        controls.appendChild(chartControl(
          "问题", problems, chart.problem,
          (value) => { chart.problem = value; }, "wide-control"
        ));
      if (["box", "multi_box", "bar", "multi_bar"].includes(chart.kind))
        controls.appendChild(chartControl(
          "归一化", NORMALIZATIONS, chart.normalization,
          (value) => { chart.normalization = value; }
        ));
      if (["bar", "multi_bar", "rank_heatmap", "convergence"].includes(chart.kind))
        controls.appendChild(chartControl(
          "统计列", sources, chart.statistic,
          (value) => { chart.statistic = value; }
        ));
      if (["bar", "multi_bar", "convergence"].includes(chart.kind))
        controls.appendChild(chartControl(
          "区间", BANDS, chart.band,
          (value) => { chart.band = value; }
        ));
      if (["box", "multi_box"].includes(chart.kind))
        controls.appendChild(toggleControl(
          "显示抖动点", chart.jitter,
          (value) => { chart.jitter = value; }
        ));
      if (chart.kind === "convergence")
        controls.appendChild(numberControl(
          "采样网格", chart.grid, 2,
          (value) => { chart.grid = value; }
        ));
      if (chart.kind === "pareto") {
        controls.append(
          numberControl("X 目标", chart.objectiveX, 0, (value) => { chart.objectiveX = value; }),
          numberControl("Y 目标", chart.objectiveY, 0, (value) => { chart.objectiveY = value; }),
          numberControl("运行轮次", chart.run, -1, (value) => { chart.run = value; }),
          toggleControl("连接点", chart.connect, (value) => { chart.connect = value; })
        );
      }
      controls.appendChild(removeButton(() => {
        state.analysis.charts.splice(index, 1);
        renderChartsSettings();
      }));
      row.appendChild(controls);
      host.appendChild(row);
    });
    if (!state.analysis.charts.length)
      host.appendChild(element("div", "analysis-empty", "未添加额外图表；仍会生成页面统计表。"));
  };

  const renderAnalysis = () => {
    renderReducer();
    renderStatistics();
    renderComparisons();
    renderBaseline();
    renderChartsSettings();
  };

  const updateGenerateState = () => {
    const hasOptimizer = selectedOptimizers().length > 0;
    const hasProblem = selectedProblems().length > 0;
    const pathChanged = pathInput.value.trim() !== state.path;
    const hasStatistic = state.analysis.statistics.length > 0;
    const referencesReady = state.analysis.reducer.kind !== "indicator"
      || selectedProblems().every((problem) =>
        (state.analysis.references[problem] || "").trim().length > 0);
    generateButton.disabled = !state.batch || !hasOptimizer || !hasProblem
      || !hasStatistic || !referencesReady || pathChanged;
  };

  const selectBatch = (batch) => {
    state.batch = batch;
    state.report = null;
    state.analysis = defaultAnalysis();
    renderBatchList();
    byId("report-selection").hidden = false;
    byId("report-results").hidden = true;
    byId("report-selection-title").textContent = `报告范围 · ${batch.tag}`;
    byId("report-ragged-badge").hidden = !batch.ragged;
    renderChoices("report-optimizer-options", batch.optimizers, "optimizer");
    renderChoices("report-problem-options", batch.problems, "problem");
    normalizeChartProblems();
    renderAnalysis();
    updateGenerateState();
    if (batch.ragged)
      showMessage(`该批次的运行集合有差异或空洞；${(batch.runDiagnostics || []).join("；")}`);
    else
      hideMessage();
  };

  const scan = async () => {
    const path = pathInput.value.trim() || "_log";
    setBusy(scanButton, true, "正在扫描…", "扫描批次");
    showMessage(`正在扫描 ${path}…`);
    try {
      const form = new URLSearchParams({ path });
      const data = await ui.api("/api/report/discover", {
        method: "POST",
        headers: formHeaders,
        body: form
      });
      if (!data.ok) throw new Error(data.error || "无法扫描日志目录");
      state.path = path;
      state.batches = data.batches || [];
      state.batch = null;
      state.report = null;
      state.scanned = true;
      byId("report-selection").hidden = true;
      byId("report-results").hidden = true;
      renderBatchList();
      showMessage(state.batches.length
        ? `已发现 ${state.batches.length} 个实验批次。`
        : (data.error || "目录中没有可分析的 .rslt 文件。"));
    } catch (error) {
      state.batches = [];
      state.batch = null;
      renderBatchList();
      byId("report-selection").hidden = true;
      byId("report-results").hidden = true;
      showMessage(error.message || "扫描失败", "error");
    } finally {
      setBusy(scanButton, false, "正在扫描…", "扫描批次");
    }
  };

  const renderReportTable = (data) => {
    const table = byId("report-table");
    table.replaceChildren();
    const subCount = Math.max(data.subheader.length, 1);
    const columnCount = Math.max(data.header.length * subCount, 1);
    const head = document.createElement("thead");
    const groupRow = document.createElement("tr");
    const corner = document.createElement("th");
    corner.rowSpan = data.subheader.length ? 2 : 1;
    corner.textContent = "问题 / 汇总";
    groupRow.appendChild(corner);
    data.header.forEach((header) => {
      const cell = document.createElement("th");
      cell.colSpan = subCount;
      cell.textContent = header;
      groupRow.appendChild(cell);
    });
    head.appendChild(groupRow);
    if (data.subheader.length) {
      const statRow = document.createElement("tr");
      data.header.forEach(() => data.subheader.forEach((stat) => {
        const cell = document.createElement("th");
        cell.textContent = stat;
        statRow.appendChild(cell);
      }));
      head.appendChild(statRow);
    }
    table.appendChild(head);

    const body = document.createElement("tbody");
    data.rows.forEach((row) => {
      const tr = document.createElement("tr");
      const name = document.createElement("th");
      name.scope = "row";
      name.textContent = row.name;
      tr.appendChild(name);
      const values = row.values || [];
      const span = values.length > 0 && columnCount >= values.length
        ? Math.max(1, Math.floor(columnCount / values.length)) : 1;
      values.forEach((value) => {
        const cell = document.createElement("td");
        cell.colSpan = span;
        cell.textContent = value;
        tr.appendChild(cell);
      });
      body.appendChild(tr);
    });
    table.appendChild(body);
  };

  const svgNode = (source) => {
    const parsed = new DOMParser().parseFromString(source, "image/svg+xml");
    if (parsed.querySelector("parsererror") || parsed.documentElement.localName !== "svg")
      throw new Error("服务生成了无效 SVG");
    const svg = document.importNode(parsed.documentElement, true);
    svg.classList.add("embedded-report-svg");
    svg.removeAttribute("width");
    svg.removeAttribute("height");
    return svg;
  };

  const downloadChart = async (chartId, format, button) => {
    const original = button.textContent;
    button.disabled = true;
    button.textContent = "正在生成…";
    try {
      const response = await ui.download("/api/report/download", {
        method: "POST",
        headers: formHeaders,
        body: buildForm({ chartId, format })
      });
      if (!response.ok) {
        const data = await response.json().catch(() => ({}));
        throw new Error(data.error || "图表下载失败");
      }
      const blob = await response.blob();
      const disposition = response.headers.get("Content-Disposition") || "";
      const match = disposition.match(/filename="([^"]+)"/);
      const name = match ? match[1] : `evoflow-${chartId}.${format}`;
      const url = URL.createObjectURL(blob);
      const anchor = document.createElement("a");
      anchor.href = url;
      anchor.download = name;
      document.body.appendChild(anchor);
      anchor.click();
      anchor.remove();
      setTimeout(() => URL.revokeObjectURL(url), 1000);
    } catch (error) {
      showMessage(error.message || "图表下载失败", "error");
    } finally {
      button.disabled = false;
      button.textContent = original;
    }
  };

  const renderOutputCharts = (charts) => {
    const host = byId("report-charts");
    host.replaceChildren();
    charts.forEach((chart) => {
      const card = element("section", "report-chart-card data-panel");
      const heading = element("div", "chart-heading");
      const title = element("div");
      title.append(element("span", "", labelOf(CHARTS, chart.type)), element("h2", "", chart.title));
      const actions = element("div", "chart-actions");
      ["svg", "png"].forEach((format) => {
        const button = element("button", "toolbar-button", `下载 ${format.toUpperCase()}`);
        button.type = "button";
        button.addEventListener("click", () => downloadChart(chart.id || chart.kind, format, button));
        actions.appendChild(button);
      });
      heading.append(title, actions);
      const canvas = element("div", "chart-canvas");
      canvas.appendChild(svgNode(chart.svg));
      card.append(heading, canvas);
      host.appendChild(card);
    });
  };

  const renderReport = (data) => {
    state.report = data;
    byId("report-summary-tag").textContent = data.chosen.tag;
    byId("report-summary-optimizers").textContent = data.chosen.optimizers.length;
    byId("report-summary-problems").textContent = data.chosen.problems.length;
    byId("report-summary-repeats").textContent = data.chosen.repeats;
    renderReportTable(data);
    renderOutputCharts(data.charts || []);
    byId("report-results").hidden = false;
    if (data.chosen.ragged || (data.warnings || []).length)
      showMessage(`报告按共同结果编号 ${(data.chosen.runNumbers || []).join("、")} 生成；${(data.chosen.runDiagnostics || []).join("；")}`);
    else
      showMessage("统计报告已生成。");
  };

  const generate = async () => {
    if (!state.batch || pathInput.value.trim() !== state.path) {
      showMessage("日志目录已修改，请重新扫描批次。", "error");
      return;
    }
    setBusy(generateButton, true, "正在分析…", "生成统计报告");
    showMessage(`正在分析批次 ${state.batch.tag}…`);
    try {
      const data = await ui.api("/api/report/build", {
        method: "POST",
        headers: formHeaders,
        body: buildForm()
      });
      if (!data.ok) throw new Error(data.error || "报告生成失败");
      renderReport(data);
    } catch (error) {
      byId("report-results").hidden = true;
      showMessage(error.message || "报告生成失败", "error");
    } finally {
      setBusy(generateButton, false, "正在分析…", "生成统计报告");
      updateGenerateState();
    }
  };

  fillSelect(byId("report-statistic-add"), [
    ...STATISTICS,
    { value: "percentile", label: "Percentile · 自定义分位数" }
  ], "mean");
  fillSelect(byId("report-comparison-add"), COMPARISONS, "best");
  fillSelect(byId("report-chart-add"), CHARTS, "table");

  byId("report-reducer-kind").addEventListener("change", (event) => {
    state.analysis.reducer.kind = event.target.value;
    state.analysis.reducer.metric = event.target.value === "indicator"
      ? INDICATORS[0].value : STATISTICS[0].value;
    renderReducer();
    updateGenerateState();
  });
  byId("report-reducer-metric").addEventListener("change", (event) => {
    state.analysis.reducer.metric = event.target.value;
  });
  byId("report-reducer-objective").addEventListener("input", (event) => {
    state.analysis.reducer.objective = Number(event.target.value);
  });
  byId("report-reducer-percentile").addEventListener("input", (event) => {
    state.analysis.reducer.percentile = Number(event.target.value);
  });

  byId("report-statistic-add-button").addEventListener("click", () => {
    const metric = byId("report-statistic-add").value;
    if (metric !== "percentile"
      && state.analysis.statistics.some((item) => item.metric === metric)) {
      showMessage("该跨轮统计指标已经存在。", "error");
      return;
    }
    state.analysis.statistics.push({ metric, percentile: 0.5 });
    renderStatistics();
    renderComparisons();
    renderChartsSettings();
    updateGenerateState();
  });

  byId("report-comparison-add-button").addEventListener("click", () => {
    const metric = byId("report-comparison-add").value;
    if (state.analysis.comparisons.some((item) => item.metric === metric)) {
      showMessage("该优化器比较指标已经存在。", "error");
      return;
    }
    const source = namedStatistics()[0]?.value;
    if (comparisonUsesSource(metric) && !source) {
      showMessage("请先添加一个非分位数的跨轮统计指标。", "error");
      return;
    }
    state.analysis.comparisons.push({ metric, on: source || "mean", direction: "min" });
    renderComparisons();
    updateGenerateState();
  });

  byId("report-chart-add-button").addEventListener("click", () => {
    const kind = byId("report-chart-add").value;
    const chart = newChart(kind, `chart-${state.analysis.nextChartId++}`);
    chart.problem = selectedProblems()[0] || "";
    chart.statistic = namedStatistics()[0]?.value || "";
    state.analysis.charts.push(chart);
    renderChartsSettings();
  });

  document.querySelectorAll("[data-toggle-report]").forEach((button) => {
    button.addEventListener("click", () => {
      const kind = button.dataset.toggleReport;
      const id = kind === "optimizers"
        ? "report-optimizer-options" : "report-problem-options";
      const inputs = Array.from(byId(id).querySelectorAll('input[type="checkbox"]'));
      const allChecked = inputs.length > 0 && inputs.every((input) => input.checked);
      inputs.forEach((input) => { input.checked = !allChecked; });
      if (kind === "optimizers") renderBaseline();
      else {
        normalizeChartProblems();
        renderReducer();
        renderChartsSettings();
      }
      updateGenerateState();
    });
  });

  pathInput.addEventListener("input", () => {
    if (state.scanned && pathInput.value.trim() !== state.path)
      showMessage("目录已修改，请重新扫描后再生成报告。");
    updateGenerateState();
  });
  pathInput.addEventListener("keydown", (event) => {
    if (event.key === "Enter") {
      event.preventDefault();
      scan();
    }
  });
  scanButton.addEventListener("click", scan);
  generateButton.addEventListener("click", generate);

  window.addEventListener("evoflow:page", (event) => {
    if (event.detail?.page === "report" && !state.scanned) scan();
  });
})();
