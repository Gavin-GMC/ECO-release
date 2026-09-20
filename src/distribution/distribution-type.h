//------------------------Description------------------------
// 分布模型的类别枚举(DistributionType)——库支持的全部分布模型种类。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once

namespace EvoFlow
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
