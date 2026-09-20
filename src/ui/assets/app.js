(() => {
  "use strict";

  const MAX_FILE_BYTES = 64 * 1024 * 1024;
  const hashToken = new URLSearchParams(location.hash.slice(1)).get("token");
  if (hashToken) {
    sessionStorage.setItem("evoflow-token", hashToken);
    history.replaceState(null, "", location.pathname);
  }
  const token = sessionStorage.getItem("evoflow-token") || "";

  const byId = (id) => document.getElementById(id);
  const t = (zh, en) => window.EvoFlowI18n?.t(zh, en) || zh;
  const message = byId("message");
  const results = byId("results");
  let currentProblemAvailable = false;
  let currentProblemFileName = "problem.data";
  let currentProblemDescription = null;
  let currentPage = "problem";
  let pageGuard = null;

  const api = async (path, options = {}) => {
    const headers = new Headers(options.headers || {});
    headers.set("X-EvoFlow-Token", token);
    const response = await fetch(path, { ...options, headers });
    const data = await response.json().catch(() => ({ ok: false, error: "服务返回的数据无法识别" }));
    if (!response.ok && data.ok !== false) data.ok = false;
    return data;
  };

  const download = async (path, options = {}) => {
    const headers = new Headers(options.headers || {});
    headers.set("X-EvoFlow-Token", token);
    return fetch(path, { ...options, headers });
  };

  const pageMeta = {
    problem: {
      eyebrow: ["优化配置 / 问题定义", "Optimization / Problem definition"],
      title: ["定义、加载并检查问题数据", "Define, load and inspect problem data"]
    },
    config: {
      eyebrow: ["优化配置 / 优化器构筑", "Optimization / Optimizer builder"],
      title: ["构筑、校验并保存优化器", "Build, validate and save an optimizer"]
    },
    report: {
      eyebrow: ["运行结果 / 统计分析", "Results / Statistical analysis"],
      title: ["比较实验批次与下载图表", "Compare experiment batches and download charts"]
    }
  };

  const showPage = (page) => {
    const meta = pageMeta[page];
    if (!meta) return false;
    if (page !== currentPage && pageGuard && pageGuard(currentPage, page) === false)
      return false;
    document.querySelectorAll(".page").forEach((node) => {
      node.hidden = node.id !== `${page}-page`;
    });
    document.querySelectorAll(".nav-item[data-page]").forEach((node) => {
      node.classList.toggle("active", node.dataset.page === page);
    });
    byId("page-eyebrow").textContent = t(...meta.eyebrow);
    byId("page-title").textContent = t(...meta.title);
    currentPage = page;
    window.dispatchEvent(new CustomEvent("evoflow:page", { detail: { page } }));
    return true;
  };

  const setConnection = (state, text) => {
    byId("connection-dot").className = `connection-dot ${state}`;
    byId("connection-text").textContent = text;
  };

  const showMessage = (text, error = false) => {
    message.hidden = false;
    message.className = `message${error ? " error" : ""}`;
    message.textContent = text;
  };

  const hideMessage = () => {
    message.hidden = true;
    message.textContent = "";
  };

  const setPageGuard = (guard) => { pageGuard = guard; };
  window.EvoFlowUi = { api, download, setConnection, showPage, setPageGuard, t };
  document.querySelectorAll(".nav-item[data-page]").forEach((button) => {
    button.addEventListener("click", () => showPage(button.dataset.page));
  });

  const formatNumber = (value) => {
    if (value === null || value === undefined) return "—";
    return Number(value).toLocaleString(
      window.EvoFlowI18n?.language === "en" ? "en-US" : "zh-CN",
      { maximumSignificantDigits: 10 }
    );
  };

  const textCell = (value, className = "") => {
    const cell = document.createElement("td");
    cell.textContent = value;
    if (className) cell.className = className;
    return cell;
  };

  const tagsCell = (items, emptyText = "—") => {
    const cell = document.createElement("td");
    if (!items || items.length === 0) {
      cell.textContent = emptyText;
      cell.className = "muted";
      return cell;
    }
    const list = document.createElement("div");
    list.className = "tag-list";
    items.forEach((item) => {
      const tag = document.createElement("span");
      tag.className = "tag";
      tag.textContent = item;
      list.appendChild(tag);
    });
    cell.appendChild(list);
    return cell;
  };

  const renderVariables = (variables) => {
    const body = byId("variables-body");
    body.replaceChildren();
    variables.forEach((variable) => {
      const row = document.createElement("tr");
      row.append(
        textCell(variable.name),
        textCell(variable.kind === "decision"
          ? t("决策变量", "Decision variable") : t("常量", "Constant")),
        textCell(variable.valueType),
        textCell(formatNumber(variable.length)),
        textCell(formatNumber(variable.lowerBound), variable.lowerBound === null ? "muted" : ""),
        textCell(formatNumber(variable.upperBound), variable.upperBound === null ? "muted" : "")
      );
      body.appendChild(row);
    });
  };

  const renderObjectives = (objectives) => {
    const body = byId("objectives-body");
    body.replaceChildren();
    objectives.forEach((objective) => {
      const row = document.createElement("tr");
      row.append(
        textCell(objective.name),
        textCell(objective.minimize
          ? t("最小化", "Minimize") : t("最大化", "Maximize")),
        textCell(formatNumber(objective.priority)),
        tagsCell(objective.variables)
      );
      body.appendChild(row);
    });
  };

  const renderConstraints = (constraints) => {
    const body = byId("constraints-body");
    body.replaceChildren();
    constraints.forEach((constraint) => {
      const row = document.createElement("tr");
      row.append(
        textCell(constraint.type),
        tagsCell(constraint.variables),
        textCell(formatNumber(constraint.penalty)),
        tagsCell(constraint.objectives, t("仅可行性约束", "Feasibility only"))
      );
      body.appendChild(row);
    });
  };

  const renderDescription = (data) => {
    byId("summary-name").textContent = data.name || t("未命名", "Unnamed");
    byId("summary-type").textContent = data.type || "—";
    byId("summary-dimension").textContent = formatNumber(data.dimension);
    byId("summary-counts").textContent =
      t(
        `${data.variableCount} 变量 · ${data.objectiveCount} 目标 · ${data.constraintCount} 约束`,
        `${data.variableCount} variables · ${data.objectiveCount} objectives · `
          + `${data.constraintCount} constraints`
      );
    byId("variable-count").textContent = data.variableCount;
    byId("objective-count").textContent = data.objectiveCount;
    byId("constraint-count").textContent = data.constraintCount;
    renderVariables(data.variables || []);
    renderObjectives(data.objectives || []);
    renderConstraints(data.constraints || []);
    results.hidden = false;
  };

  const acceptDescription = (data, statusText) => {
    currentProblemDescription = data;
    renderDescription(data);
    showMessage(statusText || `已加载 ${data.fileName || "问题"}`);
    setConnection("online", "本地服务已连接");
    currentProblemAvailable = true;
    currentProblemFileName = data.fileName || "problem.data";
    window.dispatchEvent(new CustomEvent("evoflow:problem-state", {
      detail: {
        available: true,
        type: data.type || "",
        name: data.name || "",
        fileName: data.fileName || ""
      }
    }));
  };

  const saveCurrentProblemAs = async () => {
    if (!currentProblemAvailable) {
      showMessage("请先加载或构建问题。", true);
      return;
    }
    const button = byId("problem-save-as");
    button.disabled = true;
    button.textContent = "正在准备…";
    try {
      const response = await download("/api/problem/current");
      if (!response.ok) {
        const data = await response.json().catch(() => ({}));
        throw new Error(data.error || "无法读取当前问题数据");
      }
      const blob = await response.blob();
      const suggestedName = currentProblemFileName
        .replace(/[\\/:*?"<>|\u0000-\u001f]/g, "_") || "problem.data";
      if (window.showSaveFilePicker) {
        const handle = await window.showSaveFilePicker({ suggestedName });
        const writable = await handle.createWritable();
        await writable.write(blob);
        await writable.close();
        showMessage(`已另存为 ${handle.name || suggestedName}`);
      } else {
        const url = URL.createObjectURL(blob);
        const link = document.createElement("a");
        link.href = url;
        link.download = suggestedName;
        link.click();
        window.setTimeout(() => URL.revokeObjectURL(url), 1000);
        showMessage(`已下载 ${suggestedName}`);
      }
    } catch (error) {
      if (error.name !== "AbortError")
        showMessage(error.message || "另存问题失败", true);
    } finally {
      button.disabled = false;
      button.textContent = "另存问题";
    }
  };

  const loadFile = async (file, options = {}) => {
    const quiet = Boolean(options.quiet);
    if (!file) return { ok: false, errorCode: "file_required" };
    if (file.size > MAX_FILE_BYTES) {
      const error = t(
        "文件超过 64 MiB，当前版本拒绝加载。",
        "The file is larger than 64 MiB and cannot be loaded."
      );
      if (!quiet) showMessage(error, true);
      return { ok: false, errorCode: "file_too_large", error };
    }
    hideMessage();
    if (!quiet)
      showMessage(t(`正在检查 ${file.name}…`, `Inspecting ${file.name}…`));
    try {
      const data = await api(`/api/problem/describe?name=${encodeURIComponent(file.name)}`, {
        method: "POST",
        headers: { "Content-Type": "application/octet-stream" },
        body: file
      });
      if (!data.ok) {
        if (!quiet)
          showMessage(
            t(
              `${data.error || "问题文件无法解析"} 如需解析非标准数据，请补充自然语言描述后重试。`,
              `${data.error || "The problem file could not be parsed"} Add a natural-language `
                + "description to use a non-standard data file."
            ),
            true
          );
        return data;
      }
      acceptDescription(
        data,
        t(`已加载 ${file.name}`, `Loaded ${file.name}`)
      );
      return data;
    } catch (error) {
      const detail = error.message || t("加载问题文件失败", "Failed to load problem file");
      if (!quiet) showMessage(detail, true);
      setConnection("offline", t("本地服务异常", "Local service error"));
      return { ok: false, errorCode: "network_error", error: detail };
    }
  };

  window.EvoFlowProblem = Object.freeze({
    acceptDescription,
    showMessage,
    loadStandardFile: loadFile,
    hasCurrent: () => currentProblemAvailable
  });

  byId("problem-save-as").addEventListener("click", saveCurrentProblemAs);

  byId("exit-button").addEventListener("click", async () => {
    if (pageGuard && pageGuard(currentPage, "exit") === false) return;
    try { await api("/api/shutdown", { method: "POST" }); } catch (_) {}
    document.body.innerHTML =
      '<main style="max-width:680px;margin:12vh auto;padding:32px;font-family:Segoe UI,sans-serif">' +
      "<h1>EvoFlow 已退出</h1><p>本地服务已经关闭，可以安全关闭此标签页。</p></main>";
    window.close();
  });

  window.addEventListener("pagehide", () => {
    if (!token) return;
    fetch("/api/session/closing", {
      method: "POST",
      headers: { "X-EvoFlow-Token": token },
      keepalive: true
    }).catch(() => {});
  });

  window.addEventListener("evoflow:language", () => {
    const meta = pageMeta[currentPage];
    if (meta) {
      byId("page-eyebrow").textContent = t(...meta.eyebrow);
      byId("page-title").textContent = t(...meta.title);
    }
    if (currentProblemDescription) renderDescription(currentProblemDescription);
  });

  const heartbeat = async () => {
    if (!token) {
      setConnection("offline", "缺少会话令牌");
      showMessage("此页面缺少有效的 EvoFlow 会话，请重新运行 evoflow.exe。", true);
      return;
    }
    try {
      const data = await api("/api/health");
      if (!data.ok) throw new Error(data.error);
      if (data.version) byId("product-version").textContent = `EvoFlow ${data.version}`;
      setConnection("online", "本地服务已连接");
    } catch (_) {
      setConnection("offline", "本地服务未连接");
    }
  };

  heartbeat();
  setInterval(() => api("/api/heartbeat", { method: "POST" }).catch(() => {
    setConnection("offline", "本地服务未连接");
  }), 30000);
})();
