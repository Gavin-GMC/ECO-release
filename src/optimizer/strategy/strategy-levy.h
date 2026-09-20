//------------------------Description------------------------
// Levy 飞行随机步长(无状态工具函数):Mantegna 算法从重尾 Levy 稳定分布抽一个随机标量。
//   多小步 + 偶发大跳,利于跳出局部。供 GJO(GoldenJackalPincer)及 Tier B(HHO/Aquila/MPA…)等需重尾随机的策略复用。
//   **无任何状态**:每次调用独立、不属某个体、不跨代——本质是"另一种 rand01()",非个体特性。

//-------------------------Reference-------------------------
// R. N. Mantegna, "Fast, accurate algorithm for numerical simulation of Levy stable stochastic
// processes," Physical Review E, vol. 49, no. 5, pp. 4677-4683, 1994.

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <cmath>
#include "evoflow-rand.h"

namespace EvoFlow
{
	// Mantegna Levy 飞行:sigma_u 仅依赖 beta;u~N(0,sigma_u²),v~N(0,1);step = 0.01·u/|v|^(1/beta)。beta∈(0,2],标准 1.5。
	inline double levyFlight(double beta = 1.5)
	{
		const double PI = 3.14159265358979323846;
		double num = std::tgamma(1.0 + beta) * std::sin(PI * beta / 2.0);
		double den = std::tgamma((1.0 + beta) / 2.0) * beta * std::pow(2.0, (beta - 1.0) / 2.0);
		double sigma_u = std::pow(num / den, 1.0 / beta);
		double u = get_normal(0.0, sigma_u);
		double v = get_normal(0.0, 1.0);
		return 0.01 * u / std::pow(std::fabs(v) + 1e-12, 1.0 / beta);   // +1e-12 防 v=0
	}
}
