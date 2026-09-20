//------------------------Description------------------------
// 组件目录:跨 Base 的注册表汇总层。CLI/UI 共用,属门面层(src/app)。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <string>
#include <vector>
#include "module-type.h"

namespace EvoFlow
{
    namespace App
    {
        // 参数元数据:直接映射 ParamInfo(UI 的下拉框/滑条/校验提示所需字段已齐)
        struct ParamMeta
        {
            std::string name;                  // 参数名(配置/文档显示)
            int         kind = 0;              // ParamKind 的整数值(Int/Enum 会被校验整值性)
            double      low = 0, high = 0;     // 合法取值范围(硬边界,校验用)
            double      recLow = 0, recHigh = 0;   // 推荐取值范围(通常窄于合法范围)
            bool        allowEmpty = false;    // 允许 EMPTYVALUE(未设置 → 用组件内部默认)
        };

        struct ComponentMeta
        {
            std::string tag;                   // 注册名(= .cfg 里写的那个)
            ModuleType  category = ModuleType::T_default;
            std::string categoryKey;           // 类别关键字(strategy/topology/archive/…)
            std::vector<ParamMeta> params;     // **第一级**参数;渐进披露的下级由 ParameterTemplate::next 给
        };

        // 全部 12 类 / 单类。返回按 (类别, tag) 稳定排序。
        std::vector<ComponentMeta> listComponents();
        std::vector<ComponentMeta> listComponents(ModuleType t);

        // 初始化器不走 Registry,但配置编辑器需要同形元数据。categoryKey 固定为 "initializer"。
        std::vector<ComponentMeta> listInitializers();

        // 按给定值逐级展开 ParameterTemplate::next。params 是当前取值下的完整 schema,
        // defaults 与 params 等长,取各项推荐下界;调用方可用于“新选择组件”的初值。
        struct ResolvedParameterSchema
        {
            bool                   ok = false;
            std::string            error;
            std::vector<ParamMeta> params;
            std::vector<double>    defaults;
        };
        ResolvedParameterSchema resolveParameterSchema(
            ModuleType t, const std::string& tag, const std::vector<double>& values);
        ResolvedParameterSchema resolveParameterSchema(
            const std::string& categoryKey, const std::string& tag,
            const std::vector<double>& values);

        // UI 的类别下拉:全部类别关键字(与 categoryKey 同一套字符串)
        std::vector<std::string> categoryKeys();
        // 关键字 ↔ ModuleType(覆盖全 12 类;比 config-codec 的 typeKeyword 广 ——
        //   后者只覆盖 workflow 内 6 类,子群级 6 类在 .cfg 里是独立字段而非 component 行)
        std::string categoryKeyOf(ModuleType t);
        ModuleType  categoryOfKey(const std::string& key);

        // 依赖/供给查询。**必须给参数取值**(见文件头说明);para 长度不足时按组件默认补齐。
        struct RequirementItem
        {
            std::string item;        // 断言项名(如 "objects"/"whalestate")
            int         number = 0;  // 该项的需求/供给【数量】(Assert::getNumber)
            int         owner = 0;   // 断言归属的组件类别(ModuleType 的整数值)
            std::string detail;      // 可读描述
        };
        std::vector<RequirementItem> queryRequirementsAt(ModuleType t, const std::string& tag,
                                                         const std::vector<double>& para);
        std::vector<RequirementItem> queryProvisionsAt  (ModuleType t, const std::string& tag,
                                                         const std::vector<double>& para);
        // 便捷重载:用各参数的**推荐下界**作为取值。
        //   注意:这只是"该取值下"的依赖,**不是唯一答案** —— 见文件头。
        std::vector<RequirementItem> queryRequirementsAtRecommended(ModuleType t, const std::string& tag);
    }
}
