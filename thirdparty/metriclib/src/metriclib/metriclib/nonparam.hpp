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
                    ranks[indices[i]] = i;
                }
                else {
                    end_rank = i;
                    average_rank = (begin_rank + end_rank) / 2.0;
                    for (int k = begin_rank; k <= end_rank; ++k) {
                        ranks[indices[k]] = average_rank;
                    }
                    is_begin = true;
                }
            }
        }

        // 处理最后一个元素/最后一段结值
        if (is_begin) {
            ranks[indices[size - 1]] = size - 1;
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

    // Mann-Whitney U检验（两独立样本，C风格数组输入）
    // 参数：
    //   sample1/sample2: 样本数组（double*）
    //   len1/len2: 样本长度（int）
    //   two_side: 是否双侧检验（bool，默认true）
    //   is_less: 单侧检验方向（true=sample1 < sample2，false=sample1 > sample2，仅two_side=false时生效）
    //   min_is_better: 优化方向（true=越小越好，false=越大越好，默认true）
    // 返回值：U统计量、p值（std::pair<double, double>）
    std::pair<double, double> mann_whitney_u_test(
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

        // 连续性修正
        double numerator = std::abs(U - mean_U) - 0.5;
        double z = numerator / std::sqrt(var_U);

        // 计算p值（支持单侧/双侧）
        double p_value = 0.5 * std::erfc(z / std::sqrt(2));
        if (two_side) {
            p_value *= 2;
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
    std::pair<double, double> wilcoxon_signed_rank_test(
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

        // 连续性修正
        double numerator = std::abs(W - mean_W) - 0.5;
        double z = numerator / std::sqrt(var_W);

        // 计算p值（支持单侧/双侧）
        double p_value = 0.5 * std::erfc(z / std::sqrt(2));
        if (two_side) {
            p_value *= 2;
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
    std::pair<double, double> kruskal_wallis_h_test(
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

        // 卡方近似计算p值（自由度=组数-1）
        int df = group_num - 1;
        double chi = H;
        double z = (std::pow(chi / df, 1.0 / 3.0) - (1.0 - 2.0 / (9.0 * df))) / std::sqrt(2.0 / (9.0 * df));
        double p_value = 1 - 0.5 * (1 + std::erf(z / std::sqrt(2)));
        if (two_side) {
            p_value *= 2;
        }

        // 释放内存
        delete[] merged;
        delete[] group_marks;
        delete[] ranks;
        delete[] group_R;
        delete[] group_n;

        return { H, p_value };
    }

} // namespace MetricLib

#endif // NONPARAM_HPP