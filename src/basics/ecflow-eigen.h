//------------------------Description------------------------
// 对称矩阵特征分解(cyclic Jacobi)。CMA-ES 需 C=B·D²·Bᵀ(B 正交特征向量、D²=特征值)——ECFlow 无线性代数库,自实现。
//-------------------------Reference-------------------------
// Cyclic Jacobi eigenvalue algorithm(经典对称矩阵特征分解,数值稳定、无需外部依赖)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
//-----------------------------------------------------------

#pragma once
#include <vector>
#include <cmath>

namespace ECFlow
{
    // 对称矩阵 A(n×n row-major) 的特征分解:A = V·diag(eig)·Vᵀ。A 传值(内部销毁);V 列为特征向量。
    inline void jacobiEigen(std::vector<double> A, int n, std::vector<double>& eig, std::vector<double>& V, int max_sweeps = 100)
    {
        auto a = [&](int i, int j) -> double& { return A[i * n + j]; };
        auto v = [&](int i, int j) -> double& { return V[i * n + j]; };

        V.assign((size_t)n * n, 0.0);
        for (int i = 0; i < n; i++) v(i, i) = 1.0;

        for (int sweep = 0; sweep < max_sweeps; sweep++)
        {
            // 非对角元平方和,收敛即停
            double off = 0.0;
            for (int i = 0; i < n; i++)
                for (int j = i + 1; j < n; j++)
                    off += a(i, j) * a(i, j);
            if (off < 1e-30) break;

            for (int p = 0; p < n; p++)
                for (int q = p + 1; q < n; q++)
                {
                    if (std::fabs(a(p, q)) < 1e-300) continue;
                    // 旋转角:令 a'_pq = 0 → tan(2θ) = 2a_pq/(a_qq - a_pp)
                    double theta = 0.5 * std::atan2(2.0 * a(p, q), a(q, q) - a(p, p));
                    double c = std::cos(theta), s = std::sin(theta);
                    // A ← JᵀAJ:先列旋转(A·J),再行旋转(Jᵀ·A)
                    for (int i = 0; i < n; i++)
                    {
                        double aip = a(i, p), aiq = a(i, q);
                        a(i, p) = c * aip - s * aiq;
                        a(i, q) = s * aip + c * aiq;
                    }
                    for (int i = 0; i < n; i++)
                    {
                        double api = a(p, i), aqi = a(q, i);
                        a(p, i) = c * api - s * aqi;
                        a(q, i) = s * api + c * aqi;
                    }
                    // 累积特征向量 V ← V·J
                    for (int i = 0; i < n; i++)
                    {
                        double vip = v(i, p), viq = v(i, q);
                        v(i, p) = c * vip - s * viq;
                        v(i, q) = s * vip + c * viq;
                    }
                }
        }

        eig.assign(n, 0.0);
        for (int i = 0; i < n; i++) eig[i] = a(i, i);
    }
}
