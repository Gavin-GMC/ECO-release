(() => {
  "use strict";

  const byId = (id) => document.getElementById(id);
  const ui = window.EvoFlowUi;
  if (!ui) return;

  const panel = byId("llm-settings-panel");
  const openButton = byId("llm-settings-open");
  const closeButton = byId("llm-settings-close");
  const problemEntry = byId("llm-problem-entry");
  const endpointInput = byId("llm-endpoint");
  const modelInput = byId("llm-model");
  const apiKeyInput = byId("llm-api-key");
  const testButton = byId("llm-test");
  const statusMessage = byId("llm-settings-status");
  const topDot = byId("llm-top-dot");
  const topStatus = byId("llm-top-status");
  const problemStatus = byId("llm-problem-status");

  let state = "unconfigured";

  const stateText = {
    unconfigured: {
      top: "未配置",
      problem: "尚未连接",
      message: "填写完整接口地址和模型名称后测试连接。"
    },
    testing: {
      top: "测试中",
      problem: "正在测试连接",
      message: "正在连接模型服务，最长可能需要约 90 秒…"
    },
    connected: {
      top: "已连接",
      problem: "模型连接可用",
      message: "连接测试通过。设置仅在当前页面有效。"
    },
    error: {
      top: "连接异常",
      problem: "连接测试未通过",
      message: "连接测试未通过。"
    }
  };

  const setState = (next, detail = "") => {
    state = next;
    const text = stateText[next];
    topDot.className = `model-status-dot ${next}`;
    topStatus.textContent = text.top;
    problemStatus.textContent = text.problem;
    problemStatus.dataset.state = next;
    statusMessage.className = `llm-status-message ${next}`;
    statusMessage.textContent = detail || text.message;
    window.dispatchEvent(new CustomEvent("evoflow:llm-state", {
      detail: { state: next }
    }));
  };

  const open = () => {
    panel.hidden = false;
    openButton.setAttribute("aria-expanded", "true");
    endpointInput.focus();
  };

  const close = () => {
    panel.hidden = true;
    openButton.setAttribute("aria-expanded", "false");
    openButton.focus();
  };

  const currentSettings = () => ({
    endpoint: endpointInput.value,
    model: modelInput.value,
    apiKey: apiKeyInput.value
  });

  const request = (path, payload = {}) => ui.api(path, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ ...payload, ...currentSettings() })
  });

  const attemptSuffix = (data) =>
    data && data.attempts > 1 ? `（共尝试 ${data.attempts} 次）` : "";

  const errorText = (data) => {
    const known = {
      invalid_settings: "请检查接口地址、模型名称和 API Key。",
      unsupported_endpoint: "接口地址不受支持：公网必须使用 HTTPS，本机 HTTP 仅限 127.0.0.1 或 localhost。",
      network_error: "无法连接模型服务，请检查地址、网络和本地服务状态。",
      timeout: "模型服务响应超时，请稍后重试。",
      authentication_failed: "模型服务拒绝认证，请检查 API Key。",
      rate_limited: "模型服务当前限流，请稍后重试。",
      provider_error: "模型服务拒绝了请求。",
      invalid_response: "模型服务返回了无法识别的响应。"
    };
    const message = data && data.error
      ? `${known[data.errorCode] || "连接测试失败"} ${data.error}`
      : (known[data && data.errorCode] || "连接测试失败。");
    return `${message}${attemptSuffix(data)}`;
  };

  const test = async () => {
    if (!endpointInput.value.trim() || !modelInput.value.trim()) {
      setState("error", "请先填写完整接口地址和模型名称。");
      return;
    }
    setState("testing");
    testButton.disabled = true;
    testButton.textContent = "正在测试…";
    try {
      const data = await request("/api/llm/test");
      if (!data.ok) throw data;
      setState("connected", `连接测试通过${attemptSuffix(data)}。设置仅在当前页面有效。`);
    } catch (error) {
      setState("error", errorText(error));
    } finally {
      testButton.disabled = false;
      testButton.textContent = "测试连接";
    }
  };

  const markChanged = () => {
    if (state !== "testing") {
      const hasMinimum = endpointInput.value.trim() && modelInput.value.trim();
      setState("unconfigured", hasMinimum
        ? "设置已修改，请重新测试连接。"
        : "填写完整接口地址和模型名称后测试连接。");
    }
  };

  openButton.addEventListener("click", () => panel.hidden ? open() : close());
  closeButton.addEventListener("click", close);
  problemEntry.addEventListener("click", open);
  testButton.addEventListener("click", test);
  [endpointInput, modelInput, apiKeyInput].forEach((input) => {
    input.addEventListener("input", markChanged);
  });
  document.addEventListener("keydown", (event) => {
    if (event.key === "Escape" && !panel.hidden) close();
  });

  window.EvoFlowLlm = Object.freeze({
    open,
    request,
    isConnected: () => state === "connected"
  });
  setState("unconfigured");
})();
