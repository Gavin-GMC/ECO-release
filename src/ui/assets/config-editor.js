(() => {
  "use strict";

  const Ui = window.EvoFlowUi;
  if (!Ui) return;

  const WORKFLOW_CATEGORIES = [
    "topology", "repair", "strategy", "generator", "evaluator", "selector"
  ];
  const CATEGORY_LABELS = {
    topology: ["学习拓扑", "Learning topology"],
    strategy: ["学习策略", "Learning strategy"],
    generator: ["子代生成", "Offspring generation"],
    selector: ["环境选择", "Environmental selection"],
    evaluator: ["评价器", "Evaluator"],
    repair: ["修复器", "Repair"],
    archive: ["最优档案", "Best archive"],
    manager: ["子群管理", "Subpopulation manager"],
    constructer: ["子群构建", "Subpopulation builder"],
    ctopology: ["协作拓扑", "Cooperation topology"],
    individual: ["个体", "Individual"],
    feature: ["特性", "Feature"],
    initializer: ["初始化器", "Initializer"]
  };

  const byId = (id) => document.getElementById(id);
  const t = (zh, en) => window.EvoFlowI18n?.t(zh, en) || zh;
  const categoryLabel = (category) => {
    const pair = CATEGORY_LABELS[category];
    return pair ? t(pair[0], pair[1]) : category;
  };
  const escapeHtml = (value) => String(value ?? "")
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#39;");
  const deepCopy = (value) => JSON.parse(JSON.stringify(value));

  const state = {
    initialized: false,
    initializing: false,
    catalog: null,
    draft: null,
    sourceVersion: "v3.1",
    saveVersion: "v3.1",
    fileName: "optimizer.cfg",
    fileHandle: null,
    dirty: false,
    selection: { type: "optimizer", index: 0 },
    issues: [],
    validationTimer: 0,
    validationSequence: 0,
    renderEpoch: 0,
    viewMode: "form",
    runResult: null,
    runRoundIndex: 0,
    catalogQuery: ""
  };
  let flowController = null;

  const showMessage = (text, error = false) => {
    const node = byId("config-message");
    node.hidden = !text;
    node.className = `message config-message${error ? " error" : ""}`;
    node.textContent = text || "";
  };

  const syncReplacementControl = () => {
    const start = byId("config-run-start");
    const replace = byId("config-replace-existing");
    if (!start || !replace) return;
    replace.disabled = start.disabled || start.value.trim() === "";
    if (replace.disabled) replace.checked = false;
  };

  const setBusy = (busy) => {
    ["config-new", "config-open", "config-save", "config-save-as",
      "config-deep-validate", "config-recommend", "config-run",
      "config-run-tag", "config-run-seed", "config-run-runs",
      "config-run-start", "config-replace-existing"].forEach((id) => {
      const node = byId(id);
      if (node) node.disabled = busy;
    });
    if (!busy) syncReplacementControl();
  };

  const canonicalJson = () => JSON.stringify(state.draft);

  const componentList = (category) => state.catalog
    ? state.catalog.components.filter((item) => item.category === category)
    : [];

  const firstTag = (category) => {
    const list = componentList(category);
    return list.length ? list[0].tag : "";
  };

  const optionsHtml = (items, selected, label = (item) => item) =>
    items.map((item) => {
      const value = typeof item === "string" ? item : item.tag;
      return `<option value="${escapeHtml(value)}"${value === selected ? " selected" : ""}>${escapeHtml(label(item))}</option>`;
    }).join("");

  const categoryOptions = (selected) => optionsHtml(
    WORKFLOW_CATEGORIES, selected, categoryLabel
  );

  const tagOptions = (category, selected) =>
    optionsHtml(componentList(category), selected, (item) => item.tag);

  const initializerOptions = (selected) =>
    optionsHtml(state.catalog?.initializers || [], selected, (item) => item.tag);

  const markDirty = () => {
    state.dirty = true;
    state.runResult = null;
    const runResult = byId("config-run-result");
    if (runResult) runResult.hidden = true;
    scheduleValidation();
  };

  const updateStatus = (kind, text) => {
    const badge = byId("config-status");
    badge.className = `status-badge ${kind}`;
    badge.textContent = text;
  };

  const uniqueTag = (prefix, values) => {
    let index = values.length + 1;
    let candidate = `${prefix}${index}`;
    while (values.includes(candidate)) {
      index++;
      candidate = `${prefix}${index}`;
    }
    return candidate;
  };

  const severityRank = (severity) =>
    severity === "error" ? 3 : severity === "warning" ? 2 : severity === "info" ? 1 : 0;

  const scopeSeverity = (scopeType, scopeIndex, componentIndex = -1) => {
    let severity = "";
    state.issues.forEach((issue) => {
      if (issue.scopeType !== scopeType || issue.scopeIndex !== scopeIndex
          || (componentIndex >= 0 && issue.componentIndex !== componentIndex)) return;
      if (severityRank(issue.severity) > severityRank(severity)) severity = issue.severity;
    });
    return severity;
  };

  const scopeIssueClass = (scopeType, scopeIndex, componentIndex = -1) => {
    const severity = scopeSeverity(scopeType, scopeIndex, componentIndex);
    return severity ? ` has-${severity}` : "";
  };

  const refreshComponentIssueClasses = () => {
    document.querySelectorAll(".component-row").forEach((row) => {
      row.classList.remove("has-error", "has-warning", "has-info");
      if (state.selection.type !== "workflow") return;
      const componentIndex = Number(row.id.replace("component-row-", ""));
      const severity = scopeSeverity("workflow", state.selection.index, componentIndex);
      if (severity) row.classList.add(`has-${severity}`);
    });
  };

  const renderCatalog = () => {
    if (!state.catalog) return;
    const query = state.catalogQuery.trim().toLocaleLowerCase();
    const renderHost = (hostId, summaryId, workflowOnly) => {
      const host = byId(hostId);
      const summary = byId(summaryId);
      if (!host) return;
      const categories = state.catalog.categories.filter((category) =>
        !workflowOnly || WORKFLOW_CATEGORIES.includes(category));
      const total = categories.reduce(
        (count, category) => count + componentList(category).length, 0);
      let shown = 0;
      const sections = categories.map((category) => {
        const allItems = componentList(category);
        const pair = CATEGORY_LABELS[category] || [];
        const categoryMatches = query && [
          category, pair[0], pair[1]
        ].some((value) => String(value || "").toLocaleLowerCase().includes(query));
        const items = !query || categoryMatches
          ? allItems
          : allItems.filter((item) =>
            item.tag.toLocaleLowerCase().includes(query));
        if (!items.length) return "";
        shown += items.length;
        const inferred = category === "individual" || category === "feature";
        return `<details${query ? " open" : ""}>
          <summary>${escapeHtml(categoryLabel(category))}
            <span class="catalog-count">${items.length}${inferred
              ? ` · ${t("自动", "inferred")}` : ""}</span>
          </summary>
          ${inferred
            ? `<div class="catalog-note">${escapeHtml(t(
              "装配期自动推断，不直接写入配置。",
              "Inferred during assembly and not written directly to the configuration."
            ))}</div>` : ""}
          <div class="catalog-tags">${items.map((item) =>
            WORKFLOW_CATEGORIES.includes(category)
              ? `<button class="catalog-tag" type="button" draggable="true"
                  data-flow-category="${escapeHtml(category)}"
                  data-flow-tag="${escapeHtml(item.tag)}"
                  title="${escapeHtml(t(
                    "拖入积木画布；双击追加到末尾",
                    "Drag onto the canvas; double-click to append"
                  ))}">${escapeHtml(item.tag)}</button>`
              : `<span class="catalog-tag">${escapeHtml(item.tag)}</span>`).join("")}</div>
        </details>`;
      }).join("");
      host.innerHTML = sections || `<p class="catalog-empty">${escapeHtml(t(
        "未找到匹配组件。", "No matching components."
      ))}</p>`;
      if (summary) {
        summary.textContent = query
          ? t(`显示 ${shown} / ${total} 个组件`,
            `Showing ${shown} of ${total} components`)
          : t(`共 ${total} 个组件`, `${total} components`);
      }
    };
    renderHost("component-catalog-tree", "component-catalog-summary", false);
    renderHost("flow-catalog-tree", "flow-catalog-summary", true);
    [
      ["component-catalog-search", "component-catalog-search-clear"],
      ["flow-catalog-search", "flow-catalog-search-clear"]
    ].forEach(([inputId, clearId]) => {
      const input = byId(inputId);
      const clear = byId(clearId);
      if (input && input.value !== state.catalogQuery) input.value = state.catalogQuery;
      if (clear) clear.hidden = !state.catalogQuery;
    });
  };

  const selectionActive = (type, index = 0) =>
    state.selection.type === type && state.selection.index === index ? " active" : "";

  const renderOutline = () => {
    const host = byId("config-outline");
    if (!state.draft) return;
    const workflows = state.draft.workflows || [];
    const subpopulations = state.draft.optimizer?.subpopulations || [];
    host.innerHTML = `
      <div class="outline-row">
        <button class="outline-button${selectionActive("optimizer")}" data-select="optimizer" type="button">优化器设置</button>
      </div>
      <div class="outline-group-label"><span>Workflows</span><button class="outline-add" data-add="workflow" type="button" title="新增 workflow">+</button></div>
      ${workflows.map((workflow, index) => `
        <div class="outline-row">
          <button class="outline-button${selectionActive("workflow", index)}" data-select="workflow" data-index="${index}" type="button">${escapeHtml(workflow.tag || `workflow ${index + 1}`)}</button>
          <button class="outline-remove" data-remove="workflow" data-index="${index}" type="button" title="删除 workflow">×</button>
        </div>`).join("")}
      <div class="outline-group-label"><span>Subpopulations</span><button class="outline-add" data-add="subpopulation" type="button" title="新增子种群">+</button></div>
      ${subpopulations.map((subpopulation, index) => `
        <div class="outline-row">
          <button class="outline-button${selectionActive("subpopulation", index)}" data-select="subpopulation" data-index="${index}" type="button">${escapeHtml(subpopulation.tag || `subpopulation ${index + 1}`)}</button>
          <button class="outline-remove" data-remove="subpopulation" data-index="${index}" type="button" title="删除子种群">×</button>
        </div>`).join("")}
      <div class="outline-group-label"><span>Cooperation</span></div>
      <div class="outline-row">
        <button class="outline-button${selectionActive("cooperation")}" data-select="cooperation" type="button">协作配置</button>
      </div>`;

    host.querySelectorAll("[data-select]").forEach((button) => {
      button.addEventListener("click", () => {
        const nextSelection = {
          type: button.dataset.select,
          index: Number(button.dataset.index || 0)
        };
        const selectionChanges = nextSelection.type !== state.selection.type
          || nextSelection.index !== state.selection.index;
        if (state.viewMode === "flow" && selectionChanges
            && !flowController?.confirmLeave()) return;
        state.selection = nextSelection;
        renderOutline();
        if (state.viewMode === "flow" && state.selection.type !== "workflow") {
          setEditorView("form");
          return;
        }
        renderForm();
      });
    });
    host.querySelectorAll("[data-add]").forEach((button) => {
      button.addEventListener("click", () => addStructure(button.dataset.add));
    });
    host.querySelectorAll("[data-remove]").forEach((button) => {
      button.addEventListener("click", () =>
        removeStructure(button.dataset.remove, Number(button.dataset.index)));
    });
  };

  const addStructure = (type) => {
    if (state.viewMode === "flow" && !flowController?.confirmLeave()) return;
    if (type === "workflow") {
      const tags = state.draft.workflows.map((item) => item.tag);
      state.draft.workflows.push({
        tag: uniqueTag("wf", tags),
        initializer: { tag: "Random", para: [] },
        components: []
      });
      state.selection = { type: "workflow", index: state.draft.workflows.length - 1 };
    } else {
      const items = state.draft.optimizer.subpopulations;
      const tags = items.map((item) => item.tag);
      items.push({
        tag: uniqueTag("sub", tags),
        size: 100,
        workflow: state.draft.workflows[0]?.tag || "",
        terminator: [-1, -1, -1],
        archive: { tag: "Basic", para: [] }
      });
      state.selection = { type: "subpopulation", index: items.length - 1 };
    }
    markDirty();
    renderOutline();
    if (type !== "workflow" && state.viewMode === "flow") {
      setEditorView("form");
      return;
    }
    if (state.viewMode === "flow") flowController?.render();
    else renderForm();
  };

  const removeStructure = (type, index) => {
    if (state.viewMode === "flow" && !flowController?.confirmLeave()) return;
    const list = type === "workflow"
      ? state.draft.workflows : state.draft.optimizer.subpopulations;
    if (list.length <= 1) {
      showMessage(`至少保留一个${type === "workflow" ? " workflow" : "子种群"}。`, true);
      return;
    }
    const label = list[index]?.tag || `${index + 1}`;
    if (!window.confirm(`删除 ${label}？`)) return;
    const removed = list[index];
    list.splice(index, 1);
    if (type === "workflow") {
      const fallback = state.draft.workflows[0]?.tag || "";
      state.draft.optimizer.subpopulations.forEach((item) => {
        if (item.workflow === removed.tag) item.workflow = fallback;
      });
    }
    state.selection = { type, index: Math.min(index, list.length - 1) };
    markDirty();
    renderOutline();
    if (state.viewMode === "flow" && type === "workflow")
      flowController?.render();
    else renderForm();
  };

  const editorTitle = (eyebrow, title, description) => `
    <div class="editor-title">
      <div><span>${escapeHtml(eyebrow)}</span><h2>${escapeHtml(title)}</h2><p>${escapeHtml(description)}</p></div>
    </div>`;

  const terminatorFields = (prefix, values) => `
    <div class="form-grid three">
      <label class="field">最大 FES
        <input id="${prefix}-fes" type="number" step="1" value="${Number(values?.[0] ?? -1)}">
        <small>-1 表示不设置</small>
      </label>
      <label class="field">最大停滞代数
        <input id="${prefix}-conv" type="number" step="1" value="${Number(values?.[1] ?? -1)}">
        <small>-1 表示不设置</small>
      </label>
      <label class="field">最大时间（秒）
        <input id="${prefix}-time" type="number" step="1" value="${Number(values?.[2] ?? -1)}">
        <small>-1 表示不设置</small>
      </label>
    </div>`;

  const bindNumber = (id, callback) => {
    byId(id)?.addEventListener("change", (event) => {
      callback(Number(event.target.value));
      markDirty();
    });
  };

  const bindText = (id, callback) => {
    byId(id)?.addEventListener("input", (event) => {
      callback(event.target.value);
      markDirty();
    });
  };

  const bindTerminator = (prefix, values) => {
    ["fes", "conv", "time"].forEach((name, index) =>
      bindNumber(`${prefix}-${name}`, (value) => { values[index] = value; }));
  };

  const inlineComponentHtml = (prefix, label, category, value) => `
    <div class="inline-component">
      <div class="form-grid">
        <label class="field">${escapeHtml(label)}
          <select id="${prefix}-tag">${tagOptions(category, value.tag)}</select>
        </label>
      </div>
      <div id="${prefix}-params" class="parameter-host"></div>
    </div>`;

  const renderOptimizer = () => {
    const optimizer = state.draft.optimizer;
    const host = byId("config-form");
    host.innerHTML = `${editorTitle("Optimizer", "优化器设置", "名称、预算、日志与全局档案。")}
      <section class="form-section">
        <div class="form-grid">
          <label class="field">名称
            <input id="optimizer-name" required pattern="[A-Za-z0-9_.-]+" value="${escapeHtml(optimizer.name)}">
            <small>保存到 v3.1 时作为单个标识符，不使用空格。</small>
          </label>
          <label class="field">配置默认标签
            <input id="optimizer-tag" pattern="[A-Za-z0-9_.-]*" value="${escapeHtml(optimizer.tag)}">
            <small>保存在配置中；运行设置未填写标签时使用此值。</small>
          </label>
        </div>
      </section>
      <section class="form-section">
        <div class="section-heading"><h3>终止条件</h3><small>三项均为 -1 时使用平台默认预算</small></div>
        ${terminatorFields("optimizer-term", optimizer.terminator)}
      </section>
      <section class="form-section">
        <div class="section-heading"><h3>日志</h3></div>
        <div class="check-grid">
          ${["完整结果", "过程日志", "完整过程", "整群快照"].map((label, index) =>
            `<label class="check-field"><input id="logging-${index}" type="checkbox"${optimizer.logging[index] ? " checked" : ""}>${label}</label>`).join("")}
        </div>
      </section>
      <section class="form-section">
        <div class="section-heading"><h3>全局档案</h3></div>
        ${inlineComponentHtml("global-archive", "档案组件", "archive", optimizer.g_archive)}
      </section>`;

    bindText("optimizer-name", (value) => { optimizer.name = value; });
    bindText("optimizer-tag", (value) => { optimizer.tag = value; });
    bindTerminator("optimizer-term", optimizer.terminator);
    optimizer.logging.forEach((_, index) => {
      byId(`logging-${index}`).addEventListener("change", (event) => {
        optimizer.logging[index] = event.target.checked;
        markDirty();
      });
    });
    byId("global-archive-tag").addEventListener("change", (event) => {
      optimizer.g_archive.tag = event.target.value;
      optimizer.g_archive.para = [];
      recommendParameters(optimizer.g_archive, "archive");
    });
    mountParameterEditor("global-archive-params", "archive",
      optimizer.g_archive.tag, optimizer.g_archive.para,
      (values) => { optimizer.g_archive.para = values; });
  };

  const renderWorkflow = (index) => {
    const workflow = state.draft.workflows[index];
    if (!workflow) {
      state.selection = { type: "optimizer", index: 0 };
      renderOutline();
      renderOptimizer();
      return;
    }
    const host = byId("config-form");
    host.innerHTML = `${editorTitle("Workflow", workflow.tag || `Workflow ${index + 1}`,
      "组件按列表顺序执行；同一类别可以重复出现。")}
      <section class="form-section">
        <div class="form-grid">
          <label class="field">Workflow 标识
            <input id="workflow-tag" required pattern="[A-Za-z0-9_.-]+" value="${escapeHtml(workflow.tag)}">
            <small>修改后自动同步引用此 workflow 的子种群。</small>
          </label>
          <label class="field">初始化器
            <select id="workflow-initializer">${initializerOptions(workflow.initializer.tag)}</select>
          </label>
        </div>
        <div id="workflow-initializer-params" class="parameter-host"></div>
      </section>
      <section class="form-section">
        <div class="section-heading">
          <div><h3>有序组件</h3><small>SET 型组件应位于消费它的生成组件之前</small></div>
          <button id="add-component" class="toolbar-button" type="button">添加组件</button>
        </div>
        <div id="workflow-components" class="component-list">
          ${workflow.components.length ? workflow.components.map((component, componentIndex) => `
            <article id="component-row-${componentIndex}" class="component-row${scopeIssueClass("workflow", index, componentIndex)}">
              <div class="component-row-head">
                <span class="component-order">${String(componentIndex + 1).padStart(2, "0")}</span>
                <label class="field">类别
                  <select data-component-category="${componentIndex}">${categoryOptions(component.type)}</select>
                </label>
                <label class="field">组件
                  <select data-component-tag="${componentIndex}">${tagOptions(component.type, component.tag)}</select>
                </label>
                <div class="row-actions">
                  <button class="icon-button" data-move="-1" data-index="${componentIndex}" type="button" title="上移">↑</button>
                  <button class="icon-button" data-move="1" data-index="${componentIndex}" type="button" title="下移">↓</button>
                  <button class="icon-button" data-copy="${componentIndex}" type="button" title="复制">⧉</button>
                  <button class="icon-button danger" data-delete="${componentIndex}" type="button" title="删除">×</button>
                </div>
              </div>
              <div id="component-params-${componentIndex}" class="parameter-host"></div>
            </article>`).join("") : '<div class="empty-state compact">尚无组件。请按执行顺序添加 topology、repair、strategy、generator、evaluator 与 selector。</div>'}
        </div>
      </section>`;

    bindText("workflow-tag", (value) => {
      const previous = workflow.tag;
      workflow.tag = value;
      state.draft.optimizer.subpopulations.forEach((subpopulation) => {
        if (subpopulation.workflow === previous) subpopulation.workflow = value;
      });
      renderOutline();
    });
    byId("workflow-initializer").addEventListener("change", (event) => {
      workflow.initializer.tag = event.target.value;
      workflow.initializer.para = [];
      recommendParameters(workflow.initializer, "initializer");
    });
    mountParameterEditor("workflow-initializer-params", "initializer",
      workflow.initializer.tag, workflow.initializer.para,
      (values) => { workflow.initializer.para = values; });

    byId("add-component").addEventListener("click", () => {
      const component = {
        type: "topology",
        tag: firstTag("topology"),
        para: []
      };
      workflow.components.push(component);
      markDirty();
      renderForm();
      recommendParameters(component, component.type);
    });

    workflow.components.forEach((component, componentIndex) => {
      host.querySelector(`[data-component-category="${componentIndex}"]`)
        .addEventListener("change", (event) => {
          component.type = event.target.value;
          component.tag = firstTag(component.type);
          component.para = [];
          markDirty();
          renderForm();
          recommendParameters(component, component.type);
        });
      host.querySelector(`[data-component-tag="${componentIndex}"]`)
        .addEventListener("change", (event) => {
          component.tag = event.target.value;
          component.para = [];
          markDirty();
          renderForm();
          recommendParameters(component, component.type);
        });
      mountParameterEditor(`component-params-${componentIndex}`, component.type,
        component.tag, component.para, (values) => { component.para = values; });
    });

    host.querySelectorAll("[data-move]").forEach((button) => {
      button.addEventListener("click", () => {
        const from = Number(button.dataset.index);
        const to = from + Number(button.dataset.move);
        if (to < 0 || to >= workflow.components.length) return;
        const [item] = workflow.components.splice(from, 1);
        workflow.components.splice(to, 0, item);
        markDirty();
        renderForm();
      });
    });
    host.querySelectorAll("[data-copy]").forEach((button) => {
      button.addEventListener("click", () => {
        const indexToCopy = Number(button.dataset.copy);
        workflow.components.splice(indexToCopy + 1, 0,
          deepCopy(workflow.components[indexToCopy]));
        markDirty();
        renderForm();
      });
    });
    host.querySelectorAll("[data-delete]").forEach((button) => {
      button.addEventListener("click", () => {
        workflow.components.splice(Number(button.dataset.delete), 1);
        markDirty();
        renderForm();
      });
    });
  };

  const renderSubpopulation = (index) => {
    const subpopulation = state.draft.optimizer.subpopulations[index];
    if (!subpopulation) {
      state.selection = { type: "optimizer", index: 0 };
      renderOutline();
      renderOptimizer();
      return;
    }
    const host = byId("config-form");
    host.innerHTML = `${editorTitle("Subpopulation", subpopulation.tag || `Subpopulation ${index + 1}`,
      "设置规模、绑定 workflow、局部预算与档案。")}
      <section class="form-section">
        <div class="form-grid">
          <label class="field">子种群标识
            <input id="subpopulation-tag" required pattern="[A-Za-z0-9_.-]+" value="${escapeHtml(subpopulation.tag)}">
          </label>
          <label class="field">规模
            <input id="subpopulation-size" type="number" min="1" step="1" value="${Number(subpopulation.size)}">
          </label>
          <label class="field">Workflow
            <select id="subpopulation-workflow">${optionsHtml(state.draft.workflows.map((item) => item.tag), subpopulation.workflow)}</select>
          </label>
        </div>
      </section>
      <section class="form-section">
        <div class="section-heading"><h3>局部终止条件</h3></div>
        ${terminatorFields("subpopulation-term", subpopulation.terminator)}
      </section>
      <section class="form-section">
        <div class="section-heading"><h3>子种群档案</h3></div>
        ${inlineComponentHtml("subpopulation-archive", "档案组件", "archive", subpopulation.archive)}
      </section>`;

    bindText("subpopulation-tag", (value) => {
      subpopulation.tag = value;
      renderOutline();
    });
    bindNumber("subpopulation-size", (value) => { subpopulation.size = value; });
    byId("subpopulation-workflow").addEventListener("change", (event) => {
      subpopulation.workflow = event.target.value;
      markDirty();
    });
    bindTerminator("subpopulation-term", subpopulation.terminator);
    byId("subpopulation-archive-tag").addEventListener("change", (event) => {
      subpopulation.archive.tag = event.target.value;
      subpopulation.archive.para = [];
      recommendParameters(subpopulation.archive, "archive");
    });
    mountParameterEditor("subpopulation-archive-params", "archive",
      subpopulation.archive.tag, subpopulation.archive.para,
      (values) => { subpopulation.archive.para = values; });
  };

  const renderCooperation = () => {
    const optimizer = state.draft.optimizer;
    const definitions = [
      ["manager", "子群管理器", "manager", optimizer.manager],
      ["constructer", "子群构建器", "constructer", optimizer.constructer],
      ["cooperation-topology", "协作拓扑", "ctopology", optimizer.ctopology]
    ];
    const host = byId("config-form");
    host.innerHTML = `${editorTitle("Cooperation", "协作配置", "管理多个子种群的构建方式、调度方式和连接拓扑。")}
      ${definitions.map(([prefix, label, category, value]) => `
        <section class="form-section">
          <div class="section-heading"><h3>${escapeHtml(label)}</h3></div>
          ${inlineComponentHtml(prefix, "组件", category, value)}
        </section>`).join("")}`;

    definitions.forEach(([prefix, , category, value]) => {
      byId(`${prefix}-tag`).addEventListener("change", (event) => {
        value.tag = event.target.value;
        value.para = [];
        recommendParameters(value, category);
      });
      mountParameterEditor(`${prefix}-params`, category, value.tag, value.para,
        (values) => { value.para = values; });
    });
  };

  const renderForm = () => {
    state.renderEpoch++;
    if (!state.draft || !state.catalog) return;
    switch (state.selection.type) {
      case "workflow":
        renderWorkflow(state.selection.index);
        break;
      case "subpopulation":
        renderSubpopulation(state.selection.index);
        break;
      case "cooperation":
        renderCooperation();
        break;
      default:
        renderOptimizer();
    }
    flowController?.render();
  };

  const setEditorView = (mode) => {
    const nextMode = mode === "flow" ? "flow" : "form";
    if (state.viewMode === "flow" && nextMode !== "flow"
        && !flowController?.confirmLeave()) return false;
    if (mode === "flow") {
      if (!state.draft?.workflows?.length) {
        showMessage("当前配置没有 workflow，无法打开积木视图。", true);
        return false;
      }
      if (state.selection.type !== "workflow") {
        state.selection = { type: "workflow", index: 0 };
        renderOutline();
      }
    }
    state.viewMode = nextMode;
    if (state.viewMode !== "flow") flowController?.exitFocusMode();
    byId("config-form").hidden = state.viewMode !== "form";
    byId("config-flow").hidden = state.viewMode !== "flow";
    byId("config-view-form").classList.toggle("active", state.viewMode === "form");
    byId("config-view-flow").classList.toggle("active", state.viewMode === "flow");
    if (state.viewMode === "flow") flowController?.render();
    else renderForm();
    return true;
  };

  const schemaBody = (values) => values.map((value) =>
    value === null || Number.isNaN(value) ? "null" : String(value)).join(",");

  const fetchSchema = (category, tag, values) => Ui.api(
    `/api/config/schema?category=${encodeURIComponent(category)}&tag=${encodeURIComponent(tag)}`,
    {
      method: "POST",
      headers: { "Content-Type": "text/plain" },
      body: schemaBody(values)
    }
  );

  const recommendParameters = async (target, category) => {
    const tag = target.tag;
    const data = await fetchSchema(category, tag, []);
    if (!data.ok || target.tag !== tag) return;
    target.para = [...data.defaults];
    markDirty();
    renderForm();
  };

  const mountParameterEditor = async (
    hostId, category, tag, values, apply, refresh = renderForm,
    afterChange = markDirty
  ) => {
    const host = byId(hostId);
    if (!host || !tag) return;
    const epoch = state.renderEpoch;
    host.innerHTML = '<p class="muted">正在解析参数…</p>';
    const data = await fetchSchema(category, tag, values);
    if (epoch !== state.renderEpoch || !host.isConnected) return;
    if (!data.ok) {
      host.innerHTML = `<p class="small-danger">${escapeHtml(data.error || "参数信息不可用")}</p>`;
      return;
    }
    if (!data.params.length) {
      host.innerHTML = '<p class="muted">此组件没有参数。</p>';
      return;
    }

    const useDefaults = values.length === 0;
    host.innerHTML = `
      <label class="defaults-row">
        <input data-use-defaults type="checkbox"${useDefaults ? " checked" : ""}>
        使用组件内部默认参数（配置中不写参数）
      </label>
      <div class="parameter-grid">
        ${data.params.map((parameter, index) => {
          const current = useDefaults ? data.defaults[index]
            : (index < values.length ? values[index] : data.defaults[index]);
          const empty = current === null;
          const enumCount = parameter.high - parameter.low + 1;
          const control = parameter.kind === 0 && Number.isInteger(enumCount) && enumCount <= 100
            ? `<select aria-label="${escapeHtml(parameter.name)}" data-param-value="${index}"${useDefaults || empty ? " disabled" : ""}>${Array.from(
                { length: enumCount }, (_, offset) => parameter.low + offset
              ).map((option) => `<option value="${option}"${Number(current) === option ? " selected" : ""}>${option}</option>`).join("")}</select>`
            : `<input aria-label="${escapeHtml(parameter.name)}" data-param-value="${index}" type="number" min="${parameter.low}" max="${parameter.high}" step="${parameter.kind === 2 ? "any" : "1"}" value="${empty ? "" : escapeHtml(current)}"${useDefaults || empty ? " disabled" : ""}>`;
          return `<div class="parameter-field">
            <label>${escapeHtml(parameter.name)}</label>
            ${parameter.allowEmpty ? `<label class="empty-toggle"><input data-param-empty="${index}" type="checkbox"${empty ? " checked" : ""}${useDefaults ? " disabled" : ""}> 留空</label>` : ""}
            ${control}
            <span class="parameter-meta">硬边界 [${parameter.low}, ${parameter.high}]<br>推荐 [${parameter.recLow}, ${parameter.recHigh}] · ${["Enum", "Int", "Real"][parameter.kind] || "Real"}</span>
          </div>`;
        }).join("")}
      </div>`;

    host.querySelector("[data-use-defaults]").addEventListener("change", (event) => {
      apply(event.target.checked ? [] : [...data.defaults]);
      afterChange();
      refresh();
    });
    data.params.forEach((parameter, index) => {
      const valueControl = host.querySelector(`[data-param-value="${index}"]`);
      valueControl?.addEventListener("change", (event) => {
        const next = values.length ? data.defaults.map((fallback, i) =>
          i < values.length ? values[i] : fallback) : [...data.defaults];
        next.length = data.params.length;
        next[index] = Number(event.target.value);
        apply(next);
        afterChange();
        refresh();
      });
      const emptyControl = host.querySelector(`[data-param-empty="${index}"]`);
      emptyControl?.addEventListener("change", (event) => {
        const next = values.length ? data.defaults.map((fallback, i) =>
          i < values.length ? values[i] : fallback) : [...data.defaults];
        next.length = data.params.length;
        next[index] = event.target.checked ? null : data.defaults[index];
        apply(next);
        afterChange();
        refresh();
      });
    });
  };

  const renderValidation = () => {
    const host = byId("validation-list");
    const errors = state.issues.filter((issue) => issue.severity === "error").length;
    const warnings = state.issues.filter((issue) => issue.severity === "warning").length;
    const infos = state.issues.filter((issue) => issue.severity === "info").length;
    byId("validation-summary").textContent = state.issues.length
      ? `${errors} 错误 · ${warnings} 警告 · ${infos} 信息`
      : "PASS · 未发现配置问题";
    if (!state.issues.length) {
      host.innerHTML = '<div class="empty-state compact">PASS · 静态检查未发现配置问题。</div>';
      return;
    }
    host.innerHTML = state.issues.map((issue, index) => `
      <button class="validation-item ${issue.severity}" data-issue="${index}" type="button">
        <span class="validation-mark"></span>
        <span>
          <strong><span class="validation-level">${escapeHtml(issue.severity.toUpperCase())}</span>
            ${escapeHtml(issue.scope)}${issue.componentIndex >= 0 ? ` · component[${issue.componentIndex}]` : ""}
            <span class="reason-code">${escapeHtml(issue.reason)}</span>
          </strong>
          <p>${escapeHtml(issue.message)}</p>
        </span>
      </button>`).join("");
    host.querySelectorAll("[data-issue]").forEach((button) => {
      button.addEventListener("click", () => focusIssue(state.issues[Number(button.dataset.issue)]));
    });
  };

  const focusIssue = (issue) => {
    const targetType = issue.scopeType === "workflow"
      ? "workflow" : issue.scopeType === "subpopulation" ? "subpopulation" : "";
    const selectionChanges = targetType
      && (targetType !== state.selection.type || issue.scopeIndex !== state.selection.index);
    if (state.viewMode === "flow" && selectionChanges
        && !flowController?.confirmLeave()) return;
    if (issue.scopeType === "workflow") {
      state.selection = { type: "workflow", index: issue.scopeIndex };
    } else if (issue.scopeType === "subpopulation") {
      state.selection = { type: "subpopulation", index: issue.scopeIndex };
    } else {
      return;
    }
    renderOutline();
    if (state.viewMode === "flow" && issue.scopeType === "workflow") {
      flowController?.select(issue.componentIndex);
      return;
    }
    if (state.viewMode === "flow") setEditorView("form");
    else renderForm();
    if (issue.componentIndex >= 0) {
      window.setTimeout(() => {
        byId(`component-row-${issue.componentIndex}`)?.scrollIntoView({
          behavior: "smooth", block: "center"
        });
      }, 50);
    }
  };

  const scheduleValidation = () => {
    window.clearTimeout(state.validationTimer);
    // 立即使已在途的旧请求失效，避免下一次 debounce 尚未发出时旧结果回写。
    state.validationSequence++;
    updateStatus("pending", "正在检查…");
    state.validationTimer = window.setTimeout(() => runValidation(false), 350);
  };

  const runValidation = async (deepBuild = false) => {
    if (!state.draft) return null;
    const sequence = ++state.validationSequence;
    if (deepBuild) {
      setBusy(true);
      showMessage("正在执行装配验证…");
    }
    try {
      const report = await Ui.api(
        `/api/config/validate?name=${encodeURIComponent(state.fileName)}&deep=${deepBuild ? "1" : "0"}`,
        {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: canonicalJson()
        }
      );
      if (sequence !== state.validationSequence) return null;
      if (!report.ok) {
        updateStatus("error", "校验服务失败");
        showMessage(report.error || "配置校验失败", true);
        return report;
      }
      state.issues = report.issues || [];
      const errors = state.issues.filter((issue) => issue.severity === "error").length;
      const warnings = state.issues.filter((issue) => issue.severity === "warning").length;
      const infos = state.issues.filter((issue) => issue.severity === "info").length;
      if (errors) updateStatus("error", `${errors} 个错误`);
      else if (warnings) updateStatus("warning", `${warnings} 个警告`);
      else if (infos) updateStatus("info", `${infos} 条信息`);
      else updateStatus("ok", deepBuild ? "装配通过" : "静态校验通过");
      renderValidation();
      flowController?.render();
      refreshComponentIssueClasses();
      if (deepBuild) {
        const message = errors ? "装配验证发现错误。"
          : warnings ? "装配验证通过，但存在运行可靠性警告。"
            : infos ? "装配验证通过，同时存在配置信息提示。" : "装配验证通过。";
        showMessage(message, errors > 0);
      }
      return report;
    } catch (error) {
      if (sequence === state.validationSequence) {
        updateStatus("error", "校验服务失败");
        showMessage(error?.message || "配置校验失败", true);
      }
      return null;
    } finally {
      if (deepBuild) setBusy(false);
    }
  };

  const applyDraft = (data, fileName, handle = null) => {
    try {
      state.draft = JSON.parse(data.canonicalJson);
    } catch (_) {
      showMessage("服务返回的配置数据无法识别。", true);
      return false;
    }
    state.sourceVersion = data.sourceVersion || "v3.1";
    state.saveVersion = state.sourceVersion;
    state.fileName = fileName || `${state.draft.optimizer?.name || "optimizer"}.cfg`;
    state.fileHandle = handle;
    state.dirty = false;
    state.selection = { type: "optimizer", index: 0 };
    state.viewMode = "form";
    state.issues = [];
    flowController?.exitFocusMode();
    byId("config-version").value = state.saveVersion;
    byId("config-form").hidden = false;
    byId("config-flow").hidden = true;
    byId("config-view-form").classList.add("active");
    byId("config-view-flow").classList.remove("active");
    state.runResult = null;
    state.runRoundIndex = 0;
    byId("config-run-result").hidden = true;
    renderOutline();
    renderForm();
    renderValidation();
    scheduleValidation();
    return true;
  };

  const newDraft = async () => {
    if (state.viewMode === "flow" && !flowController?.confirmLeave()) return;
    if (state.dirty && !window.confirm("当前配置尚未保存，仍然新建？")) return;
    setBusy(true);
    const data = await Ui.api("/api/config/new");
    setBusy(false);
    if (!data.ok) {
      showMessage(data.error || "无法创建配置", true);
      return;
    }
    if (applyDraft(data, "optimizer.cfg")) showMessage("已创建空白配置骨架。");
  };

  const loadFile = async (file, handle = null) => {
    if (!file) return;
    if (file.size > 64 * 1024 * 1024) {
      showMessage("配置文件超过 64 MiB。", true);
      return;
    }
    setBusy(true);
    showMessage(`正在打开 ${file.name}…`);
    const data = await Ui.api(
      `/api/config/open?name=${encodeURIComponent(file.name)}`,
      {
        method: "POST",
        headers: { "Content-Type": "application/octet-stream" },
        body: file
      }
    );
    setBusy(false);
    if (!data.ok) {
      showMessage(data.error || "配置无法解析", true);
      return;
    }
    if (applyDraft(data, file.name, handle)) showMessage(`已打开 ${file.name}`);
  };

  const openDraft = async () => {
    if (state.viewMode === "flow" && !flowController?.confirmLeave()) return;
    if (state.dirty && !window.confirm("当前配置尚未保存，仍然打开其他文件？")) return;
    if (window.showOpenFilePicker) {
      try {
        const [handle] = await window.showOpenFilePicker({
          multiple: false,
          types: [{ description: "EvoFlow 配置", accept: { "text/plain": [".cfg", ".json"] } }]
        });
        await loadFile(await handle.getFile(), handle);
      } catch (error) {
        if (error.name !== "AbortError") showMessage(error.message || "打开文件失败", true);
      }
    } else {
      byId("config-file-input").click();
    }
  };

  const suggestedFileName = () => {
    const raw = state.draft?.optimizer?.name || state.fileName.replace(/\.[^.]+$/, "") || "optimizer";
    const safe = raw.replace(/[^A-Za-z0-9_.-]+/g, "_");
    return `${safe || "optimizer"}.cfg`;
  };

  const encodeDraft = async () => {
    const version = byId("config-version").value;
    const data = await Ui.api(
      `/api/config/encode?version=${encodeURIComponent(version)}`,
      {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: canonicalJson()
      }
    );
    if (!data.ok) throw new Error(data.error || "配置编码失败");
    return data;
  };

  const confirmInvalidSave = async () => {
    const report = await runValidation(false);
    if (!report?.ok) return false;
    const errors = (report.issues || []).filter((issue) => issue.severity === "error").length;
    return errors === 0 || window.confirm(
      `当前配置仍有 ${errors} 个错误。保存后可能无法运行，仍然继续吗？`
    );
  };

  const downloadFallback = (text, name) => {
    const url = URL.createObjectURL(new Blob([text], { type: "text/plain;charset=utf-8" }));
    const link = document.createElement("a");
    link.href = url;
    link.download = name;
    link.click();
    window.setTimeout(() => URL.revokeObjectURL(url), 1000);
  };

  const writeHandle = async (handle, text) => {
    const writable = await handle.createWritable();
    await writable.write(text);
    await writable.close();
  };

  const saveAs = async (skipValidation = false) => {
    if (!skipValidation && !await confirmInvalidSave()) return;
    try {
      const encoded = await encodeDraft();
      const name = suggestedFileName();
      if (window.showSaveFilePicker) {
        const handle = await window.showSaveFilePicker({
          suggestedName: name,
          types: [{ description: "EvoFlow 配置", accept: { "text/plain": [".cfg"] } }]
        });
        await writeHandle(handle, encoded.text);
        state.fileHandle = handle;
        state.fileName = handle.name || name;
      } else {
        downloadFallback(encoded.text, name);
        state.fileHandle = null;
        state.fileName = name;
      }
      state.sourceVersion = encoded.version;
      state.dirty = false;
      showMessage(`已保存 ${state.fileName}`);
    } catch (error) {
      if (error.name !== "AbortError") showMessage(error.message || "保存失败", true);
    }
  };

  const save = async () => {
    if (!await confirmInvalidSave()) return;
    if (!state.fileHandle) {
      await saveAs(true);
      return;
    }
    try {
      const encoded = await encodeDraft();
      await writeHandle(state.fileHandle, encoded.text);
      state.sourceVersion = encoded.version;
      state.dirty = false;
      showMessage(`已保存 ${state.fileName}`);
    } catch (error) {
      showMessage(error.message || "保存失败", true);
    }
  };

  const renderRunRound = () => {
    const data = state.runResult;
    if (!data) return;
    const panel = byId("config-run-result");
    const summary = byId("config-run-summary");
    const solutions = byId("config-run-solutions");
    const rounds = Array.isArray(data.rounds) && data.rounds.length
      ? data.rounds
      : [{ run: 1, bestFitness: data.bestFitness, best: data.best || [] }];
    state.runRoundIndex = Math.max(
      0, Math.min(rounds.length - 1, state.runRoundIndex)
    );
    const round = rounds[state.runRoundIndex];
    const best = round.best || [];
    summary.replaceChildren();
    solutions.replaceChildren();

    [
      ["当前轮次", `${round.run || state.runRoundIndex + 1} / ${data.runs || rounds.length}`],
      ["最优适应度", round.bestFitness ?? "—"],
      ["返回解数量", best.length],
      ["运行标签", data.tag || "—"],
      ["优化器标识", data.optimizerName || "—"],
      ["结果编号", Number.isInteger(round.resultIndex) ? round.resultIndex : "—"],
      ["种子 / 总轮数", `${data.seed} / ${data.runs}`]
    ].forEach(([label, value]) => {
      const card = document.createElement("article");
      const name = document.createElement("span");
      const text = document.createElement("strong");
      name.textContent = label;
      text.textContent = String(value);
      card.append(name, text);
      summary.appendChild(card);
    });

    best.forEach((solution, index) => {
      const card = document.createElement("article");
      card.className = "run-solution";
      const title = document.createElement("h3");
      title.textContent = `解 ${index + 1} · fitness ${solution.fitness}`;
      const objectives = document.createElement("p");
      objectives.textContent = `目标值：${(solution.objectives || []).join("，") || "—"}`;
      const text = document.createElement("pre");
      text.textContent = solution.text || "未返回解文本";
      card.append(title, objectives, text);
      solutions.appendChild(card);
    });
    const pagination = byId("config-run-pagination");
    pagination.hidden = rounds.length <= 1;
    byId("config-run-page-label").textContent =
      `第 ${round.run || state.runRoundIndex + 1} / ${data.runs || rounds.length} 轮`;
    byId("config-run-previous").disabled = state.runRoundIndex === 0;
    byId("config-run-next").disabled = state.runRoundIndex === rounds.length - 1;
    panel.hidden = false;
    byId("config-run-state").className = "status-badge ok";
    byId("config-run-state").textContent = "运行完成";
  };

  const renderRunResult = (data) => {
    state.runResult = data;
    state.runRoundIndex = 0;
    renderRunRound();
  };

  const moveRunRound = (delta) => {
    if (!state.runResult) return;
    state.runRoundIndex += delta;
    renderRunRound();
  };

  const updateRunProgress = (data) => {
    const host = byId("config-run-progress");
    const bar = byId("config-run-progress-bar");
    const total = Math.max(1, Number(data.totalRuns || data.runs || 1));
    const completed = Math.max(0, Math.min(total, Number(data.completedRuns || 0)));
    const current = Math.max(1, Math.min(total, Number(data.currentRun || completed + 1)));
    const finished = data.state === "completed"
      || data.state === "failed" || data.state === "cancelled";
    const percent = finished && data.state === "completed"
      ? 100 : Math.round((completed / total) * 100);
    host.hidden = false;
    bar.max = total;
    bar.value = finished && data.state === "completed" ? total : completed;
    byId("config-run-progress-percent").textContent = `${percent}%`;
    byId("config-run-progress-text").textContent = finished
      ? `已完成 ${completed} / ${total} 轮`
      : `正在运行第 ${current} / ${total} 轮 · 已完成 ${completed} 轮`;
    if (!finished) {
      byId("config-run-state").className = "status-badge pending";
      byId("config-run-state").textContent = `第 ${current} / ${total} 轮`;
      showMessage(`正在运行第 ${current} / ${total} 轮，已完成 ${completed} 轮。`);
    }
  };

  const pollRunJob = async (jobId) => {
    for (;;) {
      const data = await Ui.api(
        `/api/run/status?id=${encodeURIComponent(jobId)}`
      );
      if (!data.ok) throw new Error(data.error || "无法读取运行进度");
      updateRunProgress(data);
      if (data.state === "running") {
        await new Promise((resolve) => window.setTimeout(resolve, 350));
        continue;
      }
      if (data.state === "failed") {
        throw new Error(data.error || "优化运行失败");
      }
      if (data.state === "cancelled") {
        throw new Error("优化运行已取消");
      }
      if (data.state !== "completed") {
        throw new Error("运行状态无法识别");
      }
      renderRunResult(data);
      return data;
    }
  };

  const readRunParameters = () => {
    const tag = byId("config-run-tag").value.trim();
    const seed = Number(byId("config-run-seed").value);
    const runs = Number(byId("config-run-runs").value);
    const startText = byId("config-run-start").value.trim();
    const runStart = startText === "" ? -1 : Number(startText);
    const replaceExisting = byId("config-replace-existing").checked;
    if (tag.length > 128 || !/^[A-Za-z0-9_.-]*$/.test(tag)) {
      throw new Error("运行标签只能包含字母、数字、下划线、点和连字符，最多 128 个字符。");
    }
    if (!Number.isInteger(seed) || seed < 0 || seed > 4294967295) {
      throw new Error("种子必须是 0 到 4294967295 之间的整数。");
    }
    if (!Number.isInteger(runs) || runs < 1 || runs > 10000) {
      throw new Error("运行轮数必须是 1 到 10000 之间的整数。");
    }
    if (runStart !== -1
        && (!Number.isInteger(runStart) || runStart < 0 || runStart > 2147483647)) {
      throw new Error("结果起点必须留空，或填写 0 到 2147483647 之间的整数。");
    }
    if (replaceExisting && runStart < 0) {
      throw new Error("覆写旧结果前必须填写明确的结果起点。");
    }
    return { tag, seed, runs, runStart, replaceExisting };
  };

  const runCurrentConfig = async () => {
    if (!state.draft) {
      showMessage("配置编辑器尚未完成初始化。", true);
      return;
    }
    if (!window.EvoFlowProblem?.hasCurrent()) {
      showMessage("请先在问题页面加载或构建一个问题。", true);
      return;
    }
    let runParameters;
    try {
      runParameters = readRunParameters();
    } catch (error) {
      showMessage(error.message, true);
      return;
    }
    if (runParameters.replaceExisting
        && !window.confirm(
          `将事务性覆写从结果编号 ${runParameters.runStart} 开始的 ${runParameters.runs} 轮旧结果。确认继续吗？`
        )) return;
    const report = await runValidation(true);
    if (!report?.ok) return;
    const errors = (report.issues || [])
      .filter((issue) => issue.severity === "error").length;
    if (errors) {
      showMessage(`当前配置仍有 ${errors} 个错误，修复后才能运行。`, true);
      return;
    }

    const button = byId("config-run");
    const panel = byId("config-run-result");
    state.runResult = null;
    state.runRoundIndex = 0;
    panel.hidden = false;
    byId("config-run-pagination").hidden = true;
    byId("config-run-summary").replaceChildren();
    byId("config-run-solutions").replaceChildren();
    updateRunProgress({
      state: "running",
      completedRuns: 0,
      currentRun: 1,
      totalRuns: runParameters.runs
    });
    byId("config-run-state").className = "status-badge pending";
    byId("config-run-state").textContent = `第 1 / ${runParameters.runs} 轮`;
    setBusy(true);
    button.textContent = "运行中…";
    showMessage("正在使用当前问题和配置运行优化；耗时由配置中的终止预算决定。");
    try {
      const query = new URLSearchParams({
        tag: runParameters.tag,
        seed: String(runParameters.seed),
        runs: String(runParameters.runs),
        runStart: runParameters.runStart < 0 ? "" : String(runParameters.runStart),
        replaceExisting: String(runParameters.replaceExisting)
      });
      const started = await Ui.api(`/api/run/start?${query}`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: canonicalJson()
      });
      if (!started.ok) throw new Error(started.error || "优化运行启动失败");
      await pollRunJob(started.jobId);
      showMessage("当前配置运行完成。");
    } catch (error) {
      byId("config-run-state").className = "status-badge error";
      byId("config-run-state").textContent = "运行失败";
      showMessage(error.message || "优化运行失败", true);
    } finally {
      setBusy(false);
      button.textContent = "运行当前配置";
    }
  };

  const initialize = async () => {
    if (state.initialized || state.initializing) return;
    state.initializing = true;
    setBusy(true);
    const catalog = await Ui.api("/api/config/catalog");
    if (!catalog.ok) {
      state.initializing = false;
      setBusy(false);
      showMessage(catalog.error || "无法读取组件目录", true);
      return;
    }
    state.catalog = catalog;
    renderCatalog();
    const draft = await Ui.api("/api/config/new");
    state.initializing = false;
    state.initialized = true;
    setBusy(false);
    if (!draft.ok || !applyDraft(draft, "optimizer.cfg")) {
      showMessage(draft.error || "无法初始化配置编辑器", true);
      return;
    }
    showMessage("已准备空白配置骨架。");
  };

  flowController = window.EvoFlowFlowEditor?.create({
    getDraft: () => state.draft,
    getWorkflowIndex: () =>
      state.selection.type === "workflow" ? state.selection.index : -1,
    getWorkflow: () =>
      state.selection.type === "workflow"
        ? state.draft?.workflows?.[state.selection.index] || null
        : null,
    getIssues: () => state.issues,
    componentsFor: (category) => componentList(category),
    commitComponents: (components, focusIndex) => {
      if (state.selection.type !== "workflow") return;
      const workflow = state.draft.workflows[state.selection.index];
      if (!workflow) return;
      workflow.components = deepCopy(components);
      markDirty();
      renderOutline();
    },
    mountParameters: (hostId, component, afterChange, refresh) => {
      if (!component) return;
      mountParameterEditor(
        hostId, component.type, component.tag, component.para,
        (values) => { component.para = values; },
        refresh,
        afterChange
      );
    },
    openFormAt: (componentIndex) => {
      setEditorView("form");
      window.setTimeout(() => {
        byId(`component-row-${componentIndex}`)?.scrollIntoView({
          behavior: "smooth", block: "center"
        });
      }, 50);
    }
  }) || null;

  const acceptRecommendedDraft = async (data) => {
    if (!state.initialized) await initialize();
    if (!state.initialized) {
      return { ok: false, error: "配置编辑器尚未完成初始化。" };
    }
    if (state.viewMode === "flow" && !flowController?.confirmLeave())
      return { ok: false, cancelled: true };
    if (state.dirty
        && !window.confirm("当前配置尚未保存，仍然载入模型推荐的现有配置？")) {
      return { ok: false, cancelled: true };
    }
    const fileName = data?.configId || "recommended.cfg";
    if (!data?.canonicalJson || !applyDraft(data, fileName, null)) {
      return { ok: false, error: "推荐配置无法载入现有配置草稿。" };
    }
    showMessage(`已载入推荐配置 ${fileName}；仓库中的原配置文件未被修改。`);
    return { ok: true };
  };

  window.EvoFlowConfig = Object.freeze({
    acceptRecommendedDraft
  });

  const bindCatalogSearch = (inputId, clearId) => {
    byId(inputId)?.addEventListener("input", (event) => {
      state.catalogQuery = event.target.value;
      renderCatalog();
    });
    byId(clearId)?.addEventListener("click", () => {
      state.catalogQuery = "";
      renderCatalog();
      byId(inputId)?.focus();
    });
  };
  bindCatalogSearch("component-catalog-search", "component-catalog-search-clear");
  bindCatalogSearch("flow-catalog-search", "flow-catalog-search-clear");

  byId("config-new").addEventListener("click", newDraft);
  byId("config-open").addEventListener("click", openDraft);
  byId("config-save").addEventListener("click", save);
  byId("config-save-as").addEventListener("click", () => saveAs(false));
  byId("config-deep-validate").addEventListener("click", () => runValidation(true));
  byId("config-run").addEventListener("click", runCurrentConfig);
  byId("config-run-start").addEventListener("input", syncReplacementControl);
  byId("config-run-previous").addEventListener("click", () => moveRunRound(-1));
  byId("config-run-next").addEventListener("click", () => moveRunRound(1));
  byId("config-view-form").addEventListener("click", () => setEditorView("form"));
  byId("config-view-flow").addEventListener("click", () => setEditorView("flow"));
  byId("config-version").addEventListener("change", (event) => {
    state.saveVersion = event.target.value;
    state.dirty = true;
  });
  byId("config-file-input").addEventListener("change", async (event) => {
    await loadFile(event.target.files[0]);
    event.target.value = "";
  });
  window.addEventListener("evoflow:page", (event) => {
    if (event.detail.page === "config") initialize();
    else flowController?.exitFocusMode();
  });
  window.addEventListener("evoflow:language", () => {
    if (!state.initialized) return;
    renderCatalog();
    renderOutline();
    renderValidation();
    if (state.viewMode === "flow") flowController?.render();
    else renderForm();
  });
  Ui.setPageGuard?.((from, to) => {
    if (from !== "config" || to === "config") return true;
    if (flowController?.confirmLeave() === false) return false;
    flowController?.exitFocusMode();
    return true;
  });

  initialize();
})();
