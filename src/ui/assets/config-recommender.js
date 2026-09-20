(() => {
  "use strict";

  const byId = (id) => document.getElementById(id);
  const llm = window.EvoFlowLlm;
  const problem = window.EvoFlowProblem;
  const config = window.EvoFlowConfig;
  if (!llm || !problem || !config) return;

  const button = byId("config-recommend");
  const status = byId("config-recommend-status");
  let busy = false;

  const setStatus = (text, kind = "") => {
    status.textContent = text;
    status.className = `config-recommend-status${kind ? ` ${kind}` : ""}`;
  };

  const attemptSuffix = (data) =>
    data && data.attempts > 1 ? `（共尝试 ${data.attempts} 次）` : "";

  const errorText = (data) => {
    const known = {
      invalid_settings: "模型设置无效。",
      unsupported_endpoint: "模型接口地址不受支持。",
      network_error: "无法连接模型服务。",
      timeout: "模型服务响应超时。",
      authentication_failed: "模型服务认证失败。",
      rate_limited: "模型服务当前限流。",
      provider_error: "模型服务拒绝了请求。",
      invalid_response: "模型服务返回了无效响应。",
      no_current_problem: "请先加载或智能构建一个问题。",
      problem_summary_too_large: "当前问题信息超过推荐上限。",
      config_catalog_unavailable: "现有配置候选目录不可用。",
      recommendation_invalid: "模型没有按要求返回候选配置名。",
      recommended_config_not_found: "模型返回的配置不在可用范围中。"
    };
    const prefix = known[data && data.errorCode] || "配置推荐失败。";
    const message = data && data.error ? `${prefix} ${data.error}` : prefix;
    return `${message}${attemptSuffix(data)}`;
  };

  const refreshHint = () => {
    if (busy) return;
    if (!problem.hasCurrent()) {
      setStatus("请先在“问题定义”中加载或智能构建问题。");
    } else if (!llm.isConnected()) {
      setStatus("当前问题已就绪；请先在右上角连接模型服务。");
    } else {
      setStatus("当前问题与模型均已就绪，可以开始推荐。", "ready");
    }
  };

  const recommend = async () => {
    if (busy) return;
    if (!problem.hasCurrent()) {
      setStatus("请先在“问题定义”中加载或智能构建问题。", "error");
      return;
    }
    if (!llm.isConnected()) {
      setStatus("请先在右上角完成模型设置并通过连接测试。", "error");
      llm.open();
      return;
    }

    busy = true;
    button.disabled = true;
    button.textContent = "正在选择…";
    setStatus("正在整理当前问题与现有配置候选，并请求模型精确选择…", "working");
    try {
      const data = await llm.request("/api/llm/config/recommend");
      if (!data.ok) throw data;
      const accepted = await config.acceptRecommendedDraft(data);
      if (accepted.cancelled) {
        setStatus("已取消载入，当前配置保持不变。");
        return;
      }
      if (!accepted.ok) throw new Error(accepted.error || "推荐配置无法载入。");
      setStatus(
        `已从 ${data.candidateCount} 个现有候选中选择并载入 ${data.configId}${attemptSuffix(data)}。`,
        "success"
      );
    } catch (error) {
      setStatus(
        error?.errorCode ? errorText(error) : (error?.message || "配置推荐失败。"),
        "error"
      );
    } finally {
      busy = false;
      button.disabled = false;
      button.textContent = "根据当前问题推荐";
    }
  };

  button.addEventListener("click", recommend);
  window.addEventListener("evoflow:problem-state", refreshHint);
  window.addEventListener("evoflow:llm-state", refreshHint);
  refreshHint();
})();
