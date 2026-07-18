//------------------------Description------------------------
// 优化器组件的参数模板:机器可读的参数 schema —— 种类、合法范围、是否可空、推荐范围。
//   主要面向未来"自动算法生成 / 自动调参"(在推荐范围内采样);兼作参数自文档。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <string>
#include <vector>

namespace ECFlow
{
    // 参数种类:决定自动生成 / 配置如何解释并采样该 double
    enum class ParamKind { Enum, Int, Real };

    // 单个参数的机器可读元信息
    struct ParamInfo {
        std::string name;               // 参数名(文档 / 配置显示)
        ParamKind   kind;               // 种类(枚举 / 整数 / 实数)
        double      low, high;          // 合法取值范围(硬边界,校验用)
        bool        allow_empty;        // 是否允许取 EMPTYVALUE(未设置 → 用内部默认)
        double      rec_low, rec_high;  // 推荐取值范围(自动生成 / 补全采样用)
    };

    // 组件的参数模板。**自包含**:除本级参数外,还知道"本级之后还有什么"(渐进披露),
    //   故任一 ParameterTemplate 都是可独立解析、可被嵌套复用的完整单元。
    struct ParameterTemplate {
        std::vector<ParamInfo> params;   // 本级参数

        // ---------------- 渐进披露----------------
        // 本级之后还有什么 —— schema 由**本级已确定的取值**决定;nullptr = 定长,无下一级。
        //
        // ★ 契约:`my_para` 是**本级切片**(从本级第一个参数起),**不是整个 para**。
        //   即每个模板永远从自己的下标 0 开始看世界,不关心自己被嵌在第几层 —— 这是模板可嵌套复用的前提。
        //   反例:若约定"永远传整个 para",则 `TabuDecorator` 把内层模板原样返回后,内层的 next 会去读
        //   para[0](那是 Tabu 的 `inner`,不是它自己的参数)→ 当场读错。切片语义使该错误不可能发生。
        //
        // 为何是"描述"(返模板)而非"裁决"(返 bool):参数模板的本意是**机读入口**,消费方有二 ——
        //   ①装配期校验(解析下一级后按通用规则校验) ②自动化配置生成(采完本级后问"接下来要什么");
        //   只答合法/非法的钩子服务不了 ②。
        //
        // 现有用例:`Index`/`Close`/`Kinship`(accept_type → 接受准则的参数)、
        //   `TabuDecorator`(inner → **内层组件的完整模板**,其自身的 next 随之带入,天然多级)、
        //   `AdaptiveOperatorSelection`(op_count → N 个算子枚举)。
        //
        // ⚠ 本字段使多级递归成为可能(next 可返回含 next 的模板,甚至构成环)→ **解析方必须设深度上限**,
        //   见 ParamMatcher::MAX_DEPTH。
        ParameterTemplate (*next)(const double* my_para, size_t n) = nullptr;

        size_t count() const { return params.size(); }   // **仅本级**;总数依各级取值而定,须逐级解析
        bool hasNext() const { return next != nullptr; }
        // 取下一级(无则返回空模板)。my_para 须为**本级切片**。
        ParameterTemplate nextLevel(const double* my_para, size_t n) const
        {
            return next ? next(my_para, n) : ParameterTemplate{};
        }
    };
}
