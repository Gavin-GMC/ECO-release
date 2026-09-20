//------------------------Description------------------------
// 优化器配置编辑门面:在内存中创建、读取和编码 .cfg。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <string>

namespace EvoFlow
{
    namespace App
    {
        struct ConfigDraftResult
        {
            bool        ok = false;
            std::string error;
            std::string sourceVersion;  // 输入版本;新建草稿为默认保存版本 v3.1
            std::string canonicalJson;  // 规范 v3.2 JSON,供浏览器内存编辑
        };

        struct ConfigEncodeResult
        {
            bool        ok = false;
            std::string error;
            std::string version;
            std::string text;
        };

        // 积木视图连接候选检查。错误仍细分 failsatisfied/unsatisfied；非阻断状态增加
        // informational，以与配置校验的 Error/Warning/Info/Pass 分级一致。
        struct ConfigFlowCheckResult
        {
            bool        ok = false;
            bool        allowed = false;
            std::string error;
            std::string state;       // satisfied/informational/notfullysatisfied/failsatisfied/unsatisfied
            std::string reason;      // CLI 同源 reason key
            std::string message;     // CLI 同源可读报告
            int         errors = 0;
            int         warnings = 0;
            int         infos = 0;
        };

        // 一个 workflow + 一个子种群的最小结构骨架;算法组件留给用户添加。
        ConfigDraftResult createConfigDraft();
        // 读取 v3.1/v3.2 并规范化为 v3.2 JSON。
        ConfigDraftResult loadConfigDraft(const std::string& text);
        // 将规范草稿编码成目标版本(v3.1/v3.2)。
        ConfigEncodeResult encodeConfigDraft(const std::string& canonicalJson,
                                             const std::string& version);
        // 检查候选草稿中指定积木的连接状态。只关注该组件的结构化问题；
        // workflow 尚未完整造成的全局问题不会阻止逐步搭建。
        ConfigFlowCheckResult checkConfigFlowConnection(
            const std::string& canonicalJson, int workflowIndex, int componentIndex);
    }
}
