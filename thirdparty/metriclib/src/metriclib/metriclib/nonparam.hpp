#ifndef NONPARAM_HPP
#define NONPARAM_HPP

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <map>

namespace MetricLib {

    // 辅助函数：计算数组元素的秩（支持最小化/最大化，结值平均秩，0起始）
    static double* rankElements(double* input, int size, bool min_is_better = true) {
        if (size <= 0 || input == nullptr) {
            throw std::invalid_argument("rankElements: 输入数组为空或长度≤0");
        }

        // 创建索引数组用于排序
        int* indices = new int[size];
        for (int i = 0; i < size; ++i) {
            indices[i] = i;
        }

        // 按值排序（升序/降序）
        std::sort(indices, indices + size, [input, min_is_better](int i, int j) {
            if (min_is_better) {
                return input[i] < input[j]; // 最小化：升序
            }
            else {
                return input[i] > input[j]; // 最大化：降序
            }
            });

        int begin_rank, end_rank;
        double average_rank;
        bool is_begin = true;
        double* ranks = new double[size](); // 初始化全0

        // 处理结值，计算平均秩
        for (int i = 0; i < size - 1; ++i) {
            if (std::abs(input[indices[i]] - input[indices[i + 1]]) < std::abs(input[indices[i]] * 1e-8)) {
                if (is_begin) {
                    is_begin = false;
                    begin_rank = i;
                }
            }
            else {
                if (is_begin) {
                    ranks[indices[i]] = i + 1;   // 1 基秩(标准;原 0 基致 KW 负 H、MW U 偏移)
                }
                else {
                    end_rank = i;
                    average_rank = (begin_rank + end_rank) / 2.0 + 1;   // 1 基
                    for (int k = begin_rank; k <= end_rank; ++k) {
                        ranks[indices[k]] = average_rank;
                    }
                    is_begin = true;
                }
            }
        }

        // 处理最后一个元素/最后一段结值
        if (is_begin) {
            ranks[indices[size - 1]] = size;   // 1 基
        }
        else {
            end_rank = size - 1;
            average_rank = (begin_rank + end_rank) / 2.0;
            for (int k = begin_rank; k <= end_rank; ++k) {
                ranks[indices[k]] = average_rank;
            }
        }

        delete[] indices;
        return ranks;
    }

    // 辅助函数：统计数组元素出现次数（结值修正用）
    static std::map<double, int> appearCount(double* input, int size) {
        std::map<double, int> countMap;
        if (size <= 0 || input == nullptr) {
            return countMap;
        }
        for (int i = 0; i < size; ++i) {
            countMap[input[i]]++;
        }
        return countMap;
    }

    // 前向声明(精确分布,定义在 kruskal 前;MW/Wilcoxon 在其之前调用故需先声明)
    static double mannWhitneyExactTwoSided(int n1, int n2, double u_obs);
    static double wilcoxonExactTwoSided(const double* abs_ranks, int n, double w_obs);

