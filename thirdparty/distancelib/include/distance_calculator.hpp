#ifndef DISTANCE_CALCULATOR_H
#define DISTANCE_CALCULATOR_H

#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <climits>  // 用于INT_MAX（闵可夫斯基切比雪夫特例）

// 距离计算专用命名空间
namespace distancelib {

    // ************************ 通用数值向量：闵可夫斯基距离（基础泛化）************************
    /**
     * @brief 闵可夫斯基距离（欧几里得/曼哈顿/切比雪夫的通用形式）
     * @tparam T 数值类型（int/double/float等）
     * @param vec1 第一个d维数值数组指针（不可修改）
     * @param vec2 第二个d维数值数组指针（不可修改）
     * @param dim 数组维度/长度，两个数组必须等长
     * @param p 阶数（p=1:曼哈顿，p=2:欧几里得，p=INT_MAX:切比雪夫，p>0）
     * @return 闵可夫斯基距离值（double）
     * @throw 数组指针为空/维度dim=0/p≤0 抛出非法参数异常
     */
    template <typename T>
    double minkowski(const T* vec1, const T* vec2, size_t dim, int p) {
        // 指针非空校验
        if (vec1 == nullptr || vec2 == nullptr) {
            throw std::invalid_argument("minkowski: 数值数组指针不能为空");
        }
        // 维度校验
        if (dim == 0) {
            return 0.0;
        }
        // 阶数校验
        if (p <= 0) {
            throw std::invalid_argument("minkowski: 阶数p必须大于0");
        }

        double distance = 0.0;
        for (size_t i = 0; i < dim; ++i) {
            double diff = std::fabs(static_cast<double>(vec1[i]) - vec2[i]);
            if (p == 1) {
                distance += diff; // 曼哈顿优化，避免pow计算
            }
            else if (p == 2) {
                distance += diff * diff; // 欧几里得优化
            }
            else {
                distance += std::pow(diff, p);
            }
        }

        // 切比雪夫距离：p取INT_MAX，单次遍历找各维度绝对差的最大值
        if (p == INT_MAX) {
            double max_diff = 0.0;
            for (size_t i = 0; i < dim; ++i) {
                max_diff = std::max(max_diff, std::fabs(static_cast<double>(vec1[i]) - vec2[i]));
            }
            return max_diff;
        }

        return std::pow(distance, 1.0 / p);
    }

    // ************************ 数值向量：曼哈顿距离（独立实现，p=1专用，性能优化）************************
    /**
    * @brief 曼哈顿距离（城市街区距离，独立实现无冗余计算）
    * @tparam T 数值类型（int/double/float等，支持算术运算）
    * @param vec1 第一个d维数值数组指针（不可修改）
    * @param vec2 第二个d维数值数组指针（不可修改）
    * @param dim 数组维度/长度，两个数组必须等长
    * @return 曼哈顿距离值（double，各维度绝对差之和）
    * @throw 数组指针为空 抛出非法参数异常
    */
    template <typename T>
    double manhattan(const T* vec1, const T* vec2, size_t dim) {
        if (vec1 == nullptr || vec2 == nullptr) {
            throw std::invalid_argument("manhattan: 数值数组指针不能为空");
        }
        if (dim == 0) {
            return 0.0;
        }

        double distance = 0.0;
        for (size_t i = 0; i < dim; ++i) {
            // 直接计算绝对差，无pow/分支判断，性能最优
            distance += std::fabs(static_cast<double>(vec1[i]) - vec2[i]);
        }
        return distance;
    }

    // ************************ 数值向量：欧几里得距离（独立实现，p=2专用，性能优化）************************
    /**
     * @brief 欧几里得距离（直线距离，独立实现无冗余计算）
     * @tparam T 数值类型（int/double/float等，支持算术运算）
     * @param vec1 第一个d维数值数组指针（不可修改）
     * @param vec2 第二个d维数值数组指针（不可修改）
     * @param dim 数组维度/长度，两个数组必须等长
     * @return 欧几里得距离值（double，各维度差的平方和开根号）
     * @throw 数组指针为空 抛出非法参数异常
     */
    template <typename T>
    double euclidean(const T* vec1, const T* vec2, size_t dim) {
        if (vec1 == nullptr || vec2 == nullptr) {
            throw std::invalid_argument("euclidean: 数值数组指针不能为空");
        }
        if (dim == 0) {
            return 0.0;
        }

        double sqr_sum = 0.0;
        for (size_t i = 0; i < dim; ++i) {
            const double diff = static_cast<double>(vec1[i]) - vec2[i];
            // 直接平方累加，用sqrt替代pow(1/2)，精度+性能双优
            sqr_sum += diff * diff;
        }
        return std::sqrt(sqr_sum);
    }

