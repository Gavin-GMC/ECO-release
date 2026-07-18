#pragma once
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include "func.hpp"

namespace MetricLib {

    /**
     * @brief 判断解x是否被解y支配（支持逐目标/整体设置最小化/最大化）
     * @param x 待判断的解x [dim]
     * @param y 解y [dim]
     * @param dim 目标维度
     * @param min_is_better 逐目标的优化方向：true=该目标越小越好，false=越大越好 [dim]（优先使用）
     * @param global_min_is_better 整体优化方向（仅当min_is_better为nullptr时生效），默认true（所有目标最小化）
     * @return true=x被y支配，false=x未被y支配
     * 支配规则：
     * - 最小化目标：y ≤ x；最大化目标：y ≥ x
     * - 所有目标满足上述条件，且至少一个目标严格满足（最小化：y < x；最大化：y > x）
     */
    inline bool isDominated(const double* x, const double* y, int dim,
        const bool* min_is_better = nullptr,
        bool global_min_is_better = true) {
        bool allSatisfy = true;  // 所有目标满足y的性能≥x（最小化：y≤x；最大化：y≥x）
        bool anyStrict = false;  // 至少一个目标严格满足（最小化：y<x；最大化：y>x）

        for (int i = 0; i < dim; ++i) {
            // 确定当前目标的优化方向
            bool curr_min_better = (min_is_better != nullptr) ? min_is_better[i] : global_min_is_better;

            if (curr_min_better) {
                // 最小化目标：y > x → 不满足支配；y < x → 严格满足
                if (y[i] > x[i]) {
                    allSatisfy = false;
                    break;
                }
                if (y[i] < x[i]) {
                    anyStrict = true;
                }
            }
            else {
                // 最大化目标：y < x → 不满足支配；y > x → 严格满足
                if (y[i] < x[i]) {
                    allSatisfy = false;
                    break;
                }
                if (y[i] > x[i]) {
                    anyStrict = true;
                }
            }
        }

        return allSatisfy && anyStrict;
    }

