//------------------------Description------------------------
// 计算器规格(spec):定义态存"函数指针/functor/公式",compile 期再构造可执行 Calculator。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <string>
#include <memory>
#include "ecflow-functor.hpp"   // eccalcul_functor

namespace ECFlow
{
    enum class CalcKind { none, func_ptr, functor, formula };

    struct CalcSpec
    {
        CalcKind            kind     = CalcKind::none;
        double            (*func)(double**) = nullptr;   // func_ptr
        // functor：OWNED 副本（add 时即 functor->copy() 存入）——调用方的 functor 可立即释放，
        // 不必活到 compile()。shared_ptr 使 CalcSpec 仍可拷贝/移动（随 Spec 向量）。
        std::shared_ptr<eccalcul_functor> functor;
        std::string         formula;                     // formula
    };
}