    // Mann-Whitney U检验（两独立样本，C风格数组输入）
    // 参数：
    //   sample1/sample2: 样本数组（double*）
    //   len1/len2: 样本长度（int）
    //   two_side: 是否双侧检验（bool，默认true）
    //   is_less: 单侧检验方向（true=sample1 < sample2，false=sample1 > sample2，仅two_side=false时生效）
    //   min_is_better: 优化方向（true=越小越好，false=越大越好，默认true）
    // 返回值：U统计量、p值（std::pair<double, double>）
    inline std::pair<double, double> mann_whitney_u_test(
        double* sample1, int len1,
        double* sample2, int len2,
        bool two_side = true,
        bool is_less = true,
        bool min_is_better = true) {

        // 输入校验
        if (len1 <= 0 || len2 <= 0 || sample1 == nullptr || sample2 == nullptr) {
            throw std::invalid_argument("Mann-Whitney U检验：样本为空或长度≤0");
        }
        if (len1 > 10000 || len2 > 10000) {
            throw std::invalid_argument("Mann-Whitney U检验：样本量过大（建议≤10000）");
        }

        // 合并样本
        int total_len = len1 + len2;
        double* merged = new double[total_len];
        memcpy(merged, sample1, sizeof(double) * len1);
        memcpy(merged + len1, sample2, sizeof(double) * len2);

        // 计算秩（支持优化方向）
        double* ranks = rankElements(merged, total_len, min_is_better);

        // 计算两组秩和
        double R1 = 0.0, R2 = 0.0;
        for (int i = 0; i < len1; ++i) R1 += ranks[i];
        for (int i = len1; i < total_len; ++i) R2 += ranks[i];

        // 计算U统计量
        double U1 = R1 - len1 * (len1 + 1) / 2.0;
        double U2 = len1 * len2 - U1;
        double U = (two_side) ? std::min(U1, U2) : (is_less ? U2 : U1);

        // 正态近似计算Z值（含结值修正）
        double mean_U = len1 * len2 / 2.0;
        double var_U = len1 * len2 * (total_len + 1) / 12.0;

        // 结值修正
        double tie_term = 0.0;
        std::map<double, int> count_map = appearCount(merged, total_len);
        for (auto& pair : count_map) {
            int t = pair.second;
            tie_term += t * (t * t - 1);
        }
        double tie_correction = 1.0 - tie_term / (total_len * (total_len * total_len - 1));
        var_U *= tie_correction;

        // 计算p值:小样本无结值用【精确分布】(对齐 scipy exact),否则正态近似(连续性校正)
        double p_value;
        if (two_side && tie_term == 0.0 && len1 <= 20 && len2 <= 20) {
            p_value = mannWhitneyExactTwoSided(len1, len2, std::min(U1, U2));
        } else {
            double numerator = std::abs(U - mean_U) - 0.5;
            double z = numerator / std::sqrt(var_U);
            p_value = 0.5 * std::erfc(z / std::sqrt(2));
            if (two_side) p_value *= 2;
            if (p_value > 1) p_value = 1;
        }

        // 释放内存
        delete[] merged;
        delete[] ranks;

        return { U, p_value };
    }

    // Wilcoxon符号秩检验（配对样本，C风格数组输入）
    // 参数：
    //   sample1/sample2: 配对样本数组（double*）
    //   len: 样本长度（int）
    //   two_side: 是否双侧检验（bool，默认true）
    //   is_less: 单侧检验方向（true=sample1 < sample2，false=sample1 > sample2，仅two_side=false时生效）
    // 返回值：W统计量、p值（std::pair<double, double>）
    inline std::pair<double, double> wilcoxon_signed_rank_test(
        double* sample1, double* sample2, int len,
        bool two_side = true,
        bool is_less = true) {

        // 输入校验
        if (len <= 0 || sample1 == nullptr || sample2 == nullptr) {
            throw std::invalid_argument("Wilcoxon检验：样本为空或长度≤0");
        }

        // 计算差值并过滤0值
        int diff_len = 0;
        double* diffs = new double[len];
        for (int i = 0; i < len; ++i) {
            double diff = sample1[i] - sample2[i];
            if (std::abs(diff) > 1e-8) {
                diffs[diff_len++] = diff;
            }
        }
        if (diff_len == 0) {
            delete[] diffs;
            throw std::invalid_argument("Wilcoxon检验：所有配对差值均为0，无法检验");
        }

        // 计算绝对差值并排序
        double* abs_diffs = new double[diff_len];
        int* signs = new int[diff_len];
        for (int i = 0; i < diff_len; ++i) {
            abs_diffs[i] = std::abs(diffs[i]);
            signs[i] = (diffs[i] > 0) ? 1 : -1;
        }

        // 计算绝对差值的秩
        double* abs_ranks = rankElements(abs_diffs, diff_len);

        // 计算符号秩
        double pos_sum = 0.0, neg_sum = 0.0;
        for (int i = 0; i < diff_len; ++i) {
            double signed_rank = abs_ranks[i] * signs[i];
            if (signed_rank > 0) {
                pos_sum += signed_rank;
            }
            else if (signed_rank < 0) {
                neg_sum += std::abs(signed_rank);
            }
        }

        // 计算W统计量
        double W = (two_side) ? std::min(pos_sum, neg_sum) : (is_less ? neg_sum : pos_sum);

        // 正态近似计算Z值（含结值修正）
        double mean_W = diff_len * (diff_len + 1) / 4.0;
        double var_W = diff_len * (diff_len + 1) * (2 * diff_len + 1) / 24.0;

        // 结值修正
        double tie_term = 0.0;
        std::map<double, int> count_map = appearCount(abs_diffs, diff_len);
        for (auto& pair : count_map) {
            int t = pair.second;
            tie_term += t * (t * t - 1);
        }
        double tie_correction = 1.0 - tie_term / (diff_len * (diff_len * diff_len - 1));
        var_W *= tie_correction;

        // 计算p值:小样本用【精确分布】(秩×2 缩放子集和 DP,含结值也精确,对齐 scipy exact);否则正态近似
        double p_value;
        if (two_side && diff_len <= 20) {
            p_value = wilcoxonExactTwoSided(abs_ranks, diff_len, W);
        } else {
            double numerator = std::abs(W - mean_W) - 0.5;
            double z = numerator / std::sqrt(var_W);
            p_value = 0.5 * std::erfc(z / std::sqrt(2));
            if (two_side) p_value *= 2;
            if (p_value > 1) p_value = 1;
        }

        // 释放内存
        delete[] diffs;
        delete[] abs_diffs;
        delete[] signs;
        delete[] abs_ranks;

        return { W, p_value };
    }

