(() => {
  "use strict";

  const PAIRS = [
    ["模块化演化计算工作台", "Modular evolutionary computation workbench"],
    ["主导航", "Main navigation"],
    ["问题定义", "Problem definition"],
    ["优化器构筑", "Optimizer builder"],
    ["统计分析", "Statistical analysis"],
    ["正在连接本地服务", "Connecting to local service"],
    ["本地服务已连接", "Local service connected"],
    ["本地服务未连接", "Local service disconnected"],
    ["本地服务异常", "Local service error"],
    ["缺少会话令牌", "Session token missing"],
    ["优化配置 / 问题定义", "Optimization / Problem definition"],
    ["优化配置 / 优化器构筑", "Optimization / Optimizer builder"],
    ["运行结果 / 统计分析", "Results / Statistical analysis"],
    ["加载并检查问题数据", "Load and inspect problem data"],
    ["定义、加载并检查问题数据", "Define, load and inspect problem data"],
    ["构筑、校验并保存优化器", "Build, validate and save an optimizer"],
    ["比较实验批次与下载图表", "Compare experiment batches and download charts"],
    ["模型设置", "Model settings"],
    ["未配置", "Not configured"],
    ["尚未配置", "Not configured"],
    ["退出 EvoFlow", "Exit EvoFlow"],
    ["模型连接设置", "Model connection settings"],
    ["设置只在当前页面有效；关闭或刷新页面后会清除。", "Settings apply only to this page and are cleared when it closes or reloads."],
    ["完整接口地址", "Full endpoint URL"],
    ["公网地址必须使用 HTTPS；HTTP 仅允许 127.0.0.1 或 localhost。", "Public endpoints must use HTTPS; HTTP is allowed only for 127.0.0.1 or localhost."],
    ["模型名称", "Model name"],
    ["模型标识", "Model identifier"],
    ["API Key（可选）", "API key (optional)"],
    ["本地模型可留空", "Optional for local models"],
    ["填写完整接口地址和模型名称后测试连接。", "Enter the endpoint and model name, then test the connection."],
    ["测试连接", "Test connection"],
    ["后续使用模型能力时，问题描述及可选参考数据会发送到你配置的模型服务；EvoFlow 不会把 API Key\n          写入磁盘、浏览器存储或日志。", "When model features are used, the problem description and optional reference data are sent to your configured service. EvoFlow never writes the API key to disk, browser storage or logs."],
    ["关闭模型设置", "Close model settings"],
    ["定义或加载问题", "Define or load a problem"],
    ["填写描述、附加数据文件，或直接选择标准问题文件。系统会根据输入组合自动选择处理方式，加载后不会自动运行。", "Enter a description, attach a data file, or select a standard problem file. The system chooses the appropriate flow automatically and does not run the problem after loading."],
    ["提供问题描述与数据", "Provide a problem description and data"],
    ["描述与数据文件均为可选，但至少提供一项。系统会自动识别所需流程，无需选择专业问题类型。", "The description and data file are both optional, but at least one is required. The system chooses the required flow automatically, without specialist type selection."],
    ["模型连接", "Model connection"],
    ["模型连接可用", "Model connected"],
    ["正在测试连接", "Testing connection"],
    ["连接测试未通过", "Connection test failed"],
    ["自然语言描述", "Natural-language description"],
    ["可选", "Optional"],
    ["说明对象、目标和约束。例如：我想访问附件中的所有地点，请规划总路程最短的路线。", "Describe the entities, objective and constraints. Example: I want to visit every location in the attachment; plan the shortest route."],
    ["选择或拖入问题数据文件", "Select or drop a problem data file"],
    ["问题数据文件", "Problem data file"],
    ["点击浏览或拖入文件；可以是标准问题文件，也可以是配合描述使用的文本、CSV、TSV 或 JSON 数据。", "Browse or drop a file. It can be a standard problem file, or text, CSV, TSV or JSON data used with a description."],
    ["标准文件最大 64 MiB；作为模型参考数据时须为文本且不超过 1 MiB", "Standard files: up to 64 MiB. Model reference data: text only, up to 1 MiB."],
    ["移除所选文件", "Remove selected file"],
    ["系统会优先在本地识别标准问题文件；只有需要模型理解描述或转换参考数据时，相关内容才会发送到你配置的模型服务。", "The system identifies standard problem files locally first. Content is sent to your configured model service only when the description or reference data requires model interpretation."],
    ["问题文件", "Problem file"],
    ["从文件管理器选择 EvoFlow 算例后，会显示问题名称、规模、变量、目标和约束；不会自动运行。", "Select an EvoFlow instance to inspect its name, size, variables, objectives and constraints. It will not run automatically."],
    ["模型能力", "Model assisted"],
    ["自然语言定义问题", "Define a problem in natural language"],
    ["尚未连接", "Not connected"],
    ["描述你的优化问题", "Describe your optimization problem"],
    ["系统会根据你的描述自动识别问题类型并生成可用的问题数据，无需选择或确认专业类型。", "The system identifies the problem type and generates usable data automatically; no specialist type selection is required."],
    ["自动完成", "Automatic"],
    ["问题描述", "Problem description"],
    ["说明对象、数据、决策目标和约束。例如：有若干客户及其坐标和需求，从一个车场出发，车辆容量有限，希望最小化总行驶距离。", "Describe the entities, data, decision objective and constraints. Example: customers have coordinates and demands; vehicles leave one depot with limited capacity and total travel distance should be minimized."],
    ["点击后，描述会发送到你在右上角配置的模型服务。当前支持的类型会由系统自动判断。", "Your description will be sent to the model service configured above. The supported type is selected automatically."],
    ["智能构建问题", "Build problem with model"],
    ["识别问题类型", "Identify problem type"],
    ["等待识别", "Waiting"],
    ["生成并检查问题", "Generate and inspect problem"],
    ["自动确认数据是否完整可用", "Automatically verifies that the data is complete and usable"],
    ["或加载已有问题文件", "Or load an existing problem file"],
    ["选择或拖入问题文件", "Select or drop a problem file"],
    ["选择问题文件", "Select problem file"],
    ["点击打开文件管理器，或将文件拖到这里", "Click to browse, or drop a file here"],
    ["浏览文件", "Browse files"],
    ["单个文件最大 64 MiB", "Maximum file size: 64 MiB"],
    ["当前问题已保留在本次会话中", "The current problem is retained for this session"],
    ["另存会写出原始载入数据或模型生成并通过检查的数据，不改变问题格式。", "Save As writes the original loaded data or the validated model output without changing its format."],
    ["另存问题", "Save problem as"],
    ["问题名称", "Problem name"],
    ["问题类型", "Problem type"],
    ["决策维度", "Decision dimension"],
    ["定义规模", "Definition size"],
    ["变量", "Variables"],
    ["名称", "Name"],
    ["种类", "Kind"],
    ["变量类型", "Variable type"],
    ["长度", "Length"],
    ["下界", "Lower bound"],
    ["上界", "Upper bound"],
    ["目标", "Objectives"],
    ["方向", "Direction"],
    ["优先级", "Priority"],
    ["关联变量", "Related variables"],
    ["约束", "Constraints"],
    ["类型", "Type"],
    ["作用变量", "Variables"],
    ["惩罚系数", "Penalty"],
    ["施加目标", "Target objectives"],
    ["决策变量", "Decision variable"],
    ["常量", "Constant"],
    ["最小化", "Minimize"],
    ["最大化", "Maximize"],
    ["仅可行性约束", "Feasibility only"],
    ["未命名", "Unnamed"],
    ["新建", "New"],
    ["打开 .cfg", "Open .cfg"],
    ["保存", "Save"],
    ["另存为", "Save as"],
    ["装配验证", "Assembly validation"],
    ["构筑视图", "Builder view"],
    ["格式", "Format"],
    ["v3.1 文本", "v3.1 text"],
    ["表单", "Form"],
    ["积木", "Canvas"],
    ["运行设置", "Run settings"],
    ["切换或载入配置时保持不变", "Unchanged when switching or loading configurations"],
    ["运行标签", "Run tag"],
    ["留空则使用配置值", "Leave blank to use the configuration value"],
    ["种子", "Seed"],
    ["运行轮数", "Runs"],
    ["结果起点", "Result start"],
    ["自动", "Automatic"],
    ["覆写旧结果", "Replace existing results"],
    ["运行当前配置", "Run current configuration"],
    ["使用当前问题和配置运行；终止条件取自配置，运行标签、种子与轮数按“运行设置”执行。", "Runs the current problem and configuration. Termination comes from the configuration; tag, seed and run count come from Run settings."],
    ["等待运行", "Waiting"],
    ["准备运行", "Preparing"],
    ["当前配置运行结果", "Current configuration results"],
    ["运行结果", "Run result"],
    ["优化器标识", "Optimizer identity"],
    ["结果编号", "Result index"],
    ["查看上一轮结果", "Previous run result"],
    ["查看下一轮结果", "Next run result"],
    ["配置结构", "Configuration structure"],
    ["优化器设置", "Optimizer settings"],
    ["协作配置", "Cooperation"],
    ["新增 workflow", "Add workflow"],
    ["新增子种群", "Add subpopulation"],
    ["删除 workflow", "Delete workflow"],
    ["删除子种群", "Delete subpopulation"],
    ["组件目录 · 12 类", "Component catalog · 12 categories"],
    ["检索组件名称或类别", "Search component name or category"],
    ["清除检索", "Clear search"],
    ["未找到匹配组件。", "No matching components."],
    ["浮动算子库", "Floating operator library"],
    ["可用算子", "Available operators"],
    ["收起算子库", "Collapse operator library"],
    ["展开算子库", "Expand operator library"],
    ["拖入画布或双击追加", "Drag onto the canvas or double-click to append"],
    ["学习拓扑", "Learning topology"],
    ["学习策略", "Learning strategy"],
    ["子代生成", "Offspring generation"],
    ["环境选择", "Environmental selection"],
    ["评价器", "Evaluator"],
    ["修复器", "Repair"],
    ["最优档案", "Best archive"],
    ["子群管理", "Subpopulation manager"],
    ["子群构建", "Subpopulation builder"],
    ["协作拓扑", "Cooperation topology"],
    ["个体", "Individual"],
    ["特性", "Feature"],
    ["正在读取可用组件…", "Loading available components…"],
    ["正在初始化配置编辑器…", "Initializing configuration editor…"],
    ["装配期自动推断，不直接写入配置。", "Inferred during assembly and not written directly to the configuration."],
    ["拖入积木画布；双击追加到末尾", "Drag onto the canvas; double-click to append"],
    ["积木式 Workflow", "Workflow canvas"],
    ["拖入组件后可自由排列；有向箭头表示执行顺序。完整单链会自动写回配置。", "Drop and arrange components freely. Directed arrows define execution order; a complete single path is committed automatically."],
    ["连接状态图例", "Connection status legend"],
    ["满足", "Satisfied"],
    ["信息", "Info"],
    ["有警告", "Warning"],
    ["数量不足", "Insufficient"],
    ["不满足", "Unsatisfied"],
    ["临时孤立", "Temporarily isolated"],
    ["缩小画布", "Zoom out"],
    ["放大画布", "Zoom in"],
    ["重置画布缩放", "Reset zoom"],
    ["自动整理布局", "Auto arrange"],
    ["自动整理", "Auto arrange"],
    ["适应工作流全貌", "Fit the entire workflow"],
    ["适应全貌", "Fit workflow"],
    ["画布全屏", "Full-screen canvas"],
    ["退出全屏", "Exit full screen"],
    ["画布缩放", "Canvas zoom"],
    ["Workflow 积木画布", "Workflow canvas"],
    ["选择积木后在这里检查连接关系并编辑参数。", "Select a block to inspect connections and edit parameters."],
    ["类别", "Category"],
    ["组件", "Component"],
    ["需求", "Requirements"],
    ["供给", "Provisions"],
    ["无", "None"],
    ["当前积木连接正常。", "This block is connected correctly."],
    ["在表单中定位", "Locate in form"],
    ["删除", "Delete"],
    ["固定起点 · 参数在表单中编辑", "Fixed start · edit parameters in the form"],
    ["初始化器", "Initializer"],
    ["连接状态 · 满足", "Connection · satisfied"],
    ["连接状态 · 信息", "Connection · info"],
    ["连接状态 · 警告", "Connection · warning"],
    ["连接状态 · 数量不足", "Connection · insufficient"],
    ["连接状态 · 不满足", "Connection · unsatisfied"],
    ["连接状态 · 临时孤立", "Connection · temporarily isolated"],
    ["请选择一个 workflow 进入积木视图。", "Select a workflow to open the canvas."],
    ["当前没有选中的 workflow。", "No workflow is selected."],
    ["从左侧组件目录拖入第一块积木", "Drop the first block from the component catalog"],
    ["配置校验", "Configuration validation"],
    ["等待校验", "Waiting for validation"],
    ["修改配置后会自动校验。", "The configuration is validated automatically after changes."],
    ["未发现问题。", "No issues found."],
    ["PASS · 未发现配置问题", "PASS · no configuration issues found"],
    ["PASS · 静态检查未发现配置问题。", "PASS · static validation found no configuration issues."],
    ["装配验证通过，但存在运行可靠性警告。", "Assembly passed with runtime reliability warnings."],
    ["装配验证通过，同时存在配置信息提示。", "Assembly passed with configuration info."],
    ["正在检查…", "Checking…"],
    ["静态校验通过", "Static validation passed"],
    ["装配通过", "Assembly passed"],
    ["校验服务失败", "Validation service failed"],
    ["请先在“问题定义”中加载或智能构建问题。", "Load or build a problem in Problem definition first."],
    ["已准备空白配置骨架。", "Blank configuration ready."],
    ["名称、预算、日志与全局档案。", "Name, budgets, logging and global archive."],
    ["保存到 v3.1 时作为单个标识符，不使用空格。", "Saved as one identifier without spaces in v3.1."],
    ["配置默认标签", "Default configuration tag"],
    ["保存在配置中；运行设置未填写标签时使用此值。", "Stored in the configuration and used when Run settings has no tag."],
    ["终止条件", "Termination conditions"],
    ["三项均为 -1 时使用平台默认预算", "Platform defaults are used when all three values are -1"],
    ["最大 FES", "Maximum FES"],
    ["最大停滞代数", "Maximum stagnant generations"],
    ["最大时间（秒）", "Maximum time (seconds)"],
    ["-1 表示不设置", "-1 means unset"],
    ["日志", "Logging"],
    ["完整结果", "Full results"],
    ["过程日志", "Process log"],
    ["完整过程", "Full process"],
    ["整群快照", "Population snapshots"],
    ["全局档案", "Global archive"],
    ["档案组件", "Archive component"],
    ["组件按列表顺序执行；同一类别可以重复出现。", "Components execute in list order; categories may appear more than once."],
    ["Workflow 标识", "Workflow identifier"],
    ["修改后自动同步引用此 workflow 的子种群。", "Changes are synchronized to subpopulations that reference this workflow."],
    ["有序组件", "Ordered components"],
    ["SET 型组件应位于消费它的生成组件之前", "SET components should precede the generator that consumes them"],
    ["添加组件", "Add component"],
    ["尚无组件。请按执行顺序添加 topology、repair、strategy、generator、evaluator 与 selector。", "No components yet. Add topology, repair, strategy, generator, evaluator and selector in execution order."],
    ["扫描实验批次", "Scan experiment batches"],
    ["输入结果日志目录。相对路径以 EvoFlow 启动目录为基准，也可输入本机绝对路径。", "Enter the result log directory. Relative paths use the EvoFlow launch directory; absolute paths are also accepted."],
    ["结果日志目录", "Result log directory"],
    ["扫描批次", "Scan batches"],
    ["实验批次", "Experiment batches"],
    ["扫描日志目录后在这里选择 tag。", "Scan a log directory, then select a tag here."],
    ["报告范围", "Report scope"],
    ["数据不齐", "Incomplete data"],
    ["优化器", "Optimizers"],
    ["全选 / 清空", "Select all / Clear"],
    ["问题", "Problems"],
    ["分析设置", "Analysis settings"],
    ["单个归约指标 · 多个统计与比较指标 · 自选图表", "One reducer · multiple statistics and comparisons · selected charts"],
    ["单次运行归约", "Per-run reduction"],
    ["每份报告仅一个", "Exactly one per report"],
    ["描述统计", "Descriptive statistic"],
    ["分位数", "Percentile"],
    ["质量指标", "Quality indicator"],
    ["指标", "Metric"],
    ["目标列", "Objective column"],
    ["分位点", "Percentile"],
    ["跨轮统计", "Cross-run statistics"],
    ["选择统计指标", "Select a statistic"],
    ["添加", "Add"],
    ["优化器比较", "Optimizer comparison"],
    ["选择比较指标", "Select a comparison"],
    ["基准优化器", "Baseline optimizer"],
    ["报告图表", "Report charts"],
    ["选择图表", "Select a chart"],
    ["添加图表", "Add chart"],
    ["生成统计报告", "Generate report"],
    ["批次", "Batch"],
    ["共同重复次数", "Common repeats"],
    ["统计表", "Statistics table"],
    ["PNG 下载设置与字体依赖", "PNG download and font settings"],
    ["英文字体", "Latin font"],
    ["中文字体", "CJK font"],
    ["分辨率倍率", "Resolution scale"],
    ["页内 SVG 不依赖系统字体。PNG 会在运行期读取以上字体文件；项目不会分发商业字体。", "Embedded SVG does not depend on system fonts. PNG reads the listed fonts at runtime; commercial fonts are not distributed."],
    ["从现有配置中推荐", "Recommend from existing configurations"],
    ["根据当前问题从现有配置中选择合适方案。模型不会生成或修改配置，是否保存以及保存位置仍由你决定。", "Selects a suitable existing configuration for the current problem. The model does not generate or modify configurations; you decide whether and where to save."],
    ["根据当前问题推荐", "Recommend for current problem"],
    ["请先加载问题并连接模型服务。", "Load a problem and connect a model service first."],
    ["报告已生成。", "Report generated."],
    ["统计报告已生成。", "Statistical report generated."],
    ["下载 SVG", "Download SVG"],
    ["下载 PNG", "Download PNG"],
    ["正在扫描…", "Scanning…"],
    ["正在分析…", "Analyzing…"],
    ["正在生成…", "Generating…"],
    ["正在准备…", "Preparing…"],
    ["正在读取…", "Reading…"],
    ["正在解析参数…", "Resolving parameters…"],
    ["此组件没有参数。", "This component has no parameters."],
    ["使用组件内部默认参数（配置中不写参数）", "Use component defaults (parameters are omitted from the configuration)"],
    ["留空", "Empty"],
    ["硬边界", "Hard bounds"],
    ["推荐", "Recommended"],
    ["越小越优", "Lower is better"],
    ["越大越优", "Higher is better"],
    ["原始值", "Raw values"],
    ["合并秩", "Pooled rank"],
    ["相对最优比", "Ratio to best"],
    ["不显示区间", "No interval"],
    ["显示抖动点", "Show jitter points"],
    ["采样网格", "Sampling grid"],
    ["连接点", "Connect points"],
    ["统计列", "Statistic"],
    ["区间", "Interval"],
    ["归一化", "Normalization"],
    ["运行轮次", "Run index"],
    ["没有发现 .rslt 结果文件。", "No .rslt result files found."],
    ["Mean · 均值", "Mean"],
    ["Variance · 方差", "Variance"],
    ["Std · 标准差", "Standard deviation"],
    ["Median · 中位数", "Median"],
    ["Mode · 众数", "Mode"],
    ["Smallest · 最小值", "Smallest"],
    ["Largest · 最大值", "Largest"],
    ["Q1 · 下四分位", "Q1 · lower quartile"],
    ["Q3 · 上四分位", "Q3 · upper quartile"],
    ["IQR · 四分位距", "IQR · interquartile range"],
    ["Range · 极差", "Range"],
    ["SEM · 均值标准误", "SEM · standard error"],
    ["CV · 变异系数", "CV · coefficient of variation"],
    ["Geometric mean · 几何均值", "Geometric mean"],
    ["Harmonic mean · 调和均值", "Harmonic mean"],
    ["Percentile · 自定义分位数", "Custom percentile"],
    ["Best · 最优标记", "Best marker"],
    ["Rank · 排名", "Rank"],
    ["Significance · 显著性", "Significance"],
    ["统计表图", "Statistics table"],
    ["单问题箱线图", "Single-problem box plot"],
    ["跨问题箱线图", "Cross-problem box plot"],
    ["单问题统计柱状图", "Single-problem statistic bars"],
    ["跨问题统计柱状图", "Cross-problem statistic bars"],
    ["平均秩柱状图", "Average-rank bars"],
    ["排名热力图", "Rank heatmap"],
    ["胜 / 平 / 负图", "Win / tie / loss chart"],
    ["Nemenyi 临界差异图", "Nemenyi critical-difference chart"],
    ["收敛曲线", "Convergence curve"],
    ["Pareto 前沿", "Pareto front"],
    ["另存问题失败", "Failed to save problem"],
    ["EvoFlow 已退出", "EvoFlow has exited"],
    ["本地服务已经关闭，可以安全关闭此标签页。", "The local service has stopped. You can safely close this tab."]
  ];

  const zhToEn = new Map(PAIRS);
  const enToZh = new Map(PAIRS.map(([zh, en]) => [en, zh]));
  let language = "zh";
  let applying = false;

  const patterns = {
    en: [
      [/^(\d+) 错误 · (\d+) 警告 · (\d+) 信息$/, "$1 errors · $2 warnings · $3 info"],
      [/^(\d+) 错误 · (\d+) 警告$/, "$1 errors · $2 warnings"],
      [/^第 (\d+) \/ (\d+) 轮$/, "Run $1 / $2"],
      [/^(\d+) 变量 · (\d+) 目标 · (\d+) 约束$/, "$1 variables · $2 objectives · $3 constraints"],
      [/^报告范围 · (.+)$/, "Report scope · $1"],
      [/^积木式 Workflow · (.+)$/, "Workflow canvas · $1"],
      [/^已发现 (\d+) 个实验批次。$/, "$1 experiment batch(es) found."],
      [/^(\d+) 优化器$/, "$1 optimizer(s)"],
      [/^(\d+) 问题$/, "$1 problem(s)"],
      [/^(\d+) 次共同运行$/, "$1 common run(s)"],
      [/^(\d+) 文件$/, "$1 file(s)"],
      [/^(\d+) 个错误$/, "$1 error(s)"],
      [/^(\d+) 个警告$/, "$1 warning(s)"],
      [/^(\d+) 条信息$/, "$1 info item(s)"],
      [/^显示 (\d+) \/ (\d+) 个组件$/, "Showing $1 of $2 components"],
      [/^共 (\d+) 个组件$/, "$1 components"],
      [/^(\d+) · 自动$/, "$1 · inferred"],
      [/^Component (\d+)$/, "Component $1"]
    ],
    zh: [
      [/^(\d+) errors · (\d+) warnings · (\d+) info$/, "$1 错误 · $2 警告 · $3 信息"],
      [/^(\d+) errors · (\d+) warnings$/, "$1 错误 · $2 警告"],
      [/^Run (\d+) \/ (\d+)$/, "第 $1 / $2 轮"],
      [/^(\d+) variables · (\d+) objectives · (\d+) constraints$/, "$1 变量 · $2 目标 · $3 约束"],
      [/^Report scope · (.+)$/, "报告范围 · $1"],
      [/^Workflow canvas · (.+)$/, "积木式 Workflow · $1"],
      [/^(\d+) experiment batch\(es\) found\.$/, "已发现 $1 个实验批次。"],
      [/^(\d+) optimizer\(s\)$/, "$1 优化器"],
      [/^(\d+) problem\(s\)$/, "$1 问题"],
      [/^(\d+) common run\(s\)$/, "$1 次共同运行"],
      [/^(\d+) file\(s\)$/, "$1 文件"],
      [/^(\d+) error\(s\)$/, "$1 个错误"],
      [/^(\d+) warning\(s\)$/, "$1 个警告"],
      [/^(\d+) info item\(s\)$/, "$1 条信息"],
      [/^Showing (\d+) of (\d+) components$/, "显示 $1 / $2 个组件"],
      [/^(\d+) components$/, "共 $1 个组件"],
      [/^(\d+) · inferred$/, "$1 · 自动"]
    ]
  };

  const convert = (value, target = language) => {
    if (!value) return value;
    const direct = (target === "en" ? zhToEn : enToZh).get(value);
    if (direct) return direct;
    for (const [pattern, replacement] of patterns[target])
      if (pattern.test(value)) return value.replace(pattern, replacement);
    return value;
  };

  const ignored = (node) => node.parentElement?.closest(
    "[data-i18n-ignore], #variables-body, #objectives-body, #constraints-body, "
    + ".run-solution-list, .report-table tbody"
  );

  const translateTextNode = (node) => {
    if (ignored(node)) return;
    const raw = node.nodeValue || "";
    const trimmed = raw.trim();
    if (!trimmed) return;
    const translated = convert(trimmed);
    if (translated === trimmed) return;
    const begin = raw.slice(0, raw.indexOf(trimmed));
    const end = raw.slice(raw.indexOf(trimmed) + trimmed.length);
    node.nodeValue = begin + translated + end;
  };

  const translateElement = (element) => {
    if (!(element instanceof Element)) return;
    ["placeholder", "aria-label", "title"].forEach((name) => {
      const value = element.getAttribute(name);
      if (value) element.setAttribute(name, convert(value));
    });
    for (const child of element.childNodes)
      if (child.nodeType === Node.TEXT_NODE) translateTextNode(child);
  };

  const translateTree = (root = document.body) => {
    if (!root) return;
    applying = true;
    if (root.nodeType === Node.TEXT_NODE) translateTextNode(root);
    else {
      translateElement(root);
      root.querySelectorAll?.("*").forEach(translateElement);
    }
    applying = false;
  };

  const updateToggle = () => {
    const button = document.getElementById("language-toggle");
    if (!button) return;
    button.setAttribute("aria-label",
      language === "zh" ? "Switch to English" : "Switch to Chinese");
    button.setAttribute("title",
      language === "zh" ? "Switch to English" : "Switch to Chinese");
    button.querySelectorAll("[data-language]").forEach((item) => {
      item.classList.toggle("active", item.dataset.language === language);
    });
  };

  const setLanguage = (next) => {
    const normalized = next === "en" ? "en" : "zh";
    if (normalized === language) return;
    language = normalized;
    document.documentElement.lang = language === "en" ? "en" : "zh-CN";
    translateTree(document.body);
    updateToggle();
    window.dispatchEvent(new CustomEvent("evoflow:language", {
      detail: { language }
    }));
  };

  const t = (zh, en) => language === "en" ? en : zh;

  window.EvoFlowI18n = Object.freeze({
    get language() { return language; },
    t,
    setLanguage,
    translateTree
  });

  document.getElementById("language-toggle")?.addEventListener("click", () =>
    setLanguage(language === "zh" ? "en" : "zh"));
  updateToggle();

  new MutationObserver((records) => {
    if (applying) return;
    for (const record of records)
      record.addedNodes.forEach((node) => translateTree(node));
  }).observe(document.body, { childList: true, subtree: true });
})();
