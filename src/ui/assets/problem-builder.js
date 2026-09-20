(() => {
  "use strict";

  const MAX_ATTACHMENT_BYTES = 1024 * 1024;
  const byId = (id) => document.getElementById(id);
  const t = (zh, en) => window.EvoFlowI18n?.t(zh, en) || zh;
  const llm = window.EvoFlowLlm;
  const problemUi = window.EvoFlowProblem;
  if (!llm || !problemUi) return;

  const descriptionInput = byId("ai-problem-description");
  const fileInput = byId("file-input");
  const chooseButton = byId("choose-button");
  const clearFileButton = byId("clear-problem-file");
  const dropZone = byId("drop-zone");
  const selectedFileCard = byId("selected-problem-file");
  const selectedFileName = byId("selected-problem-file-name");
  const selectedFileSize = byId("selected-problem-file-size");
  const buildButton = byId("ai-problem-build");
  const message = byId("ai-problem-message");
  const progress = byId("ai-problem-progress");
  const classifyStep = byId("ai-step-classify");
  const generateStep = byId("ai-step-generate");
  const classifiedType = byId("ai-classified-type");

  let selectedFile = null;
  let busy = false;

  const setMessage = (text, error = false) => {
    message.hidden = false;
    message.className = `ai-problem-message${error ? " error" : ""}`;
    message.textContent = text;
  };

  const resetProgress = () => {
    progress.hidden = false;
    classifyStep.className = "ai-step pending";
    generateStep.className = "ai-step pending";
    classifiedType.textContent = t("等待识别", "Waiting");
  };

  const setStep = (step, state) => {
    step.className = `ai-step ${state}`;
  };

  const formatFileSize = (bytes) => {
    if (bytes < 1024) return `${bytes} B`;
    if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KiB`;
    return `${(bytes / (1024 * 1024)).toFixed(1)} MiB`;
  };

  const inputMode = () => {
    const hasDescription = Boolean(descriptionInput.value.trim());
    if (hasDescription && selectedFile) return "combined";
    if (hasDescription) return "description";
    if (selectedFile) return "standard";
    return "empty";
  };

  const updateMode = () => {
    if (!busy) {
      buildButton.textContent = t("定义或加载问题", "Define or load problem");
      buildButton.disabled = inputMode() === "empty";
    }
  };

  const setSelectedFile = (file) => {
    selectedFile = file || null;
    fileInput.value = "";
    selectedFileCard.hidden = !selectedFile;
    dropZone.classList.toggle("has-file", Boolean(selectedFile));
    if (selectedFile) {
      selectedFileName.textContent = selectedFile.name;
      selectedFileSize.textContent = formatFileSize(selectedFile.size);
    } else {
      selectedFileName.textContent = "";
      selectedFileSize.textContent = "";
    }
    updateMode();
  };

  const attemptSuffix = (data) =>
    data && data.attempts > 1
      ? t(`（共尝试 ${data.attempts} 次）`, ` (${data.attempts} attempts)`)
      : "";

  const errorText = (data) => {
    const known = {
      invalid_settings: ["模型设置无效。", "Model settings are invalid."],
      invalid_description: ["问题描述为空或过长。", "The problem description is empty or too long."],
      invalid_attachment: ["所附数据不是可用的文本数据。", "The attachment is not usable text data."],
      attachment_too_large: ["所附参考数据超过 1 MiB。", "The reference data is larger than 1 MiB."],
      unsupported_endpoint: ["模型接口地址不受支持。", "The model endpoint is not supported."],
      network_error: ["无法连接模型服务。", "Could not connect to the model service."],
      timeout: ["模型服务响应超时。", "The model service timed out."],
      authentication_failed: ["模型服务认证失败。", "Model service authentication failed."],
      rate_limited: ["模型服务当前限流。", "The model service is rate-limiting requests."],
      provider_error: ["模型服务拒绝了请求。", "The model service rejected the request."],
      invalid_response: ["模型服务返回了无效响应。", "The model service returned an invalid response."],
      classification_invalid: ["模型没有按要求返回问题类型。", "The model did not return a valid problem type."],
      unsupported_problem_type: [
        "当前输入暂时无法归入支持的问题类型。",
        "The input does not match a supported problem type."
      ],
      template_catalog_mismatch: ["可用问题类型暂时不可用。", "Problem types are temporarily unavailable."],
      template_material_unavailable: ["该问题类型暂时不可用。", "This problem type is temporarily unavailable."],
      temporary_problem_unavailable: ["无法保存生成的问题数据。", "Generated problem data could not be saved."],
      generation_invalid: ["模型没有按要求返回问题数据。", "The model did not return valid problem data."],
      generated_data_too_large: ["模型生成的问题数据超过大小限制。", "Generated problem data is too large."],
      generated_data_invalid: ["模型生成的数据未通过检查。", "Generated data failed validation."],
      problem_validation_failed: ["生成的数据未通过检查。", "Generated data failed validation."]
    };
    const prefix = known[data && data.errorCode] || ["问题构建失败。", "Problem construction failed."];
    const localized = t(...prefix);
    const detail = data && data.error ? ` ${data.error}` : "";
    return `${localized}${detail}${attemptSuffix(data)}`;
  };

  const readAttachmentText = async (file) => {
    if (file.size > MAX_ATTACHMENT_BYTES) {
      throw {
        errorCode: "attachment_too_large",
        error: t(
          "描述与文件联合构建时，参考数据须不超过 1 MiB。",
          "Reference data must be no larger than 1 MiB for a combined build."
        )
      };
    }
    const bytes = new Uint8Array(await file.arrayBuffer());
    if (bytes.includes(0)) {
      throw {
        errorCode: "invalid_attachment",
        error: t(
          "当前仅支持 UTF-8 文本、CSV、TSV 或 JSON 参考数据，不支持二进制文件。",
          "Only UTF-8 text, CSV, TSV or JSON reference data is supported; binary files are not."
        )
      };
    }
    let text;
    try {
      text = new TextDecoder("utf-8", { fatal: true }).decode(bytes);
    } catch (_) {
      throw {
        errorCode: "invalid_attachment",
        error: t(
          "参考数据不是有效的 UTF-8 文本。",
          "The reference data is not valid UTF-8 text."
        )
      };
    }
    if (!text.trim()) {
      throw {
        errorCode: "invalid_attachment",
        error: t("参考数据文件为空。", "The reference data file is empty.")
      };
    }
    return text;
  };

  const buildWithModel = async (description, attachment = null) => {
    if (!llm.isConnected()) {
      setMessage(
        t(
          "请先在右上角完成模型设置并通过连接测试。",
          "Configure the model in the top-right corner and pass the connection test first."
        ),
        true
      );
      llm.open();
      return;
    }

    resetProgress();
    setStep(classifyStep, "active");
    setMessage(t("正在识别问题类型…", "Identifying the problem type…"));
    const attachmentFields = attachment ? {
      attachmentName: attachment.fileName,
      attachmentText: attachment.text
    } : {};

    const classified = await llm.request("/api/llm/problem/classify", {
      description,
      ...attachmentFields
    });
    if (!classified.ok) throw classified;
    classifiedType.textContent = classified.type;
    setStep(classifyStep, "complete");
    setStep(generateStep, "active");
    setMessage(
      t(
        `已识别为 ${classified.type}${attemptSuffix(classified)}，正在生成并检查问题数据…`,
        `Identified as ${classified.type}${attemptSuffix(classified)}. Generating and `
          + "validating problem data…"
      )
    );

    const generated = await llm.request("/api/llm/problem/generate", {
      description,
      type: classified.type,
      ...attachmentFields
    });
    if (!generated.ok) throw generated;
    setStep(generateStep, "complete");
    setMessage(
      t(
        `已自动生成并加载 ${classified.type} 问题${attemptSuffix(generated)}。`,
        `Generated and loaded a ${classified.type} problem${attemptSuffix(generated)}.`
      )
    );
    problemUi.acceptDescription(
      generated,
      t(
        `已通过模型生成并加载 ${classified.type} 问题`,
        `Generated and loaded a ${classified.type} problem`
      )
    );
  };

  const submit = async () => {
    const mode = inputMode();
    const description = descriptionInput.value.trim();
    if (mode === "empty") {
      setMessage(
        t("请填写问题描述或选择数据文件。", "Enter a problem description or select a data file."),
        true
      );
      descriptionInput.focus();
      return;
    }

    busy = true;
    buildButton.disabled = true;
    buildButton.textContent = t("正在处理…", "Processing…");
    try {
      if (mode === "standard") {
        progress.hidden = true;
        setMessage(t("正在按标准问题格式检查文件…", "Inspecting the standard problem file…"));
        await problemUi.loadStandardFile(selectedFile);
        message.hidden = true;
        return;
      }

      let attachment = null;
      if (mode === "combined") {
        progress.hidden = true;
        setMessage(t(
          "正在识别所选数据文件…",
          "Identifying the selected data file…"
        ));
        const local = await problemUi.loadStandardFile(selectedFile, { quiet: true });
        if (local?.ok) {
          message.hidden = true;
          return;
        }
        if (local?.errorCode === "network_error"
            || local?.errorCode === "file_too_large")
          throw local;
        setMessage(t(
          "文件不是标准问题格式，正在读取为参考数据…",
          "The file is not a standard problem file. Reading it as reference data…"
        ));
        attachment = {
          fileName: selectedFile.name,
          text: await readAttachmentText(selectedFile)
        };
      }
      await buildWithModel(description, attachment);
    } catch (error) {
      if (classifyStep.classList.contains("active")) setStep(classifyStep, "error");
      if (generateStep.classList.contains("active")) setStep(generateStep, "error");
      setMessage(errorText(error), true);
    } finally {
      busy = false;
      updateMode();
    }
  };

  chooseButton.addEventListener("click", (event) => {
    event.stopPropagation();
    fileInput.click();
  });
  clearFileButton.addEventListener("click", (event) => {
    event.stopPropagation();
    setSelectedFile(null);
  });
  dropZone.addEventListener("click", (event) => {
    if (!event.target.closest("button")) fileInput.click();
  });
  dropZone.addEventListener("keydown", (event) => {
    if (event.target.closest("button")) return;
    if (event.key === "Enter" || event.key === " ") {
      event.preventDefault();
      fileInput.click();
    }
  });
  fileInput.addEventListener("change", () => setSelectedFile(fileInput.files[0]));
  ["dragenter", "dragover"].forEach((eventName) => {
    dropZone.addEventListener(eventName, (event) => {
      event.preventDefault();
      dropZone.classList.add("dragging");
    });
  });
  ["dragleave", "drop"].forEach((eventName) => {
    dropZone.addEventListener(eventName, (event) => {
      event.preventDefault();
      dropZone.classList.remove("dragging");
    });
  });
  dropZone.addEventListener("drop", (event) => {
    setSelectedFile(event.dataTransfer.files[0]);
  });
  buildButton.addEventListener("click", submit);
  descriptionInput.addEventListener("input", updateMode);
  descriptionInput.addEventListener("keydown", (event) => {
    if ((event.ctrlKey || event.metaKey) && event.key === "Enter") submit();
  });
  window.addEventListener("evoflow:language", updateMode);

  progress.hidden = true;
  updateMode();
})();