    // Kruskal-Wallis H检验（多独立样本，C风格数组输入）
    // 参数：
    //   samples: 样本集合（double**，每个元素是一个样本数组）
    //   lens: 每个样本的长度（int*）
    //   group_num: 样本组数（int）
    //   two_side: 是否双侧检验（bool，默认true）
    // 返回值：H统计量、p值（std::pair<double, double>）
    // 正则化下不完全伽马 P(a,x)=γ(a,x)/Γ(a)(Numerical Recipes:级数/连分数)。
    static double regularizedGammaP(double a, double x) {
        if (x <= 0.0 || a <= 0.0) return 0.0;
        double lg = std::lgamma(a);
        if (x < a + 1.0) {                     // 级数展开(P)
            double ap = a, del = 1.0 / a, sum = del;
            for (int n = 0; n < 300; ++n) {
                ap += 1.0; del *= x / ap; sum += del;
                if (std::abs(del) < std::abs(sum) * 1e-16) break;
            }
            return sum * std::exp(-x + a * std::log(x) - lg);
        } else {                               // 连分数(Q=1-P)
            double b = x + 1.0 - a, c = 1e300, d = 1.0 / b, h = d;
            for (int i = 1; i <= 300; ++i) {
                double an = -1.0 * i * (i - a);
                b += 2.0; d = an * d + b; if (std::abs(d) < 1e-300) d = 1e-300;
                c = b + an / c; if (std::abs(c) < 1e-300) c = 1e-300;
                d = 1.0 / d; double del = d * c; h *= del;
                if (std::abs(del - 1.0) < 1e-16) break;
            }
            return 1.0 - std::exp(-x + a * std::log(x) - lg) * h;
        }
    }
    // 卡方生存函数 chi2.sf(x, df) = Q(df/2, x/2)(对齐 scipy.stats.chi2.sf)。
    static double chiSquareSurvival(double x, int df) {
        if (x <= 0.0) return 1.0;
        return 1.0 - regularizedGammaP(df / 2.0, x / 2.0);
    }

    // 精确 Mann-Whitney U 双侧 p(无结值时;经典递推 p(m,n,u)=p(m-1,n,u-n)+p(m,n-1,u),对齐 scipy exact)。
    static double mannWhitneyExactTwoSided(int n1, int n2, double u_obs) {
        int Umax = n1 * n2;
        std::vector<std::vector<std::vector<double>>> dp(n1 + 1, std::vector<std::vector<double>>(n2 + 1));
        for (int m = 0; m <= n1; ++m)
            for (int n = 0; n <= n2; ++n) {
                dp[m][n].assign(m * n + 1, 0.0);
                if (m == 0 || n == 0) { dp[m][n][0] = 1.0; continue; }
                for (int u = 0; u <= m * n; ++u) {
                    double a = (u - n >= 0) ? dp[m - 1][n][u - n] : 0.0;
                    double b = (u <= m * (n - 1)) ? dp[m][n - 1][u] : 0.0;
                    dp[m][n][u] = a + b;
                }
            }
        double total = 0; for (double c : dp[n1][n2]) total += c;
        int uo = (int)std::round(u_obs);
        double cum = 0; for (int u = 0; u <= uo && u <= Umax; ++u) cum += dp[n1][n2][u];
        return std::min(1.0, 2.0 * cum / total);   // 双侧 = 2×下尾
    }

