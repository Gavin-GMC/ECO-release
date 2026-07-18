// 头文件保护，防止重复包含
#pragma once
// 引入必要的标准库头文件
#include <cmath>       // 数学函数（取整、绝对值、特殊值判断）
#include <stdexcept>   // 异常处理（非法精度/浮点值校验）
#include <limits>      // 用于浮点数极值判断
#include <cstdint>     // 固定宽度整数，处理浮点数二进制位（uint64_t/int64_t）

// 浮点数处理库命名空间
namespace doublelib {
    // 全局精度变量，默认1e-9（固定epsilon比较的默认值）
    static double g_precision = 1e-9;

    /**
     * @brief 设置全局默认epsilon精度（原有固定精度比较的无参调用时使用）
     * @param precision 精度值，必须大于0
     * @throw std::invalid_argument 若精度<=0则抛出异常
     */
    inline void setPrecision(double precision) {
        if (precision <= 0) {
            throw std::invalid_argument("Precision must be a positive number.");
        }
        g_precision = precision;
    }

    /**
     * @brief 获取当前全局默认epsilon精度
     * @return 当前全局精度值
     */
    inline double getPrecision() {
        return g_precision;
    }

    // -------------------------- 基于固定epsilon的比较函数 --------------------------
    inline bool isEqual(const double& a, const double& b, double precision = g_precision) {
        return std::fabs(a - b) < precision;
    }

    inline bool isGreater(const double& a, const double& b, double precision = g_precision) {
        return (a - b) > precision;
    }

    inline bool isLess(const double& a, const double& b, double precision = g_precision) {
        return (b - a) > precision;
    }

    inline bool isGreaterOrEqual(const double& a, const double& b, double precision = g_precision) {
        return isGreater(a, b, precision) || isEqual(a, b, precision);
    }

    inline bool isLessOrEqual(const double& a, const double& b, double precision = g_precision) {
        return isLess(a, b, precision) || isEqual(a, b, precision);
    }

    // -------------------------- 内部辅助函数：ULP计算相关（对外不可见） --------------------------
    /**
     * @brief 内部辅助：将double转换为64位有符号整数（映射IEEE 754二进制位）
     * @param num 待转换双精度浮点数
     * @return 对应的64位有符号整数
     */
    inline int64_t doubleToInt64(const double& num) {
        static_assert(sizeof(double) == sizeof(int64_t), "double must be 64 bits (IEEE 754)");
        return *reinterpret_cast<const int64_t*>(&num);
    }

    /**
     * @brief 内部辅助：计算两个double之间的ULP距离（核心ULP计算逻辑）
     * @param a 第一个浮点数
     * @param b 第二个浮点数
     * @return 两个数之间的ULP个数（无符号，保证非负）
     * @throw std::invalid_argument 若包含NaN/INF则抛出异常
     */
    inline uint64_t ulpDistance(const double& a, const double& b) {
        // 第一步：校验特殊值（NaN/INF无法进行ULP比较）
        if (std::isnan(a) || std::isnan(b) || std::isinf(a) || std::isinf(b)) {
            throw std::invalid_argument("Cannot calculate ULP distance for NaN or infinite value.");
        }

        // 第二步：映射为64位整数（利用IEEE 754双精度的单调性：数值越大，整数表示越大）
        int64_t ia = doubleToInt64(a);
        int64_t ib = doubleToInt64(b);

        // 第三步：处理符号不同的情况（正数和负数的ULP距离直接取差值绝对值）
        if ((ia ^ ib) < 0) { // 符号位不同（异或后最高位为0表示同号，1表示异号）
            const double zero_ulp = 1.0;
            return static_cast<uint64_t>(ulpDistance(a, zero_ulp) + ulpDistance(b, zero_ulp));
        }

        // 第四步：计算同号数的ULP距离（整数差值的绝对值即为ULP个数）
        uint64_t diff = static_cast<uint64_t>(ia > ib ? ia - ib : ib - ia);
        return diff;
    }

    // -------------------------- 基于ULP的比较函数（默认2倍ULP） --------------------------
    /**
     * @brief 基于ULP的浮点数相等比较（核心ULP比较接口）
     * @param a 第一个浮点数
     * @param b 第二个浮点数
     * @param ulp_multiplier ULP倍数，默认2（适配常规存储/单次运算误差）
     * @return 若两数ULP距离≤ulp_multiplier则返回true，否则false
     * @throw std::invalid_argument 若包含NaN/INF或ulp_multiplier<0则抛出异常
     */
    inline bool isEqualByULP(const double& a, const double& b, int ulp_multiplier = 2) {
        if (ulp_multiplier < 0) {
            throw std::invalid_argument("ULP multiplier must be a non-negative integer.");
        }
        return ulpDistance(a, b) <= static_cast<uint64_t>(ulp_multiplier);
    }

