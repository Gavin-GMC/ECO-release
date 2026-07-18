//------------------------Description------------------------
// 分布模型的类别枚举(DistributionType)——库支持的全部分布模型种类。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once

namespace ECFlow
{
    enum class DistributionType
    {
        F_default,    // 0:占位无效
        F_Gaussian,   // 1:高斯
        F_Cauchy,     // 2:柯西(重尾)
        F_Uniform,    // 3:区间均匀
        F_Histogram,  // 4:边缘直方图(多峰)
        F_end
    };
}