    // 精确 Wilcoxon 符号秩双侧 p(小样本;秩×2 缩放使含结的平均秩变整数 → 子集和 DP,对齐 scipy exact)。
    static double wilcoxonExactTwoSided(const double* abs_ranks, int n, double w_obs) {
        int Wmax2 = 0; std::vector<int> r2(n);
        for (int i = 0; i < n; ++i) { r2[i] = (int)std::round(abs_ranks[i] * 2.0); Wmax2 += r2[i]; }
        std::vector<double> dp(Wmax2 + 1, 0.0); dp[0] = 1.0;
        for (int i = 0; i < n; ++i)
            for (int w = Wmax2; w >= r2[i]; --w) dp[w] += dp[w - r2[i]];
        double total = std::pow(2.0, n);
        int wo = (int)std::round(w_obs * 2.0);
        double cum = 0; for (int w = 0; w <= wo && w <= Wmax2; ++w) cum += dp[w];
        return std::min(1.0, 2.0 * cum / total);
    }

    inline std::pair<double, double> kruskal_wallis_h_test(
        double** samples, int* lens, int group_num,
        bool two_side = true) {

        // 输入校验
        if (group_num < 2 || samples == nullptr || lens == nullptr) {
            throw std::invalid_argument("Kruskal-Wallis检验：至少需要2个样本");
        }
        for (int i = 0; i < group_num; ++i) {
            if (lens[i] <= 0 || samples[i] == nullptr) {
                throw std::invalid_argument("Kruskal-Wallis检验：样本为空或长度≤0");
            }
        }

        // 合并所有样本并记录所属组
        int total_len = 0;
        for (int i = 0; i < group_num; ++i) total_len += lens[i];
        double* merged = new double[total_len];
        int* group_marks = new int[total_len];

        int pos = 0;
        for (int g = 0; g < group_num; ++g) {
            memcpy(merged + pos, samples[g], sizeof(double) * lens[g]);
            for (int i = 0; i < lens[g]; ++i) {
                group_marks[pos + i] = g;
            }
            pos += lens[g];
        }

        // 计算秩
        double* ranks = rankElements(merged, total_len);

        // 计算每组秩和
        double* group_R = new double[group_num]();
        int* group_n = new int[group_num]();
        for (int i = 0; i < total_len; ++i) {
            int g = group_marks[i];
            group_R[g] += ranks[i];
            group_n[g]++;
        }

        // 计算H统计量
        double H = 0.0;
        for (int g = 0; g < group_num; ++g) {
            H += (group_R[g] * group_R[g]) / group_n[g];
        }
        H = (12.0 / (total_len * (total_len + 1))) * H - 3 * (total_len + 1);

        // 结值修正
        double tie_term = 0.0;
        std::map<double, int> count_map = appearCount(merged, total_len);
        for (auto& pair : count_map) {
            int t = pair.second;
            tie_term += t * (t * t - 1);
        }
        double tie_correction = 1.0 - tie_term / (total_len * (total_len * total_len - 1));
        H /= tie_correction;
        if (H < 0) H = 0;   // 浮点误差致微负(同分布组 H≈0);守卫 pow(负,1/3)=NaN

        // p 值 = 卡方生存函数(自由度=组数-1;KW 为卡方上尾【单侧】综合)。对齐 scipy.stats.kruskal 的 chi2.sf。
        (void)two_side;
        int df = group_num - 1;
        double p_value = chiSquareSurvival(H, df);

        // 释放内存
        delete[] merged;
        delete[] group_marks;
        delete[] ranks;
        delete[] group_R;
        delete[] group_n;

        return { H, p_value };
    }

