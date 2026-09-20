(() => {
  "use strict";

  const Ui = window.EvoFlowUi;
  if (!Ui) return;

  const SVG_NS = "http://www.w3.org/2000/svg";
  const START_ID = "__start__";
  const WORKFLOW_CATEGORIES = [
    "topology", "repair", "strategy", "generator", "evaluator", "selector"
  ];
  const CATEGORY_LABELS = {
    topology: ["学习拓扑", "Learning topology"],
    repair: ["修复器", "Repair"],
    strategy: ["学习策略", "Learning strategy"],
    generator: ["子代生成", "Offspring generation"],
    evaluator: ["评价器", "Evaluator"],
    selector: ["环境选择", "Environmental selection"]
  };
  const BLOCK_WIDTH = 176;
  const BLOCK_HEIGHT = 98;
  const MIN_WORLD_WIDTH = 920;
  const MIN_WORLD_HEIGHT = 570;
  const MARGIN = 54;
  const START_POSITION = { x: 58, y: 86 };
  const ZOOM_MIN = 0.4;
  const ZOOM_MAX = 1.8;
  const ZOOM_STEP = 0.1;

  const byId = (id) => document.getElementById(id);
  const deepCopy = (value) => JSON.parse(JSON.stringify(value));
  const clamp = (value, low, high) => Math.max(low, Math.min(high, value));
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
  const schemaBody = (values) => (values || []).map((value) =>
    value === null || Number.isNaN(value) ? "null" : String(value)).join(",");

  const svgNode = (name, attributes = {}) => {
    const node = document.createElementNS(SVG_NS, name);
    Object.entries(attributes).forEach(([key, value]) =>
      node.setAttribute(key, String(value)));
    return node;
  };

  const appendText = (parent, className, x, y, value) => {
    const text = svgNode("text", { class: className, x, y });
    text.textContent = value;
    parent.appendChild(text);
    return text;
  };

  const wirePath = (x1, y1, x2, y2) => {
    const direction = x2 >= x1 ? 1 : -1;
    const bend = clamp(Math.abs(x2 - x1) * 0.45 + 48, 66, 178);
    return `M ${x1} ${y1} C ${x1 + direction * bend} ${y1}, `
      + `${x2 - direction * bend} ${y2}, ${x2} ${y2}`;
  };

  let nextNodeId = 1;
  const makeNodeId = () => `flow-node-${nextNodeId++}`;

  const create = (adapter) => {
    const canvas = byId("flow-canvas");
    const shell = byId("flow-canvas-shell");
    const inspector = byId("flow-inspector");
    const message = byId("flow-message");
    const catalog = byId("component-catalog-tree");
    const floatingCatalog = byId("flow-catalog-tree");
    const floatingPalette = byId("flow-catalog-float");
    if (!canvas || !shell || !inspector || !catalog
        || !floatingCatalog || !floatingPalette) return null;

    const workspaces = new WeakMap();
    let workspace = null;
    let workflow = null;
    let pointerAction = null;
    let renderToken = 0;
    let commitSequence = 0;
    let busy = false;
    let focusMode = false;
    let parameterPanelOpen = false;

    const showMessage = (text, error = false, kind = "") => {
      message.hidden = !text;
      message.className = `flow-message${error ? " error" : ""}`;
      message.textContent = text || "";
      message.dataset.kind = kind;
    };

    const initialPosition = (index) => ({
      x: 296 + (index % 3) * 226,
      y: 74 + Math.floor(index / 3) * 154
    });

    const snapshot = (value) => ({
      nodes: value.nodes.map((node) => ({
        id: node.id,
        component: deepCopy(node.component),
        x: node.x,
        y: node.y
      })),
      edges: [...value.edges.entries()],
      committedOrder: [...value.committedOrder],
      selectedId: value.selectedId,
      zoom: value.zoom
    });

    const restoreSnapshot = (value, saved) => {
      value.nodes = saved.nodes.map((node) => ({
        ...node, component: deepCopy(node.component)
      }));
      value.edges = new Map(saved.edges);
      value.committedOrder = [...saved.committedOrder];
      value.selectedId = saved.selectedId;
      value.zoom = saved.zoom;
      value.transient = false;
    };

    const createWorkspace = (currentWorkflow) => {
      const nodes = (currentWorkflow.components || []).map((component, index) => ({
        id: makeNodeId(),
        component: deepCopy(component),
        ...initialPosition(index)
      }));
      const edges = new Map();
      if (nodes.length) {
        edges.set(START_ID, nodes[0].id);
        nodes.slice(0, -1).forEach((node, index) =>
          edges.set(node.id, nodes[index + 1].id));
      }
      const value = {
        nodes,
        edges,
        selectedId: nodes[0]?.id || null,
        committedOrder: nodes.map((node) => node.id),
        committedSignature: JSON.stringify(currentWorkflow.components || []),
        zoom: 1,
        transient: false,
        lastValid: null
      };
      value.lastValid = snapshot(value);
      return value;
    };

    const ensureWorkspace = () => {
      const current = adapter.getWorkflow();
      if (!current) {
        workflow = null;
        workspace = null;
        return null;
      }
      let value = workspaces.get(current);
      const signature = JSON.stringify(current.components || []);
      if (!value || value.committedSignature !== signature) {
        value = createWorkspace(current);
        workspaces.set(current, value);
      }
      workflow = current;
      workspace = value;
      return value;
    };

    const nodeById = (id) => workspace?.nodes.find((node) => node.id === id) || null;

    const analyzeGraph = () => {
      if (!workspace) return {
        valid: false, order: [], reached: new Set(), indegree: new Map()
      };
      const nodeIds = new Set(workspace.nodes.map((node) => node.id));
      const indegree = new Map(workspace.nodes.map((node) => [node.id, 0]));
      let malformed = false;
      for (const [source, target] of workspace.edges) {
        if ((source !== START_ID && !nodeIds.has(source))
            || !nodeIds.has(target) || source === target) {
          malformed = true;
          continue;
        }
        indegree.set(target, (indegree.get(target) || 0) + 1);
      }
      const order = [];
      const reached = new Set();
      let current = workspace.edges.get(START_ID);
      while (current && nodeIds.has(current) && !reached.has(current)) {
        reached.add(current);
        order.push(current);
        current = workspace.edges.get(current);
      }
      if (current) malformed = true;
      const valid = workspace.nodes.length === 0
        ? workspace.edges.size === 0
        : !malformed
          && order.length === workspace.nodes.length
          && workspace.edges.size === workspace.nodes.length
          && workspace.nodes.every((node) => indegree.get(node.id) === 1)
          && !workspace.edges.has(order[order.length - 1]);
      return { valid, order, reached, indegree };
    };

    const isTransient = () => {
      const value = ensureWorkspace();
      return Boolean(value && (value.transient || !analyzeGraph().valid));
    };

    const updateLastValid = () => {
      if (!workspace || workspace.transient || !analyzeGraph().valid) return;
      workspace.lastValid = snapshot(workspace);
    };

    const worldSize = () => {
      if (!workspace?.nodes.length)
        return { width: MIN_WORLD_WIDTH, height: MIN_WORLD_HEIGHT };
      return {
        width: Math.max(MIN_WORLD_WIDTH,
          ...workspace.nodes.map((node) => node.x + BLOCK_WIDTH + MARGIN)),
        height: Math.max(MIN_WORLD_HEIGHT,
          ...workspace.nodes.map((node) => node.y + BLOCK_HEIGHT + MARGIN))
      };
    };

    const applyZoom = () => {
      if (!workspace) return;
      const size = worldSize();
      canvas.setAttribute("viewBox", `0 0 ${size.width} ${size.height}`);
      canvas.setAttribute("width", Math.round(size.width * workspace.zoom));
      canvas.setAttribute("height", Math.round(size.height * workspace.zoom));
      const label = byId("flow-zoom-reset");
      if (label) label.textContent = `${Math.round(workspace.zoom * 100)}%`;
    };

    const fitToView = () => {
      if (!ensureWorkspace()) return;
      const size = worldSize();
      const width = Math.max(1, shell.clientWidth - 24);
      const height = Math.max(1, shell.clientHeight - 24);
      const fitted = Math.floor(Math.min(
        width / size.width, height / size.height
      ) * 20) / 20;
      workspace.zoom = clamp(fitted, ZOOM_MIN, ZOOM_MAX);
      applyZoom();
      shell.scrollLeft = 0;
      shell.scrollTop = 0;
    };

    const updateFocusButton = () => {
      const button = byId("flow-focus-mode");
      if (!button) return;
      button.setAttribute("aria-pressed", String(focusMode));
      button.textContent = focusMode
        ? t("退出全屏", "Exit full screen")
        : t("画布全屏", "Full-screen canvas");
      button.title = button.textContent;
    };

    const updateFloatingPalette = () => {
      const toggle = byId("flow-catalog-toggle");
      const collapsed = floatingPalette.classList.contains("collapsed");
      floatingPalette.setAttribute("aria-hidden", String(!focusMode));
      if (!toggle) return;
      toggle.textContent = collapsed ? "+" : "−";
      toggle.setAttribute("aria-expanded", String(!collapsed));
      toggle.setAttribute("aria-label", collapsed
        ? t("展开算子库", "Expand operator library")
        : t("收起算子库", "Collapse operator library"));
      toggle.title = toggle.getAttribute("aria-label");
    };

    const updateParameterPanel = () => {
      const editor = byId("config-flow");
      if (!editor) return;
      editor.classList.toggle(
        "focus-parameters-open",
        focusMode && parameterPanelOpen
      );
      inspector.setAttribute(
        "aria-hidden",
        String(focusMode && !parameterPanelOpen)
      );
    };

    const setFocusMode = (active) => {
      const editor = byId("config-flow");
      if (!editor) return;
      focusMode = Boolean(active);
      if (focusMode) floatingPalette.classList.remove("collapsed");
      else parameterPanelOpen = false;
      editor.classList.toggle("flow-focus-mode", focusMode);
      document.body.classList.toggle("flow-focus-open", focusMode);
      updateFocusButton();
      updateFloatingPalette();
      updateParameterPanel();
      window.requestAnimationFrame(() => {
        if (focusMode) fitToView();
        else applyZoom();
      });
    };

    const canvasPoint = (event) => {
      const point = canvas.createSVGPoint();
      point.x = event.clientX;
      point.y = event.clientY;
      const matrix = canvas.getScreenCTM();
      if (!matrix) return { x: 0, y: 0 };
      const result = point.matrixTransform(matrix.inverse());
      return { x: result.x, y: result.y };
    };

    const portPoint = (id, output) => {
      if (id === START_ID) {
        return {
          x: START_POSITION.x + BLOCK_WIDTH,
          y: START_POSITION.y + BLOCK_HEIGHT / 2
        };
      }
      const node = nodeById(id);
      if (!node) return { x: 0, y: 0 };
      return {
        x: node.x + (output ? BLOCK_WIDTH : 0),
        y: node.y + BLOCK_HEIGHT / 2
      };
    };

    const committedIndex = (id) => workspace?.committedOrder.indexOf(id) ?? -1;

    const issueSeverityRank = (severity) =>
      severity === "error" ? 3 : severity === "warning" ? 2 : severity === "info" ? 1 : 0;

    const stateFor = (id, analysis = analyzeGraph()) => {
      const index = committedIndex(id);
      if (!analysis.reached.has(id) || (analysis.indegree.get(id) || 0) !== 1)
        return "isolated";
      if (index < 0) return "isolated";
      const workflowIndex = adapter.getWorkflowIndex();
      const issues = (adapter.getIssues() || []).filter((issue) =>
        issue.scopeType === "workflow"
        && issue.scopeIndex === workflowIndex
        && issue.componentIndex === index);
      if (issues.some((issue) =>
        issue.severity === "error" && issue.reason === "insufficient_number"))
        return "failsatisfied";
      if (issues.some((issue) => issue.severity === "error"))
        return "unsatisfied";
      if (issues.some((issue) => issue.severity === "warning"))
        return "notfullysatisfied";
      if (issues.some((issue) => issue.severity === "info"))
        return "informational";
      return "satisfied";
    };

    const stateIssue = (id) => {
      const index = committedIndex(id);
      if (index < 0) return null;
      const workflowIndex = adapter.getWorkflowIndex();
      const issues = (adapter.getIssues() || []).filter((issue) =>
        issue.scopeType === "workflow"
        && issue.scopeIndex === workflowIndex
        && issue.componentIndex === index);
      issues.sort((a, b) => issueSeverityRank(b.severity) - issueSeverityRank(a.severity));
      return issues[0] || null;
    };

    const defineMarkers = () => {
      const defs = svgNode("defs");
      const marker = svgNode("marker", {
        id: "flow-arrow",
        viewBox: "0 0 10 10",
        refX: 8,
        refY: 5,
        markerWidth: 7,
        markerHeight: 7,
        orient: "auto-start-reverse",
        markerUnits: "strokeWidth"
      });
      marker.appendChild(svgNode("path", {
        d: "M 0 0 L 10 5 L 0 10 z",
        fill: "context-stroke"
      }));
      defs.appendChild(marker);
      canvas.appendChild(defs);
    };

    const drawWire = (source, target, state) => {
      const from = portPoint(source, true);
      const to = portPoint(target, false);
      to.x -= 11;
      const path = wirePath(from.x, from.y, to.x, to.y);
      const hit = svgNode("path", {
        class: "flow-wire-hit",
        d: path,
        "data-edge-source": source,
        "data-edge-target": target,
        role: "button",
        tabindex: "0",
        "aria-label": t("删除连接", "Delete connection")
      });
      canvas.appendChild(hit);
      canvas.appendChild(svgNode("path", {
        class: `flow-wire ${state}`,
        d: path,
        "marker-end": "url(#flow-arrow)"
      }));
    };

    const drawStart = () => {
      const x = START_POSITION.x;
      const y = START_POSITION.y;
      const group = svgNode("g", {
        class: "flow-block flow-start",
        transform: `translate(${x} ${y})`
      });
      group.appendChild(svgNode("rect", {
        class: "block-body", width: BLOCK_WIDTH, height: BLOCK_HEIGHT, rx: 13
      }));
      group.appendChild(svgNode("path", {
        class: "block-band",
        d: `M 13 0 H ${BLOCK_WIDTH - 13} Q ${BLOCK_WIDTH} 0 ${BLOCK_WIDTH} 13 `
          + `V 29 H 0 V 13 Q 0 0 13 0 Z`
      }));
      appendText(group, "block-index", 12, 19, "START");
      appendText(group, "block-category", 57, 19, t("初始化器", "Initializer"));
      appendText(group, "block-tag", 14, 54, workflow?.initializer?.tag || "Random");
      appendText(group, "block-contract", 14, 78,
        t("固定起点 · 参数在表单中编辑", "Fixed start · edit parameters in the form"));
      group.appendChild(svgNode("circle", {
        class: "flow-port", cx: BLOCK_WIDTH,
        cy: BLOCK_HEIGHT / 2, r: 7,
        "data-port": "out", "data-node-id": START_ID
      }));
      canvas.appendChild(group);
    };

    const drawBlock = (node, analysis) => {
      const state = stateFor(node.id, analysis);
      const graphIndex = analysis.order.indexOf(node.id);
      const displayIndex = graphIndex >= 0 ? graphIndex + 1 : committedIndex(node.id) + 1;
      const group = svgNode("g", {
        class: `flow-block state-${state}${workspace.selectedId === node.id ? " selected" : ""}`,
        transform: `translate(${node.x} ${node.y})`,
        "data-node-id": node.id,
        tabindex: "0",
        role: "button",
        "aria-label": `${displayIndex > 0 ? displayIndex : "?"} `
          + `${node.component.type} ${node.component.tag}`
      });
      const title = svgNode("title");
      title.textContent = t(
        "全屏时双击打开参数设置",
        "Double-click in full-screen mode to edit parameters"
      );
      group.appendChild(title);
      group.appendChild(svgNode("rect", {
        class: "block-body", width: BLOCK_WIDTH, height: BLOCK_HEIGHT, rx: 13
      }));
      group.appendChild(svgNode("path", {
        class: "block-band",
        d: `M 13 0 H ${BLOCK_WIDTH - 13} Q ${BLOCK_WIDTH} 0 ${BLOCK_WIDTH} 13 `
          + `V 29 H 0 V 13 Q 0 0 13 0 Z`
      }));
      appendText(group, "block-index", 12, 19,
        displayIndex > 0 ? String(displayIndex).padStart(2, "0") : "—");
      appendText(group, "block-category", 39, 19, categoryLabel(node.component.type));
      appendText(group, "block-tag", 14, 54, node.component.tag);
      const contract = state === "satisfied"
        ? t("连接状态 · 满足", "Connection · satisfied")
        : state === "informational"
          ? t("连接状态 · 信息", "Connection · info")
        : state === "notfullysatisfied"
          ? t("连接状态 · 警告", "Connection · warning")
          : state === "failsatisfied"
            ? t("连接状态 · 数量不足", "Connection · insufficient")
            : state === "isolated"
              ? t("连接状态 · 临时孤立", "Connection · temporarily isolated")
              : t("连接状态 · 不满足", "Connection · unsatisfied");
      appendText(group, "block-contract", 14, 78, contract);
      group.appendChild(svgNode("circle", {
        class: "flow-port", cx: 0, cy: BLOCK_HEIGHT / 2, r: 7,
        "data-port": "in", "data-node-id": node.id
      }));
      group.appendChild(svgNode("circle", {
        class: "flow-port", cx: BLOCK_WIDTH, cy: BLOCK_HEIGHT / 2, r: 7,
        "data-port": "out", "data-node-id": node.id
      }));
      canvas.appendChild(group);
    };

    const renderCanvas = () => {
      if (!workspace || !workflow) return;
      canvas.replaceChildren();
      defineMarkers();
      applyZoom();
      const analysis = analyzeGraph();
      for (const [source, target] of workspace.edges)
        drawWire(source, target, stateFor(target, analysis));
      drawStart();
      workspace.nodes.forEach((node) => drawBlock(node, analysis));
      if (!workspace.nodes.length)
        appendText(canvas, "flow-empty-label", 300, 136,
          t("从左侧组件目录拖入第一块积木", "Drop the first block from the component catalog"));
      if (pointerAction?.kind === "connect") {
        const start = portPoint(pointerAction.source, true);
        canvas.appendChild(svgNode("path", {
          id: "flow-wire-preview",
          class: "flow-wire preview",
          d: wirePath(start.x, start.y, pointerAction.x, pointerAction.y),
          "marker-end": "url(#flow-arrow)"
        }));
      }
    };

    const fetchContract = (component) => Ui.api(
      `/api/config/contract?category=${encodeURIComponent(component.type)}`
      + `&tag=${encodeURIComponent(component.tag)}`,
      {
        method: "POST",
        headers: { "Content-Type": "text/plain" },
        body: schemaBody(component.para)
      }
    );

    const contractGroup = (title, items) => `
      <div class="flow-contract-group">
        <strong>${title}</strong>
        <div class="flow-contract-list">
          ${items.length ? items.map((item) =>
            `<span class="flow-contract-chip">${escapeHtml(item.detail)}</span>`
          ).join("") : `<span class="flow-contract-empty">${t("无", "None")}</span>`}
        </div>
      </div>`;

    const tryCommit = async (actionLabel, focusId = workspace?.selectedId) => {
      if (!workspace || busy) return false;
      const analysis = analyzeGraph();
      workspace.transient = true;
      if (!analysis.valid) {
        showMessage(t(
          "画布可暂时保留孤立或断开的组件；连接为覆盖全部组件的单一有向流后会自动保存。",
          "Disconnected or isolated blocks may remain temporarily. A single directed path covering every block is saved automatically."
        ));
        renderCanvas();
        return false;
      }

      const components = analysis.order.map((id) => deepCopy(nodeById(id).component));
      const workflowIndex = adapter.getWorkflowIndex();
      const draft = deepCopy(adapter.getDraft());
      if (!draft?.workflows?.[workflowIndex]) return false;
      draft.workflows[workflowIndex].components = deepCopy(components);
      const focusIndex = Math.max(0, analysis.order.indexOf(focusId));
      const sequence = ++commitSequence;
      busy = true;
      showMessage(t("正在检查完整有向流…", "Checking the complete directed path…"));
      let result;
      try {
        result = await Ui.api(
          `/api/config/flow/check?workflow=${workflowIndex}&component=${focusIndex}`,
          {
            method: "POST",
            headers: { "Content-Type": "text/plain" },
            body: JSON.stringify(draft)
          }
        );
      } catch (error) {
        if (sequence !== commitSequence) return false;
        busy = false;
        showMessage(error?.message
          || t("连接检查失败。", "Connection check failed."), true);
        renderCanvas();
        return false;
      }
      if (sequence !== commitSequence) {
        busy = false;
        return false;
      }
      busy = false;
      if (!result.ok) {
        showMessage(result.error || t("连接检查失败。", "Connection check failed."), true);
        renderCanvas();
        return false;
      }
      if (!result.allowed) {
        showMessage(`${actionLabel}${t("被拒绝：", " rejected: ")}`
          + `${result.message || result.reason || result.state}`, true);
        renderCanvas();
        return false;
      }

      workspace.committedOrder = [...analysis.order];
      workspace.committedSignature = JSON.stringify(components);
      workspace.transient = false;
      adapter.commitComponents(components, focusIndex);
      workspace.lastValid = snapshot(workspace);
      showMessage(result.state === "notfullysatisfied"
        ? `${actionLabel}${t("已应用，但存在警告：", " applied with a warning: ")}`
          + `${result.message || result.reason}`
        : result.state === "informational"
          ? `${actionLabel}${t("已应用，同时存在信息：", " applied with info: ")}`
            + `${result.message || result.reason}`
          : `${actionLabel}${t("已应用。", " applied.")}`);
      render();
      return true;
    };

    const removeNode = async (id) => {
      if (!workspace) return;
      const node = nodeById(id);
      if (!node) return;
      const predecessor = [...workspace.edges.entries()]
        .find(([, target]) => target === id)?.[0];
      const successor = workspace.edges.get(id);
      if (predecessor) workspace.edges.delete(predecessor);
      workspace.edges.delete(id);
      for (const [source, target] of [...workspace.edges])
        if (target === id) workspace.edges.delete(source);
      if (predecessor && successor && predecessor !== successor)
        workspace.edges.set(predecessor, successor);
      workspace.nodes = workspace.nodes.filter((item) => item.id !== id);
      workspace.selectedId = workspace.nodes[0]?.id || null;
      if (!workspace.selectedId) {
        parameterPanelOpen = false;
        updateParameterPanel();
      }
      workspace.transient = true;
      await tryCommit(`${t("移除", "Remove")} ${node.component.tag}`);
      render();
    };

    const renderInspector = async (token) => {
      const node = nodeById(workspace?.selectedId);
      if (!workspace || !node) {
        inspector.innerHTML = `<div class="empty-state compact">${t(
          "选择积木后在这里检查连接关系并编辑参数。",
          "Select a block to inspect connections and edit parameters."
        )}</div>`;
        return;
      }
      const index = committedIndex(node.id);
      const tags = adapter.componentsFor(node.component.type);
      const issue = stateIssue(node.id);
      const isolated = stateFor(node.id) === "isolated";
      inspector.innerHTML = `
        <div class="flow-inspector-heading">
          <div><span>${index >= 0 ? `Component ${String(index + 1).padStart(2, "0")}`
            : t("临时组件", "Temporary block")}</span>
            <h3>${escapeHtml(node.component.tag)}</h3></div>
          <div class="flow-inspector-actions">
            ${focusMode ? `<button class="icon-button" data-flow-close-parameters
                    type="button" title="${t(
                      "关闭参数设置", "Close parameter settings"
                    )}">×</button>` : ""}
            <button class="icon-button danger" data-flow-delete type="button"
                    title="${t("删除", "Delete")}">${focusMode ? "⌫" : "×"}</button>
          </div>
        </div>
        <label class="field">${t("类别", "Category")}
          <select id="flow-component-category">${WORKFLOW_CATEGORIES.map((category) =>
            `<option value="${category}"${category === node.component.type ? " selected" : ""}>`
            + `${categoryLabel(category)}</option>`
          ).join("")}</select>
        </label>
        <label class="field">${t("组件", "Component")}
          <select id="flow-component-tag">${tags.map((item) =>
            `<option value="${escapeHtml(item.tag)}"${item.tag === node.component.tag ? " selected" : ""}>`
            + `${escapeHtml(item.tag)}</option>`
          ).join("")}</select>
        </label>
        <div id="flow-contracts" class="flow-contracts">
          <p class="muted">${t(
            "正在读取当前参数下的需求与供给…",
            "Reading requirements and provisions for the current parameters…"
          )}</p>
        </div>
        <div class="flow-state-note ${isolated ? "isolated" : stateFor(node.id)}">
          ${isolated
            ? t("此组件尚未接入起点开始的单一有向流，可继续拖动或连接。",
              "This block is not yet in the directed path from Start. You may keep arranging or connecting it.")
            : issue ? escapeHtml(issue.message)
              : t("当前积木连接正常。", "This block is connected correctly.")}
        </div>
        <div id="flow-params" class="parameter-host"></div>
        <button id="flow-open-form" class="toolbar-button" type="button">
          ${t("在表单中定位", "Locate in form")}
        </button>`;

      inspector.querySelector("[data-flow-delete]")
        ?.addEventListener("click", () => removeNode(node.id));
      inspector.querySelector("[data-flow-close-parameters]")
        ?.addEventListener("click", () => {
          parameterPanelOpen = false;
          updateParameterPanel();
        });
      byId("flow-open-form")?.addEventListener("click", () => {
        if (!confirmLeave()) return;
        const restoredIndex = committedIndex(workspace.selectedId);
        adapter.openFormAt(Math.max(0, restoredIndex));
      });
      byId("flow-component-category")?.addEventListener("change", async (event) => {
        const category = event.target.value;
        const replacement = adapter.componentsFor(category)[0];
        if (!replacement) return;
        node.component = { type: category, tag: replacement.tag, para: [] };
        workspace.transient = true;
        await tryCommit(t("切换积木类别", "Change block category"), node.id);
      });
      byId("flow-component-tag")?.addEventListener("change", async (event) => {
        node.component = {
          type: node.component.type,
          tag: event.target.value,
          para: []
        };
        workspace.transient = true;
        await tryCommit(t("切换组件", "Change component"), node.id);
      });

      adapter.mountParameters(
        "flow-params",
        node.component,
        async () => {
          workspace.transient = true;
          await tryCommit(t("更新参数", "Update parameters"), node.id);
        },
        () => render()
      );
      const contract = await fetchContract(node.component);
      if (token !== renderToken || workspace?.selectedId !== node.id) return;
      const host = byId("flow-contracts");
      if (!host) return;
      host.innerHTML = contract.ok
        ? contractGroup(t("需求", "Requirements"), contract.requirements || [])
          + contractGroup(t("供给", "Provisions"), contract.provisions || [])
        : `<p class="small-danger">${escapeHtml(
          contract.error || t("无法检查连接关系", "Unable to inspect connections")
        )}</p>`;
    };

    const render = () => {
      const token = ++renderToken;
      if (!ensureWorkspace()) {
        canvas.replaceChildren();
        showMessage(t(
          "请选择一个 workflow 进入积木视图。",
          "Select a workflow to open the canvas."
        ), false, "context");
        inspector.innerHTML = `<div class="empty-state compact">${t(
          "当前没有选中的 workflow。",
          "No workflow is selected."
        )}</div>`;
        return;
      }
      if (message.dataset.kind === "context") showMessage("");
      byId("flow-title").textContent =
        `${t("积木式 Workflow", "Workflow canvas")} · ${workflow.tag}`;
      renderCanvas();
      renderInspector(token);
    };

    const clearPointerAction = () => {
      pointerAction = null;
      window.removeEventListener("pointermove", onPointerMove);
      window.removeEventListener("pointerup", onPointerUp);
      window.removeEventListener("pointercancel", onPointerUp);
    };

    const onPointerMove = (event) => {
      if (!pointerAction || !workspace) return;
      const point = canvasPoint(event);
      if (pointerAction.kind === "move") {
        const node = nodeById(pointerAction.id);
        if (!node) return;
        node.x = Math.max(MARGIN, point.x - pointerAction.offsetX);
        node.y = Math.max(MARGIN, point.y - pointerAction.offsetY);
      } else {
        pointerAction.x = point.x;
        pointerAction.y = point.y;
      }
      renderCanvas();
    };

    const onPointerUp = async (event) => {
      if (!pointerAction || !workspace) return;
      const action = pointerAction;
      if (action.kind === "move") {
        clearPointerAction();
        updateLastValid();
        render();
        return;
      }

      const targetPort = document.elementFromPoint(event.clientX, event.clientY)
        ?.closest?.('[data-port="in"]');
      clearPointerAction();
      if (!targetPort) {
        workspace.transient = true;
        showMessage(t(
          "连线已释放；原输出保持断开，可继续连接或在离开时恢复最近有效单链。",
          "The connection was released. Its output remains disconnected; reconnect it or restore the latest valid path when leaving."
        ));
        render();
        return;
      }
      const target = targetPort.dataset.nodeId;
      if (!target || target === action.source) {
        showMessage(t("不能将组件连接到自身。", "A block cannot connect to itself."), true);
        render();
        return;
      }
      for (const [source, value] of [...workspace.edges])
        if (value === target) workspace.edges.delete(source);
      workspace.edges.set(action.source, target);
      workspace.transient = true;
      await tryCommit(t("卡扣连接", "Connector link"), target);
      render();
    };

    canvas.addEventListener("pointerdown", (event) => {
      if (busy || event.button !== 0 || !ensureWorkspace()) return;
      const output = event.target.closest?.('[data-port="out"]');
      if (output) {
        event.preventDefault();
        const source = output.dataset.nodeId;
        const start = portPoint(source, true);
        workspace.edges.delete(source);
        workspace.transient = true;
        pointerAction = {
          kind: "connect", source, x: start.x + 1, y: start.y
        };
        window.addEventListener("pointermove", onPointerMove);
        window.addEventListener("pointerup", onPointerUp);
        window.addEventListener("pointercancel", onPointerUp);
        renderCanvas();
        return;
      }

      const block = event.target.closest?.(".flow-block[data-node-id]");
      if (!block) return;
      event.preventDefault();
      const id = block.dataset.nodeId;
      const node = nodeById(id);
      if (!node) return;
      workspace.selectedId = id;
      const point = canvasPoint(event);
      pointerAction = {
        kind: "move",
        id,
        offsetX: point.x - node.x,
        offsetY: point.y - node.y
      };
      window.addEventListener("pointermove", onPointerMove);
      window.addEventListener("pointerup", onPointerUp);
      window.addEventListener("pointercancel", onPointerUp);
      render();
    });

    canvas.addEventListener("click", (event) => {
      if (!ensureWorkspace()) return;
      const edge = event.target.closest?.("[data-edge-source]");
      if (edge) {
        workspace.edges.delete(edge.dataset.edgeSource);
        workspace.transient = true;
        showMessage(t(
          "连接已删除；临时断开的画布尚未写回配置。",
          "Connection removed. The temporarily disconnected canvas has not been committed."
        ));
        render();
        return;
      }
      const block = event.target.closest?.(".flow-block[data-node-id]");
      if (!block || event.target.closest?.(".flow-port")) return;
      workspace.selectedId = block.dataset.nodeId;
      render();
    });

    canvas.addEventListener("dblclick", (event) => {
      if (!focusMode || !ensureWorkspace()) return;
      const block = event.target.closest?.(".flow-block[data-node-id]");
      if (!block || event.target.closest?.(".flow-port")) return;
      event.preventDefault();
      workspace.selectedId = block.dataset.nodeId;
      parameterPanelOpen = true;
      updateParameterPanel();
      render();
    });

    canvas.addEventListener("keydown", (event) => {
      const edge = event.target.closest?.("[data-edge-source]");
      if (edge && (event.key === "Delete" || event.key === "Backspace"
          || event.key === "Enter" || event.key === " ")) {
        event.preventDefault();
        workspace.edges.delete(edge.dataset.edgeSource);
        workspace.transient = true;
        render();
        return;
      }
      const block = event.target.closest?.(".flow-block[data-node-id]");
      if (!block) return;
      workspace.selectedId = block.dataset.nodeId;
      if (event.key === "Delete" || event.key === "Backspace") {
        event.preventDefault();
        removeNode(workspace.selectedId);
      } else if (event.key === "Enter" || event.key === " ") {
        event.preventDefault();
        if (focusMode) {
          parameterPanelOpen = true;
          updateParameterPanel();
        }
        render();
      }
    });

    const addCatalogComponent = async (category, tag, point, append = false) => {
      if (!WORKFLOW_CATEGORIES.includes(category) || !tag || !ensureWorkspace()) {
        showMessage(t(
          "请先在配置结构中选择一个 workflow。",
          "Select a workflow in the configuration structure first."
        ), true);
        return null;
      }
      const node = {
        id: makeNodeId(),
        component: { type: category, tag, para: [] },
        x: Math.max(MARGIN, point?.x ?? initialPosition(workspace.nodes.length).x),
        y: Math.max(MARGIN, point?.y ?? initialPosition(workspace.nodes.length).y)
      };
      const before = analyzeGraph();
      workspace.nodes.push(node);
      workspace.selectedId = node.id;
      workspace.transient = true;
      if (append && before.valid) {
        workspace.edges.set(
          before.order[before.order.length - 1] || START_ID,
          node.id
        );
        await tryCommit(`${t("添加", "Add")} ${tag}`, node.id);
      } else {
        showMessage(t(
          `已添加孤立组件 ${tag}；请用有向箭头将其接入单一流。`,
          `Added isolated block ${tag}. Connect it into the single path with directed arrows.`
        ));
      }
      render();
      return node.id;
    };

    shell.addEventListener("dragover", (event) => {
      if (!Array.from(event.dataTransfer.types || [])
        .includes("application/x-evoflow-component")) return;
      event.preventDefault();
      event.dataTransfer.dropEffect = "copy";
      shell.classList.add("dragging");
    });
    shell.addEventListener("dragleave", (event) => {
      if (!shell.contains(event.relatedTarget)) shell.classList.remove("dragging");
    });
    shell.addEventListener("drop", async (event) => {
      event.preventDefault();
      shell.classList.remove("dragging");
      const raw = event.dataTransfer.getData("application/x-evoflow-component");
      if (!raw) return;
      let item;
      try { item = JSON.parse(raw); } catch { return; }
      const point = canvasPoint(event);
      await addCatalogComponent(item.category, item.tag, {
        x: point.x - BLOCK_WIDTH / 2,
        y: point.y - BLOCK_HEIGHT / 2
      });
    });

    const bindCatalogInteractions = (host) => {
      host.addEventListener("dragstart", (event) => {
        const item = event.target.closest?.("[data-flow-category][data-flow-tag]");
        if (!item) return;
        event.dataTransfer.effectAllowed = "copy";
        event.dataTransfer.setData(
          "application/x-evoflow-component",
          JSON.stringify({
            category: item.dataset.flowCategory,
            tag: item.dataset.flowTag
          })
        );
      });
      host.addEventListener("dblclick", async (event) => {
        const item = event.target.closest?.("[data-flow-category][data-flow-tag]");
        if (!item || byId("config-flow").hidden) return;
        const added = await addCatalogComponent(
          item.dataset.flowCategory,
          item.dataset.flowTag,
          initialPosition(workspace?.nodes.length || 0),
          true
        );
        if (focusMode && added) {
          parameterPanelOpen = true;
          updateParameterPanel();
          render();
        }
      });
    };
    [catalog, floatingCatalog].forEach(bindCatalogInteractions);

    const setZoom = (value) => {
      if (!ensureWorkspace()) return;
      workspace.zoom = clamp(Math.round(value * 10) / 10, ZOOM_MIN, ZOOM_MAX);
      applyZoom();
    };
    byId("flow-zoom-out")?.addEventListener("click", () =>
      setZoom((workspace?.zoom || 1) - ZOOM_STEP));
    byId("flow-zoom-in")?.addEventListener("click", () =>
      setZoom((workspace?.zoom || 1) + ZOOM_STEP));
    byId("flow-zoom-reset")?.addEventListener("click", () => setZoom(1));
    byId("flow-fit-view")?.addEventListener("click", fitToView);
    byId("flow-focus-mode")?.addEventListener("click", () =>
      setFocusMode(!focusMode));
    byId("flow-catalog-toggle")?.addEventListener("click", () => {
      floatingPalette.classList.toggle("collapsed");
      updateFloatingPalette();
    });
    byId("flow-auto-layout")?.addEventListener("click", () => {
      if (!ensureWorkspace()) return;
      const analysis = analyzeGraph();
      const order = analysis.valid ? analysis.order : [
        ...analysis.order,
        ...workspace.nodes.map((node) => node.id)
          .filter((id) => !analysis.reached.has(id))
      ];
      order.forEach((id, index) => {
        const node = nodeById(id);
        Object.assign(node, initialPosition(index));
      });
      updateLastValid();
      render();
    });

    const confirmLeave = () => {
      if (!isTransient()) return true;
      const confirmed = window.confirm(t(
        "画布中还有孤立、断开或尚未通过检查的组件。离开后将丢弃这些临时修改，并恢复历史最近的有效单一流。仍要离开吗？",
        "The canvas still contains isolated, disconnected or unvalidated blocks. Leaving discards these temporary edits and restores the latest valid single path. Leave anyway?"
      ));
      if (!confirmed) return false;
      restoreSnapshot(workspace, workspace.lastValid);
      workspace.committedSignature = JSON.stringify(
        workspace.committedOrder.map((id) => nodeById(id).component)
      );
      showMessage("");
      render();
      return true;
    };

    window.addEventListener("evoflow:language", () => {
      render();
      updateFocusButton();
      updateFloatingPalette();
    });
    document.addEventListener("keydown", (event) => {
      if (event.key !== "Escape" || !focusMode) return;
      if (parameterPanelOpen) {
        parameterPanelOpen = false;
        updateParameterPanel();
      } else {
        setFocusMode(false);
      }
    });
    window.addEventListener("beforeunload", (event) => {
      if (!isTransient()) return;
      event.preventDefault();
      event.returnValue = "";
    });

    return {
      render,
      exitFocusMode() {
        if (focusMode) setFocusMode(false);
      },
      select(index) {
        if (!ensureWorkspace()) return;
        workspace.selectedId = workspace.committedOrder[index]
          || workspace.nodes[index]?.id || null;
        render();
      },
      hasTransient: isTransient,
      confirmLeave,
      discardTransient() {
        if (!ensureWorkspace() || !isTransient()) return;
        restoreSnapshot(workspace, workspace.lastValid);
        render();
      }
    };
  };

  window.EvoFlowFlowEditor = { create };
})();