    // ************************ 数值向量：切比雪夫距离（独立实现，p→∞专用，性能优化）************************
    /**
     * @brief 切比雪夫距离（棋盘距离，独立实现无冗余计算）
     * @tparam T 数值类型（int/double/float等，支持算术运算）
     * @param vec1 第一个d维数值数组指针（不可修改）
     * @param vec2 第二个d维数值数组指针（不可修改）
     * @param dim 数组维度/长度，两个数组必须等长
     * @return 切比雪夫距离值（double，各维度绝对差的最大值）
     * @throw 数组指针为空 抛出非法参数异常
     */
    template <typename T>
    double chebyshev(const T* vec1, const T* vec2, size_t dim) {
        if (vec1 == nullptr || vec2 == nullptr) {
            throw std::invalid_argument("chebyshev: 数值数组指针不能为空");
        }
        if (dim == 0) {
            return 0.0;
        }

        double max_diff = 0.0;
        for (size_t i = 0; i < dim; ++i) {
            const double diff = std::fabs(static_cast<double>(vec1[i]) - vec2[i]);
            // 单次遍历找最大值，无二次遍历，性能提升显著
            max_diff = std::max(max_diff, diff);
        }
        return max_diff;
    }

    // ************************ 高维向量：余弦距离（方向相似性，适配图着色高维特征）************************
    /**
     * @brief 余弦距离（1 - 余弦相似度，取值[0,2]，值越小方向越相似）
     * @tparam T 数值类型
     * @param vec1 第一个d维向量数组指针（不可修改，支持高维稀疏/稠密）
     * @param vec2 第二个d维向量数组指针（不可修改）
     * @param dim 数组维度/长度，两个数组必须等长
     * @return 余弦距离值（double）
     * @throw 数组指针为空/零向量 抛出异常
     */
    template <typename T>
    double cosine(const T* vec1, const T* vec2, size_t dim) {
        // 空指针检查
        if (vec1 == nullptr || vec2 == nullptr) {
            throw std::invalid_argument("cosine: 数值数组指针不能为空");
        }
        if (dim == 0) {
            return 0.0;
        }

        double dot_product = 0.0;    // 点积
        double norm1 = 0.0;          // 向量1的模的平方
        double norm2 = 0.0;          // 向量2的模的平方
        // 一次循环计算点积+模平方，效率最优
        for (size_t i = 0; i < dim; ++i) {
            double v1 = static_cast<double>(vec1[i]);
            double v2 = static_cast<double>(vec2[i]);
            dot_product += v1 * v2;
            norm1 += v1 * v1;
            norm2 += v2 * v2;
        }

        // 浮点数比较加绝对值，避免下溢导致的判断失效
        if (std::fabs(norm1) < 1e-8 || std::fabs(norm2) < 1e-8) {
            throw std::invalid_argument("cosine: 输入向量不能为零向量（所有元素为0）");
        }

        // 计算余弦相似度
        double cos_sim = dot_product / (std::sqrt(norm1) * std::sqrt(norm2));
        // 钳位到[-1, 1]，避免数值越界（核心鲁棒性保障）
        cos_sim = std::max(-1.0, std::min(cos_sim, 1.0));

        // 余弦距离 = 1 - 余弦相似度
        return 1.0 - cos_sim;
    }

    // ************************ 离散序列：汉明距离（适配图着色等长标签序列）************************
    /**
     * @brief 汉明距离（等长离散序列，对应位置不同元素的个数，适配图着色顶点标签序列）
     * @tparam T 可比较类型（int/char/string等，需支持!=运算符）
     * @param seq1 第一个等长离散序列数组指针（不可修改）
     * @param seq2 第二个等长离散序列数组指针（不可修改）
     * @param len 序列长度，两个序列必须等长
     * @return 汉明距离值（int，差异元素个数）
     * @throw 序列指针为空 抛出异常
     */
    template <typename T>
    int hamming(const T* seq1, const T* seq2, size_t len) {
        if (seq1 == nullptr || seq2 == nullptr) {
            throw std::invalid_argument("hamming: 序列数组指针不能为空");
        }
        if (len == 0) {
            return 0;
        }

        int diff_count = 0;
        for (size_t i = 0; i < len; ++i) {
            if (seq1[i] != seq2[i]) {
                diff_count++;
            }
        }
        return diff_count;
    }

    // ************************ 集合/分簇：图着色分簇最少修改数（声明保持风格一致）************************
    /**
     * @brief 计算图着色分簇方案转换的最少节点修改数（簇ID无意义，仅看同簇关系）
     * @tparam T 簇ID类型（int/size_t/char等，支持哈希和比较）
     * @param cluster1 方案1的簇编号数组指针（不可修改，n维，n为节点数，第i维=节点i的簇ID）
     * @param cluster2 方案2的簇编号数组指针（不可修改，与cluster1等长，同维度对应同一节点）
     * @param n 节点总数/数组长度，两个数组必须等长
     * @return int 最少修改的节点数量
     * @throw 数组指针为空 抛出异常
     */
    // template <typename T>
    // int min_cluster_modify(const T* cluster1, const T* cluster2, size_t n) {
    //    return 0;
    // }

} // namespace distancelib

#endif // DISTANCE_CALCULATOR_H