    // Friedman 检验(K 处理 × N 区块的秩基综合检验,对齐 scipy.stats.friedmanchisquare)。
    //   data[i][j] = 处理 i(算法) 在区块 j(问题) 的代表值。返回 {χ², p}(方向无关)。
    inline std::pair<double, double> friedman_test(double** data, int k, int n) {
        if (k < 2 || n < 1 || data == nullptr)
            throw std::invalid_argument("Friedman检验：需 k>=2 处理、n>=1 区块");
        std::vector<double> R(k, 0.0);      // 各处理的秩和
        double ties = 0.0;                  // 结值项 Σ(t³-t)
        std::vector<double> block(k);
        for (int j = 0; j < n; ++j) {
            for (int i = 0; i < k; ++i) block[i] = data[i][j];
            double* r = rankElements(block.data(), k);   // 块内 1 基秩(含结平均)
            for (int i = 0; i < k; ++i) R[i] += r[i];
            delete[] r;
            std::map<double, int> cnt = appearCount(block.data(), k);
            for (auto& pr : cnt) { double t = pr.second; ties += t * t * t - t; }
        }
        double ssbn = 0.0; for (double r : R) ssbn += r * r;
        double chi = 12.0 / (n * k * (k + 1.0)) * ssbn - 3.0 * n * (k + 1.0);
        double c = 1.0 - ties / (n * (double)(k * k * k - k));   // scipy 结值校正
        if (c > 0.0) chi /= c;
        double p = chiSquareSurvival(chi, k - 1);
        return { chi, p };
    }

    // Nemenyi 临界差 CD(Friedman 后验;k 处理、n 区块)。CD=q_α·√(k(k+1)/(6n))。秩差>CD 即成对显著。
    //   q_α 表(Demšar 2006,α=0.05,k=2..20);目前仅 α=0.05。
    static double nemenyiCriticalDifference(int k, int n, double alpha = 0.05) {
        static const double q05[] = { 0, 0, 1.960, 2.343, 2.569, 2.728, 2.850, 2.949, 3.031, 3.102,
                                      3.164, 3.219, 3.268, 3.313, 3.354, 3.391, 3.426, 3.458, 3.489, 3.517, 3.544 };
        (void)alpha;
        double q = (k >= 2 && k <= 20) ? q05[k] : 3.544;
        return q * std::sqrt((double)k * (k + 1) / (6.0 * n));
    }

    // Dunn 后验(KW 后,每问题):baseline(组 0) vs 各组 → Bonferroni(K-1)校正双侧 p。
    //   pooled 秩 + 结值校正 σ²=N(N+1)/12 − Σ(t³-t)/(12(N-1));对齐 scikit-posthocs 未校正 p。padj[0]=NaN。
    static std::vector<double> dunn_baseline_bonferroni(double** samples, int* lens, int group_num) {
        std::vector<double> padj(group_num, std::numeric_limits<double>::quiet_NaN());
        if (group_num < 2 || samples == nullptr || lens == nullptr) return padj;
        int N = 0; for (int g = 0; g < group_num; ++g) N += lens[g];
        if (N < 2) return padj;
        std::vector<double> pooled(N); std::vector<int> mark(N);
        int pos = 0;
        for (int g = 0; g < group_num; ++g) for (int i = 0; i < lens[g]; ++i) { pooled[pos] = samples[g][i]; mark[pos] = g; ++pos; }
        double* ranks = rankElements(pooled.data(), N);
        std::vector<double> Rsum(group_num, 0.0);
        for (int i = 0; i < N; ++i) Rsum[mark[i]] += ranks[i];
        delete[] ranks;
        std::vector<double> Rbar(group_num);
        for (int g = 0; g < group_num; ++g) Rbar[g] = Rsum[g] / lens[g];
        double tie = 0.0; std::map<double, int> cnt = appearCount(pooled.data(), N);
        for (auto& pr : cnt) { double t = pr.second; tie += t * t * t - t; }
        double sigma2 = N * (N + 1.0) / 12.0 - tie / (12.0 * (N - 1));
        int comparisons = group_num - 1;
        for (int g = 1; g < group_num; ++g) {
            double se = std::sqrt(sigma2 * (1.0 / lens[0] + 1.0 / lens[g]));
            double z = (Rbar[0] - Rbar[g]) / se;
            double p = std::erfc(std::abs(z) / std::sqrt(2.0));   // 双侧
            padj[g] = std::min(1.0, p * comparisons);             // Bonferroni(K-1)
        }
        return padj;
    }

} // namespace MetricLib

#endif // NONPARAM_HPP