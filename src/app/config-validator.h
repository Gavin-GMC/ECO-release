//------------------------Description------------------------
// 配置验证。CLI 的 `evoflow validate` 与 UI 的「优化器构筑页实时校验」共用。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <string>
#include <vector>

namespace EvoFlow
{
    namespace App
    {
        struct ValidateOptions
        {
            std::string path;              // .cfg 文件,或含 .cfg 的目录
            bool recursive = false;        // 目录递归(默认只扫一层)
            bool deepBuild = false;        // --build:真装配一次(默认关)
        };

        // 一条问题。component/reason 来自底层 ValidateIssue 的结构化字段(v2.3.1.4)。
        struct ConfigIssue
        {
            std::string file;        // 所属配置文件(相对给定路径)
            std::string scope;       // 所属范围:workflow tag / subpopulation tag / "(build)"
            std::string scopeType;   // workflow/subpopulation/load/build
            int         scopeIndex = -1;       // workflows/subpopulations 下标;load/build 等为 -1
            std::string component;   // 触发组件的 tag(非组件级问题为空)
            std::string category;    // 该组件的类别关键字(strategy/topology/…)
            int         componentIndex = -1;   // workflow.components 下标;非 workflow 组件为 -1
            std::string reason;      // 机器可读原因键(见 assert-matcher.h 的键表)
            std::string message;     // 完整可读消息
            std::string severity = "error"; // error/warning/info；pass 只用于整体状态
            bool        isError = true;      // 兼容旧消费方
        };

        struct FileReport
        {
            std::string file;
            bool ok = true;          // 无 error；warning/info 均不影响
            int  errorCount = 0;
            int  warningCount = 0;
            int  infoCount = 0;
            std::string status = "pass"; // error/warning/info/pass，取该文件最高严重级
            std::string loadError;   // 非空 = 配置根本没读进来(解析失败)，同时产生 load Error issue
        };

        struct ValidateReport
        {
            int scanned = 0, passed = 0, failed = 0;
            int errorFiles = 0, warningFiles = 0, infoFiles = 0, passFiles = 0;
            std::string status = "pass";    // 所有文件中的最高严重级
            bool deepBuild = false;              // 本次用的哪种深度(报告里要标明)
            std::vector<FileReport> files;
            std::vector<ConfigIssue> issues;     // 全部文件的问题汇总(按 file 有序)
            std::string fatal;                   // 非空 = 路径不存在等,整体失败
        };

        // 扫描并验证。不输出、不抛;失败经 report.fatal / FileReport::loadError 回报。
        ValidateReport validatePath(const ValidateOptions& opt);

        // 验证内存中的一份 v3.1/v3.2 配置文本。与 validatePath 走同一聚合函数和 matcher,
        // UI 实时校验不得另建一套规则。fileName 只用于报告展示,不访问文件系统。
        ValidateReport validateConfigText(const std::string& text, bool deepBuild = false,
                                          const std::string& fileName = "(memory)");
    }
}
