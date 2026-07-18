#pragma once
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <cstring>

#include "func.hpp"

namespace MetricLib
{
    /**
     * @brief 计算峰值比率（Peak Ratio）：评估算法找到的解覆盖参考峰值的比例
     * @param input 算法输出的解集合（二维数组，input[size][demension]，每行一个解）
     * @param refer_point 参考峰值的坐标集合（二维数组，refer_point[refer_size][demension]，每行一个峰值）
     * @param size 算法输出的解的数量
     * @param refer_size 参考峰值的数量
     * @param demension 解/峰值的维度
     * @param accuracy_d 距离精度阈值（小于该值认为解覆盖对应峰值）
     * @return 峰值覆盖率（覆盖峰值数 / 参考峰值总数）
     */
    double peakRatio(const double** input, const double** refer_point, int size, int refer_size, int demension, double accuracy_d = 1e-3)
    {
        // 入参合法性校验
        if (input == nullptr || refer_point == nullptr || size <= 0 || refer_size <= 0 || demension <= 0) {
            return 0.0; // 非法输入返回0
        }

        double distance = 0.0;
        int counter = 0; // 被覆盖的参考峰值数量

        // 遍历每个参考峰值，检查是否被算法解覆盖
        for (int i = 0; i < refer_size; ++i)
        {
            const double* aim_pointer = refer_point[i]; // 第i个参考峰值的地址（二维数组行地址）
            bool is_covered = false;

            for (int j = 0; j < size; ++j)
            {
                const double* solution_pointer = input[j]; // 第j个算法解的地址（二维数组行地址）
                distance = eu_distance(const_cast<double*>(solution_pointer), const_cast<double*>(aim_pointer), demension);

                if (distance < accuracy_d) {
                    is_covered = true;
                    break; // 该峰值已覆盖，无需检查其他解
                }
            }

            if (is_covered) {
                counter++;
            }
        }

        return static_cast<double>(counter) / refer_size;
    }

    /**
     * @brief 计算峰值比率（带适应度筛选）：先按适应度筛选解，再计算覆盖的峰值数
     * @param input_fitness 算法输出解的适应度数组（一维数组，input_fitness[size]，每个元素对应一个解的适应度）
     * @param input_decision 算法输出解的决策变量集合（二维数组，input_decision[size][demension]，每行一个解）
     * @param refer_fitness 参考适应度（筛选解的阈值）
     * @param size 算法输出的解的数量
     * @param refer_point 参考峰值的坐标集合（二维数组，refer_point[refer_size][demension]，每行一个峰值）
     * @param refer_size 参考峰值的数量
     * @param demension 解/峰值的维度
     * @param accuracy_f 适应度精度阈值（与参考适应度差值小于该值则保留）
     * @param accuracy_d 距离精度阈值（小于该值认为解覆盖对应峰值）
     * @return 峰值覆盖率（去重后覆盖峰值数 / 参考峰值总数）
     */
    double peakRatio(const double* input_fitness, const double** input_decision, int size, double refer_fitness,
        const double** refer_point, int refer_size, int demension,
        double accuracy_f = 1e-3, double accuracy_d = 1e-3)
    {
        // 入参合法性校验
        if (input_fitness == nullptr || input_decision == nullptr || refer_point == nullptr ||
            size <= 0 || refer_size <= 0 || demension <= 0) {
            return 0.0;
        }

        // 判断适应度优化方向：参考适应度 < 第一个解的适应度 → 越小越好
        bool min_is_better = (refer_fitness < input_fitness[0]);

        // 标记解是否保留（初始为false）
        bool* inlist = new bool[size];
        std::memset(inlist, 0, size * sizeof(bool));
        int left_size = 0; // 保留的解的数量

        // 第一步：按适应度筛选解
        for (int i = 0; i < size; ++i)
        {
            inlist[i] = (std::abs(input_fitness[i] - refer_fitness) < accuracy_f);
            if (inlist[i]) {
                left_size++;
            }
        }

        int counter = 0; // 去重后覆盖的峰值数
        int index = 0;
        int best_id = 0;
        double best_fitness = 0.0;

        // 第二步：对保留的解去重（距离小于阈值视为同一峰值）
        while (left_size > 0)
        {
            counter++;

            // 找到第一个未被排除的解
            index = 0;
            while (index < size && !inlist[index]) {
                index++;
            }

            // 找到保留解中适应度最优的解（根据min_is_better判断）
            best_id = index;
            best_fitness = input_fitness[best_id];
            for (index += 1; index < size; ++index)
            {
                if (!inlist[index]) {
                    continue;
                }
                // 适应度比较：^ 为异或，实现“越小越好/越大越好”的统一判断
                if ((best_fitness < input_fitness[index]) ^ min_is_better)
                {
                    best_id = index;
                    best_fitness = input_fitness[best_id];
                }
            }

            // 排除与最优解距离过近的解（视为同一峰值）
            const double* best_pointer = input_decision[best_id];
            inlist[best_id] = false;
            left_size--;

            for (index = 0; index < size; ++index)
            {
                if (!inlist[index]) {
                    continue;
                }
                const double* curr_pointer = input_decision[index];
                if (eu_distance(const_cast<double*>(best_pointer), const_cast<double*>(curr_pointer), demension) < accuracy_d)
                {
                    inlist[index] = false;
                    left_size--;
                }
            }
        }

        // 释放内存
        delete[] inlist;
        inlist = nullptr;

        return static_cast<double>(counter) / refer_size;
    }