    /**
     * @brief 基于ULP的浮点数大于比较（严格大于，且ULP距离超过设定倍数）
     * @param a 第一个浮点数
     * @param b 第二个浮点数
     * @param ulp_multiplier ULP倍数，默认2
     * @return 若a严格大于b且ULP距离>ulp_multiplier则返回true，否则false
     * @throw std::invalid_argument 若包含NaN/INF或ulp_multiplier<0则抛出异常
     */
    inline bool isGreaterByULP(const double& a, const double& b, int ulp_multiplier = 2) {
        if (ulp_multiplier < 0) {
            throw std::invalid_argument("ULP multiplier must be a non-negative integer.");
        }
        // 先判断是否相等（ULP范围内），相等则直接返回false
        if (isEqualByULP(a, b, ulp_multiplier)) {
            return false;
        }
        // 非相等情况下，直接判断数值大小（已排除精度误差）
        return a > b;
    }

    /**
     * @brief 基于ULP的浮点数小于比较（严格小于，且ULP距离超过设定倍数）
     * @param a 第一个浮点数
     * @param b 第二个浮点数
     * @param ulp_multiplier ULP倍数，默认2
     * @return 若a严格小于b且ULP距离>ulp_multiplier则返回true，否则false
     * @throw std::invalid_argument 若包含NaN/INF或ulp_multiplier<0则抛出异常
     */
    inline bool isLessByULP(const double& a, const double& b, int ulp_multiplier = 2) {
        if (ulp_multiplier < 0) {
            throw std::invalid_argument("ULP multiplier must be a non-negative integer.");
        }
        if (isEqualByULP(a, b, ulp_multiplier)) {
            return false;
        }
        return a < b;
    }

    /**
     * @brief 基于ULP的浮点数大于等于比较
     * @param a 第一个浮点数
     * @param b 第二个浮点数
     * @param ulp_multiplier ULP倍数，默认2
     * @return 若a>b（ULP外）或a==b（ULP内）则返回true，否则false
     * @throw std::invalid_argument 若包含NaN/INF或ulp_multiplier<0则抛出异常
     */
    inline bool isGreaterOrEqualByULP(const double& a, const double& b, int ulp_multiplier = 2) {
        return isGreaterByULP(a, b, ulp_multiplier) || isEqualByULP(a, b, ulp_multiplier);
    }

    /**
     * @brief 基于ULP的浮点数小于等于比较
     * @param a 第一个浮点数
     * @param b 第二个浮点数
     * @param ulp_multiplier ULP倍数，默认2
     * @return 若a<b（ULP外）或a==b（ULP内）则返回true，否则false
     * @throw std::invalid_argument 若包含NaN/INF或ulp_multiplier<0则抛出异常
     */
    inline bool isLessOrEqualByULP(const double& a, const double& b, int ulp_multiplier = 2) {
        return isLessByULP(a, b, ulp_multiplier) || isEqualByULP(a, b, ulp_multiplier);
    }

    // -------------------------- 带精度容错的取整函数 --------------------------
    inline bool isIntegerInPrecision(const double& num) {
        double nearest_int = std::round(num);
        return std::fabs(num - nearest_int) < g_precision;
    }

    inline double round(const double& num) {
        if (std::isnan(num) || std::isinf(num)) {
            throw std::invalid_argument("Cannot round NaN or infinite value.");
        }
        if (isIntegerInPrecision(num)) {
            return std::round(num);
        }
        return std::round(num);
    }

    inline double ceil(const double& num) {
        if (std::isnan(num) || std::isinf(num)) {
            throw std::invalid_argument("Cannot ceil NaN or infinite value.");
        }
        if (isIntegerInPrecision(num)) {
            return std::round(num);
        }
        return std::ceil(num);
    }

    inline double floor(const double& num) {
        if (std::isnan(num) || std::isinf(num)) {
            throw std::invalid_argument("Cannot floor NaN or infinite value.");
        }
        if (isIntegerInPrecision(num)) {
            return std::round(num);
        }
        return std::floor(num);
    }

    inline double trunc(const double& num) {
        if (std::isnan(num) || std::isinf(num)) {
            throw std::invalid_argument("Cannot trunc NaN or infinite value.");
        }
        if (isIntegerInPrecision(num)) {
            return std::round(num);
        }
        return std::trunc(num);
    }
} // namespace doublelib