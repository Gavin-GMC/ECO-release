// config-editor.cpp — 内存配置草稿与既有 .cfg codec 的适配层。

// SPDX-License-Identifier: BSD-3-Clause

#include "config-editor.h"
#include "config-validator.h"
#include "evoflow.h"
#include <cctype>

namespace EvoFlow
{
    namespace App
    {
        namespace
        {
            FullConfig decodeText(const std::string& text, std::string* version = nullptr)
            {
                const std::string detected = ConfigBuilder::detectVersion(text);
                if (version) *version = detected;
                if (detected == "v3.1") return ConfigCodecV31::decode(text);
                if (detected == "v3.2") return ConfigCodecV32::decode(text);
                throw std::runtime_error(
                    "[config] unsupported/undetected version: '" + detected + "'");
            }

            ConfigDraftResult fromConfig(const FullConfig& config,
                                         const std::string& sourceVersion)
            {
                ConfigDraftResult result;
                result.ok = true;
                result.sourceVersion = sourceVersion;
                result.canonicalJson = ConfigCodecV32::encode(config);
                return result;
            }

            bool isPortableToken(const std::string& value, bool allowEmpty = false)
            {
                if (value.empty()) return allowEmpty;
                for (unsigned char ch : value)
                    if (!(std::isalnum(ch) || ch == '_' || ch == '-'
                          || ch == '.' || ch == '+'))
                        return false;
                return true;
            }

            void requirePortableTokens(const FullConfig& config)
            {
                if (!isPortableToken(config.optimizer.name))
                    throw std::runtime_error(
                        "[config] optimizer.name must use letters, digits, '_', '-', '.' or '+'");
                if (!isPortableToken(config.optimizer.tag, true))
                    throw std::runtime_error(
                        "[config] optimizer.tag must be empty or a portable token");
                for (size_t w = 0; w < config.workflows.size(); w++)
                {
                    const WorkflowConfig& workflow = config.workflows[w];
                    if (!isPortableToken(workflow.tag))
                        throw std::runtime_error(
                            "[config] workflow tag must be a portable token");
                    if (!isPortableToken(workflow.ini_tag))
                        throw std::runtime_error(
                            "[config] initializer tag must be a portable token");
                    for (size_t c = 0; c < workflow.components.size(); c++)
                        if (!isPortableToken(workflow.components[c].tag))
                            throw std::runtime_error(
                                "[config] component tag must be a portable token");
                }
                const OptimizerConfig& optimizer = config.optimizer;
                const std::string cooperationTags[] = {
                    optimizer.cooperation.manager_tag,
                    optimizer.cooperation.constructer_tag,
                    optimizer.cooperation.topology_tag,
                    optimizer.g_archive_tag
                };
                for (const std::string& tag : cooperationTags)
                    if (!isPortableToken(tag))
                        throw std::runtime_error(
                            "[config] cooperation/archive tag must be a portable token");
                for (size_t s = 0; s < optimizer.subpopulations.size(); s++)
                {
                    const SubpopulationConfig& subpopulation = optimizer.subpopulations[s];
                    if (!isPortableToken(subpopulation.tag)
                        || !isPortableToken(subpopulation.workflow_tag)
                        || !isPortableToken(subpopulation.archive_tag))
                        throw std::runtime_error(
                            "[config] subpopulation fields must be portable tokens");
                }
            }
        }

        ConfigDraftResult createConfigDraft()
        {
            try
            {
                FullConfig config;
                WorkflowConfig workflow;
                workflow.tag = "wf";
                workflow.ini_tag = "Random";
                config.workflows.push_back(workflow);

                config.optimizer.name = "optimizer";
                config.optimizer.tag.clear();
                SubpopulationConfig subpopulation;
                subpopulation.tag = "1";
                subpopulation.size = 100;
                subpopulation.workflow_tag = workflow.tag;
                config.optimizer.subpopulations.push_back(subpopulation);
                return fromConfig(config, "v3.1");
            }
            catch (const std::exception& ex)
            {
                ConfigDraftResult result;
                result.error = ex.what();
                return result;
            }
            catch (...)
            {
                ConfigDraftResult result;
                result.error = "unknown error while creating config draft";
                return result;
            }
        }