    /**
     * @brief 计算峰值距离（Peak Distance）：算法解到最近参考峰值的平均距离
     * @param input 算法输出的解集合（二维数组，input[size][demension]，每行一个解）
     * @param refer_point 参考峰值的坐标集合（二维数组，refer_point[refer_size][demension]，每行一个峰值）
     * @param size 算法输出的解的数量
     * @param refer_size 参考峰值的数量
     * @param demension 解/峰值的维度
     * @return 平均峰值距离（值越小，解越接近参考峰值）
     */
    double peakDistance(const double** input, const double** refer_point, int size, int refer_size, int demension)
    {
        // 入参合法性校验
        if (input == nullptr || refer_point == nullptr || size <= 0 || refer_size <= 0 || demension <= 0) {
            return 0.0;
        }

        double total_dist = 0.0;
        double distance_min = 0.0;
        double distance_buffer = 0.0;

        // 遍历每个算法解，计算到最近参考峰值的距离
        for (int i = 0; i < size; ++i)
        {
            const double* solution_pointer = input[i]; // 第i个解的地址
            // 初始化为到第一个参考峰值的距离
            distance_min = eu_distance(const_cast<double*>(refer_point[0]), const_cast<double*>(solution_pointer), demension);

            // 遍历所有参考峰值，找到最小距离
            for (int j = 1; j < refer_size; ++j)
            {
                const double* aim_pointer = refer_point[j];
                distance_buffer = eu_distance(const_cast<double*>(solution_pointer), const_cast<double*>(aim_pointer), demension);
                if (distance_buffer < distance_min) {
                    distance_min = distance_buffer;
                }
            }

            total_dist += distance_min;
        }

        // 计算平均距离
        return total_dist / static_cast<double>(size);
    }

    /**
     * @brief 计算峰值不准确度（Peak Inaccuracy）：算法解的适应度与最近参考峰值适应度的平均差值
     * @param input_fitness 算法输出解的适应度数组（一维数组，input_fitness[size]，每个元素对应一个解的适应度）
     * @param input_decision 算法输出解的决策变量集合（二维数组，input_decision[size][demension]，每行一个解）
     * @param refer_fitness 参考峰值的适应度数组（一维数组，refer_fitness[refer_size]，每个元素对应一个峰值的适应度）
     * @param refer_decision 参考峰值的决策变量集合（二维数组，refer_decision[refer_size][demension]，每行一个峰值）
     * @param size 算法输出的解的数量
     * @param refer_size 参考峰值的数量
     * @param demension 解/峰值的维度
     * @return 平均峰值不准确度（值越小，解的适应度越接近参考峰值）
     */
    double peakInaccuracy(const double* input_fitness, const double** input_decision,
        const double* refer_fitness, const double** refer_decision,
        int size, int refer_size, int demension)
    {
        // 入参合法性校验
        if (input_fitness == nullptr || input_decision == nullptr || refer_fitness == nullptr ||
            refer_decision == nullptr || size <= 0 || refer_size <= 0 || demension <= 0) {
            return 0.0;
        }

        double total_inaccuracy = 0.0;
        int min_id = 0;
        double distance_min = 0.0;
        double distance_buffer = 0.0;

        // 遍历每个算法解，找到最近的参考峰值，计算适应度差值
        for (int i = 0; i < size; ++i)
        {
            const double* solution_pointer = input_decision[i]; // 第i个解的决策变量
            // 初始化为到第一个参考峰值的距离
            distance_min = eu_distance(const_cast<double*>(refer_decision[0]), const_cast<double*>(solution_pointer), demension);
            min_id = 0;

            // 找到最近的参考峰值
            for (int j = 1; j < refer_size; ++j)
            {
                const double* aim_pointer = refer_decision[j];
                distance_buffer = eu_distance(const_cast<double*>(solution_pointer), const_cast<double*>(aim_pointer), demension);
                if (distance_buffer < distance_min)
                {
                    min_id = j;
                    distance_min = distance_buffer;
                }
            }

            // 累加适应度绝对差值
            total_inaccuracy += std::abs(refer_fitness[min_id] - input_fitness[i]);
        }

        // 计算平均不准确度
        return total_inaccuracy / static_cast<double>(size);
    }
}