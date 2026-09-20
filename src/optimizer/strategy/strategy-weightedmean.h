//------------------------Description------------------------
// 向量加权均值(Weighted Mean of Vectors)通用原语:给一个**按优劣排好序**的三元解(l1 优→l3 劣)与维号 d,
//   按 fitness 差的 cos/exp 权算"位置差的加权均值方向"(逐维)。INFO 的 MeanRule 由此构成;通用可复用。
//   **无状态纯函数**(同 strategy-levy.h 的 levyFlight),谁都能调。三元的排序由拓扑负责(方向感知)。

//-------------------------Reference-------------------------
// M. Ahmadianfar, A. A. Heidari, A. H. Gandomi, X. Chu, H. Chen, "INFO: An efficient optimization
// algorithm based on weighted mean of vectors," Expert Systems with Applications, vol. 195, 116516,
// 2022, doi: 10.1016/j.eswa.2022.116516.(权公式取自其加权均值规则)

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <cmath>
#include "solution.h"

namespace EvoFlow
{
	// 加权均值(逐维):l1/l2/l3 须**已按优劣排序**(l1 最优、l3 最差,由拓扑方向感知排好)。
	//   w_k = cos((f_a−f_b)+π)·exp(−|(f_a−f_b)/wf|),wf=|f1−f3|(fitness 跨度归一);
	//   返回 (w1·(x1−x2)+w2·(x1−x3)+w3·(x2−x3)) / (Σw+ε)。注:权按 min 向 fitness 差取(标准 INFO),方向在选择层保证。
	inline double weightedMeanVectors(Solution* l1, Solution* l2, Solution* l3, int d)
	{
		const double PI = 3.14159265358979323846;
		double f1 = l1->fitness[0], f2 = l2->fitness[0], f3 = l3->fitness[0];
		double wf = std::fabs(f1 - f3) + 1e-12;
		double w1 = std::cos((f1 - f2) + PI) * std::exp(-std::fabs((f1 - f2) / wf));
		double w2 = std::cos((f1 - f3) + PI) * std::exp(-std::fabs((f1 - f3) / wf));
		double w3 = std::cos((f2 - f3) + PI) * std::exp(-std::fabs((f2 - f3) / wf));
		double x1 = (*l1)[d], x2 = (*l2)[d], x3 = (*l3)[d];
		double num = w1 * (x1 - x2) + w2 * (x1 - x3) + w3 * (x2 - x3);
		double den = w1 + w2 + w3 + 1e-12;
		return num / den;
	}
}