        ConfigDraftResult loadConfigDraft(const std::string& text)
        {
            try
            {
                std::string version;
                const FullConfig config = decodeText(text, &version);
                return fromConfig(config, version);
            }
            catch (const std::exception& ex)
            {
                ConfigDraftResult result;
                result.error = ex.what();
                return result;
            }
            catch (...)
            {
                ConfigDraftResult result;
                result.error = "unknown error while loading config draft";
                return result;
            }
        }

        ConfigEncodeResult encodeConfigDraft(const std::string& canonicalJson,
                                             const std::string& version)
        {
            ConfigEncodeResult result;
            result.version = version;
            try
            {
                const FullConfig config = decodeText(canonicalJson);
                requirePortableTokens(config);
                if (version == "v3.1")
                    result.text = ConfigCodecV31::encode(config);
                else if (version == "v3.2")
                    result.text = ConfigCodecV32::encode(config);
                else
                    throw std::runtime_error(
                        "[config] unsupported version for saving: " + version);
                result.ok = true;
            }
            catch (const std::exception& ex)
            {
                result.error = ex.what();
            }
            catch (...)
            {
                result.error = "unknown error while encoding config draft";
            }
            return result;
        }

        ConfigFlowCheckResult checkConfigFlowConnection(
            const std::string& canonicalJson, int workflowIndex, int componentIndex)
        {
            ConfigFlowCheckResult result;
            try
            {
                const FullConfig config = decodeText(canonicalJson);
                if (workflowIndex < 0
                    || workflowIndex >= static_cast<int>(config.workflows.size()))
                    throw std::runtime_error("[config] workflow index is out of range");
                const WorkflowConfig& workflow =
                    config.workflows[static_cast<size_t>(workflowIndex)];
                if (componentIndex < 0
                    || componentIndex >= static_cast<int>(workflow.components.size()))
                    throw std::runtime_error("[config] component index is out of range");

                const ValidateReport report =
                    validateConfigText(canonicalJson, false, "flow-preview.cfg");
                bool hasTargetError = false;
                bool hasTargetWarning = false;
                bool hasTargetInfo = false;
                bool hasInsufficientNumber = false;
                int selectedSeverity = -1;
                for (const ConfigIssue& issue : report.issues)
                {
                    if (issue.severity == "error") result.errors++;
                    else if (issue.severity == "warning") result.warnings++;
                    else if (issue.severity == "info") result.infos++;
                    if (issue.scopeType != "workflow"
                        || issue.scopeIndex != workflowIndex
                        || issue.componentIndex != componentIndex)
                        continue;

                    if (issue.isError)
                    {
                        hasTargetError = true;
                        if (issue.reason == "insufficient_number")
                            hasInsufficientNumber = true;
                    }
                    else if (issue.severity == "warning")
                        hasTargetWarning = true;
                    else if (issue.severity == "info")
                        hasTargetInfo = true;

                    const int issueSeverity = issue.severity == "error" ? 2
                        : (issue.severity == "warning" ? 1 : 0);
                    if (issueSeverity > selectedSeverity)
                    {
                        selectedSeverity = issueSeverity;
                        result.reason = issue.reason;
                        result.message = issue.message;
                    }
                }

                result.ok = true;
                result.allowed = !hasTargetError;
                if (hasInsufficientNumber)
                    result.state = "failsatisfied";
                else if (hasTargetError)
                    result.state = "unsatisfied";
                else if (hasTargetWarning)
                    result.state = "notfullysatisfied";
                else if (hasTargetInfo)
                    result.state = "informational";
                else
                    result.state = "satisfied";
            }
            catch (const std::exception& ex)
            {
                result.error = ex.what();
            }
            catch (...)
            {
                result.error = "unknown error while checking a flow connection";
            }
            return result;
        }
    }
}
