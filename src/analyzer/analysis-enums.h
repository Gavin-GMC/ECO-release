//------------------------Description------------------------
// 分析层【共享枚举】。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once

namespace EvoFlow
{
    // 跨阶段标量算子(归约维:对解集;聚合维:对多次运行)
    enum class Stat { Mean, Variance, Std, Median, Mode, Smallest, Largest,
                      Q1, Q3, IQR, Range, SEM, CV, GeometricMean, HarmonicMean };
}