    /**
     * @brief 判断解x是否被解集Y中的至少一个解支配（支持逐目标/整体优化方向）
     * @param x 待判断的解x [dim]
     * @param Y 解集Y [Y_size][dim]
     * @param Y_size 解集Y的大小
     * @param dim 目标维度
     * @param min_is_better 逐目标的优化方向：true=该目标越小越好，false=越大越好 [dim]（优先使用）
     * @param global_min_is_better 整体优化方向（仅当min_is_better为nullptr时生效），默认true（所有目标最小化）
     * @return true=x被Y中至少一个解支配，false=x未被支配
     */
    inline bool isDominatedBySet(const double* x, double** Y, int Y_size, int dim,
        const bool* min_is_better = nullptr,
        bool global_min_is_better = true) {
        for (int i = 0; i < Y_size; ++i) {
            if (isDominated(x, Y[i], dim, min_is_better, global_min_is_better)) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 世代距离 (GD) - 衡量解集逼近真实前沿的程度（越小越好）
     * @param input 待评估解集 [size][object_number]（double**）
     * @param refer_point 参考前沿解集 [refer_size][object_number]（double**）
     * @param size 待评估解集大小
     * @param refer_size 参考前沿解集大小
     * @param object_number 目标维度
     * @return GD值
     */
    double gd(double** input, double** refer_point, int size, int refer_size, int object_number) {
        if (size == 0 || refer_size == 0) return 0.0;

        double total_dist = 0.0;
        double min_dist, curr_dist;

        for (int i = 0; i < size; ++i) {
            // 初始化最小距离为当前点到参考前沿第一个点的距离
            min_dist = eu_distance(input[i], refer_point[0], object_number);
            // 遍历参考前沿所有点找最小距离
            for (int j = 1; j < refer_size; ++j) {
                curr_dist = eu_distance(input[i], refer_point[j], object_number);
                if (curr_dist < min_dist) {
                    min_dist = curr_dist;
                }
            }
            total_dist += min_dist;
        }

        return total_dist / size;
    }

    /**
     * @brief 反世代距离 (IGD) - 衡量参考前沿被解集覆盖的程度（越小越好）
     * @param input 待评估解集 [size][object_number]（double**）
     * @param refer_point 参考前沿解集 [refer_size][object_number]（double**）
     * @param size 待评估解集大小
     * @param refer_size 参考前沿解集大小
     * @param object_number 目标维度
     * @return IGD值
     */
    double igd(double** input, double** refer_point, int size, int refer_size, int object_number) {
        if (size == 0 || refer_size == 0) return 0.0;

        double total_dist = 0.0;
        double min_dist, curr_dist;

        for (int i = 0; i < refer_size; ++i) {
            // 初始化最小距离为参考点到解集第一个点的距离
            min_dist = eu_distance(refer_point[i], input[0], object_number);
            // 遍历解集所有点找最小距离
            for (int j = 1; j < size; ++j) {
                curr_dist = eu_distance(refer_point[i], input[j], object_number);
                if (curr_dist < min_dist) {
                    min_dist = curr_dist;
                }
            }
            total_dist += min_dist;
        }

        return total_dist / refer_size;
    }

    /**
     * @brief HV计算核心迭代函数（适配double**输入）
     * @param input 点集 [size][demension]（double**）
     * @param size 点集大小
     * @param demension 目标维度
     * @return 超体积值
     */
    double _hv_iter(double** input, int size, int demension) {
        if (demension == 1) {
            double max_val = input[0][0];
            for (int i = 0; i < size; ++i) {
                max_val = std::max(max_val, input[i][0]);
            }
            return max_val;
        }
        else {
            double hv = 0.0;
            // 构建降维后的点集（demension-1维）
            double** buffer = new double* [size];
            for (int i = 0; i < size; ++i) {
                buffer[i] = new double[demension - 1];
            }

            sortHelper<int, double>* index_buffer = new sortHelper<int, double>[size];

            // 按第一维排序，关联索引
            for (int i = 0; i < size; ++i) {
                index_buffer[i].id = i;
                index_buffer[i].value = input[i][0]; // 第一维值
            }
            std::sort(index_buffer, index_buffer + size);

            // 构建降维后的点集（去掉第一维）
            int new_dim = demension - 1;
            for (int i = 0; i < size; ++i) {
                int orig_idx = index_buffer[i].id;
                for (int d = 0; d < new_dim; ++d) {
                    buffer[i][d] = input[orig_idx][d + 1]; // 取第2~demension维
                }
            }

            // 迭代计算超体积
            int curr_idx = 0;
            double curr_h = 0.0;
            while (curr_idx != size) {
                if (index_buffer[curr_idx].value <= curr_h) {
                    curr_idx++;
                    continue;
                }
                hv += (index_buffer[curr_idx].value - curr_h) * _hv_iter(buffer + curr_idx, size - curr_idx, new_dim);
                curr_h = index_buffer[curr_idx].value;
            }

            // 释放内存
            delete[] index_buffer;
            for (int i = 0; i < size; ++i) {
                delete[] buffer[i];
            }
            delete[] buffer;

            return hv;
        }
    }

    /**
     * @brief 基于精确计算的超体积 (HV)
     * @param input 解集 [size][object_number]（double**）
     * @param refer_point 参考点 [object_number]（double*）
     * @param size 解集大小
     * @param object_number 目标维度
     * @return 超体积值
     */
    double hv_math(double** input, double* refer_point, int size, int object_number) {
        if (size == 0 || object_number == 0) return 0.0;

        double hv_total = 0.0;

        // 步骤1：平移到参考点为原点的坐标系，生成新的点集
        double** shifted_input = new double* [size];
        int* belongs = new int[size](); // 记录每个点所属象限
        for (int i = 0; i < size; ++i) {
            shifted_input[i] = new double[object_number];
            int quadrant = 0;
            for (int o = 0; o < object_number; ++o) {
                shifted_input[i][o] = input[i][o] - refer_point[o];
                // 计算象限并处理负坐标
                quadrant *= 2;
                if (shifted_input[i][o] < 0) {
                    quadrant++;
                    shifted_input[i][o] *= -1; // 负坐标取反
                }
            }
            belongs[i] = quadrant;
        }

        // 步骤2：按象限计算超体积并累加
        int quadrants = static_cast<int>(std::pow(2, object_number));
        for (int b = 0; b < quadrants; ++b) {
            // 收集当前象限的所有点
            int counter = 0;
            for (int i = 0; i < size; ++i) {
                if (belongs[i] == b) counter++;
            }
            if (counter == 0) continue;

            // 构建当前象限的点集
            double** quad_points = new double* [counter];
            int idx = 0;
            for (int i = 0; i < size; ++i) {
                if (belongs[i] == b) {
                    quad_points[idx] = new double[object_number];
                    for (int o = 0; o < object_number; ++o) {
                        quad_points[idx][o] = shifted_input[i][o];
                    }
                    idx++;
                }
            }

            // 计算当前象限的超体积并累加
            hv_total += _hv_iter(quad_points, counter, object_number);

            // 释放当前象限点集内存
            for (int i = 0; i < counter; ++i) {
                delete[] quad_points[i];
            }
            delete[] quad_points;
        }

        // 释放平移后的点集内存
        for (int i = 0; i < size; ++i) {
            delete[] shifted_input[i];
        }
        delete[] shifted_input;
        delete[] belongs;

        return hv_total;
    }

    /**
     * @brief 基于蒙特卡洛采样的超体积 (HV)
     * @param input 解集 [size][object_number]（double**）
     * @param refer_point 参考点 [object_number]（double*）
     * @param size 解集大小
     * @param object_number 目标维度
     * @return 超体积估计值
     */
    double hv_montecarlo(double** input, double* refer_point, int size, int object_number) {
        if (size == 0 || object_number == 0) return 0.0;

        // 步骤1：计算采样空间的上下界
        double* o_max = new double[object_number];
        double* o_min = new double[object_number];
        for (int o = 0; o < object_number; ++o) {
            o_min[o] = refer_point[o];
            o_max[o] = refer_point[o];
        }

        for (int i = 0; i < size; ++i) {
            for (int o = 0; o < object_number; ++o) {
                o_min[o] = std::min(o_min[o], input[i][o]);
                o_max[o] = std::max(o_max[o], input[i][o]);
            }
        }

        // 步骤2：计算采样空间体积
        double space_volume = 1.0;
        for (int o = 0; o < object_number; ++o) {
            space_volume *= (o_max[o] - o_min[o]);
        }

        // 步骤3：蒙特卡洛采样
        const int sampled = 50000; // 采样次数
        int counter = 0;
        bool is_in_hv;
        double* s_point = new double[object_number];

        for (int s = 0; s < sampled; ++s) {
            // 生成随机采样点
            for (int o = 0; o < object_number; ++o) {
                s_point[o] = rand01() * (o_max[o] - o_min[o]) + o_min[o];
            }

            // 判断采样点是否在超体积内
            is_in_hv = false;
            for (int i = 0; i < size; ++i) {
                is_in_hv = true;
                for (int o = 0; o < object_number; ++o) {
                    if ((s_point[o] < refer_point[o] && s_point[o] < input[i][o]) ||
                        (s_point[o] > refer_point[o] && s_point[o] > input[i][o])) {
                        is_in_hv = false;
                        break;
                    }
                }
                if (is_in_hv) {
                    counter++;
                    break;
                }
            }
        }

        // 释放内存
        delete[] o_max;
        delete[] o_min;
        delete[] s_point;

        // 估计超体积
        return static_cast<double>(counter) / sampled * space_volume;
    }

    /**
     * @brief Coverage指标（原C-Indicator）- 衡量解集A对解集B的支配程度（支持逐目标/整体优化方向）
     * @param setA 解集A [sizeA][object_number]（double**）
     * @param sizeA 解集A大小
     * @param setB 解集B [sizeB][object_number]（double**）
     * @param sizeB 解集B大小
     * @param object_number 目标维度
     * @param min_is_better 逐目标的优化方向：true=该目标越小越好，false=越大越好 [object_number]（优先使用）
     * @param global_min_is_better 整体优化方向（仅当min_is_better为nullptr时生效），默认true（所有目标最小化）
     * @return Coverage(A,B)值：B中被A支配的解的比例 (0≤C≤1)
     */
    double coverage(double** setA, int sizeA, double** setB, int sizeB, int object_number,
        const bool* min_is_better = nullptr,
        bool global_min_is_better = true) {
        if (sizeA == 0 || sizeB == 0) return 0.0;

        int dominated_count = 0;
        // 统计B中被A至少一个解支配的解的数量
        for (int i = 0; i < sizeB; ++i) {
            if (isDominatedBySet(setB[i], setA, sizeA, object_number, min_is_better, global_min_is_better)) {
                dominated_count++;
            }
        }

        // 返回比例
        return static_cast<double>(dominated_count) / sizeB;
    }

} // namespace MetricLib