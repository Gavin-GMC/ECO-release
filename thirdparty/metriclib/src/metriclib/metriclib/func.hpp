#pragma once
#include <cmath>
#include <algorithm>
#include <cstdlib>

// 辅助结构体：用于排序时关联索引和值
template <typename T1, typename T2>
struct sortHelper {
    T1 id;
    T2 value;

    // 重载比较运算符，用于std::sort
    bool operator<(const sortHelper& other) const {
        return value < other.value;
    }
};

// 辅助函数：欧几里得距离计算
inline double eu_distance(const double* a, const double* b, int dim) {
    double dist = 0.0;
    for (int i = 0; i < dim; ++i) {
        dist += std::pow(a[i] - b[i], 2);
    }
    return std::sqrt(dist);
}

// 辅助函数：生成0-1之间的随机数
inline double rand01() {
    return static_cast<double>(rand()) / RAND_MAX;
}
