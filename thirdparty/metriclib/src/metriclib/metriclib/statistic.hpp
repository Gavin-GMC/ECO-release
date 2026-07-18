#pragma once
#include <cmath>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

// 引入浮点比较库
#include <doublelib.hpp>

namespace MetricLib
{
    /**
     * @brief 四分位数计算结果结构体
     * @details 包含下四分位数(Q1/25%)、上四分位数(Q3/75%)、四分位距(IQR=Q3-Q1)
     */
    struct QuartileResult
    {
        double Q1;    // 下四分位数（25%分位）
        double Q3;    // 上四分位数（75%分位）
        double IQR;   // 四分位距 = Q3 - Q1
    };

    /**
     * @brief 计算一维数值数组的算术均值
     * @param data 指向数值数组的指针（只读），不能为空
     * @param length 数组长度（只读），必须大于0
     * @return 数组的算术均值（double类型）
     * @throw std::invalid_argument 如果data为空指针或length<=0
     */
    double mean(const double* data, const int length) {
        if (data == nullptr || length <= 0) {
            throw std::invalid_argument("mean(): data is nullptr or length <= 0");
        }

        double sum = 0.0;
        for (int i = 0; i < length; ++i) {
            sum += data[i];
        }
        return sum / length;
    }

    /**
     * @brief 计算一维数值数组的总体方差（除以n，非样本方差）
     * @param data 指向数值数组的指针（只读），不能为空
     * @param length 数组长度（只读），必须大于0
     * @return 数组的总体方差（double类型）
     * @throw std::invalid_argument 如果data为空指针或length<=0
     * @note 样本方差需除以(length-1)，如需样本方差可修改此函数或新增接口
     */
    double variance(const double* data, const int length) {
        if (data == nullptr || length <= 0) {
            throw std::invalid_argument("variance(): data is nullptr or length <= 0");
        }

        double meanValue = mean(data, length);
        double sum = 0.0;
        for (int i = 0; i < length; ++i) {
            double diff = data[i] - meanValue;
            sum += diff * diff;
        }
        return sum / length;
    }

    /**
     * @brief 计算一维数值数组的总体标准差（方差的平方根）
     * @param data 指向数值数组的指针（只读），不能为空
     * @param length 数组长度（只读），必须大于0
     * @return 数组的总体标准差（double类型）
     * @throw std::invalid_argument 如果data为空指针或length<=0
     */
    double standardDeviation(const double* data, const int length) {
        if (data == nullptr || length <= 0) {
            throw std::invalid_argument("standardDeviation(): data is nullptr or length <= 0");
        }

        return sqrt(variance(data, length));
    }

    /**
     * @brief 计算一维数值数组的中位数
     * @param data 指向数值数组的指针（只读），不能为空
     * @param length 数组长度（只读），必须大于0
     * @return 数组的中位数（double类型）
     * @throw std::invalid_argument 如果data为空指针或length<=0
     * @note 函数内部会创建数组副本并排序，不修改原数组
     */
    double median(const double* data, const int length) {
        if (data == nullptr || length <= 0) {
            throw std::invalid_argument("median(): data is nullptr or length <= 0");
        }

        std::vector<double> sortedData(data, data + length);
        std::sort(sortedData.begin(), sortedData.end());

        if (length % 2 == 0) {
            return (sortedData[length / 2 - 1] + sortedData[length / 2]) / 2.0;
        }
        else {
            return sortedData[length / 2];
        }
    }

    /**
     * @brief 计算一维数值数组的众数（出现频率最高的数值）
     * @param data 指向数值数组的指针（只读），不能为空
     * @param length 数组长度（只读），必须大于0
     * @return 数组的众数（double类型）；若多个数值频率相同，返回第一个出现的众数
     * @throw std::invalid_argument 如果data为空指针或length<=0
     */
    double mode(const double* data, const int length) {
        if (data == nullptr || length <= 0) {
            throw std::invalid_argument("mode(): data is nullptr or length <= 0");
        }

        std::vector<std::pair<double, int>> countList;
        for (int i = 0; i < length; ++i) {
            double current = data[i];
            bool found = false;
            for (auto& pair : countList) {
                if (doublelib::isEqualByULP(pair.first, current)) {
                    pair.second++;
                    found = true;
                    break;
                }
            }
            if (!found) {
                countList.emplace_back(current, 1);
            }
        }

        double modeValue = std::numeric_limits<double>::quiet_NaN();
        int maxCount = 0;
        for (const auto& pair : countList) {
            if (pair.second > maxCount) {
                maxCount = pair.second;
                modeValue = pair.first;
            }
        }

        if (std::isnan(modeValue)) {
            modeValue = data[0];
        }

        return modeValue;
    }

    /**
     * @brief 查找一维数值数组中的最大值
     * @param input 指向数值数组的指针（只读），不能为空
     * @param size 数组长度（只读），必须大于0
     * @return 数组的最大值（double类型）
     * @throw std::invalid_argument 如果input为空指针或size<=0
     */
    double largest(const double* input, const int size)
    {
        if (input == nullptr || size <= 0) {
            throw std::invalid_argument("largest(): input is nullptr or size <= 0");
        }

        double back = input[0];
        for (int i = 1; i < size; i++)
        {
            if (doublelib::isGreaterByULP(input[i], back)) {
                back = input[i];
            }
        }
        return back;
    }

    /**
     * @brief 查找一维数值数组中的最小值
     * @param input 指向数值数组的指针（只读），不能为空
     * @param size 数组长度（只读），必须大于0
     * @return 数组的最小值（double类型）
     * @throw std::invalid_argument 如果input为空指针或size<=0
     */
    double smallest(const double* input, const int size)
    {
        if (input == nullptr || size <= 0) {
            throw std::invalid_argument("smallest(): input is nullptr or size <= 0");
        }

        double back = input[0];
        for (int i = 1; i < size; i++)
        {
            if (doublelib::isLessByULP(input[i], back)) {
                back = input[i];
            }
        }
        return back;
    }

    /**
     * @brief 计算一维数值数组的极差（全距）= 最大值 - 最小值
     * @param data 指向数值数组的指针（只读），不能为空
     * @param length 数组长度（只读），必须大于0
     * @return 数组的极差（double类型），长度为1时极差为0
     * @throw std::invalid_argument 如果data为空指针或length<=0
     */
    double range(const double* data, const int length)
    {
        if (data == nullptr || length <= 0) {
            throw std::invalid_argument("range(): data is nullptr or length <= 0");
        }
        if (length == 1) {
            return 0.0;
        }
        return largest(data, length) - smallest(data, length);
    }

    /**
     * @brief 计算一维数值数组的几何均值（n个非负数乘积的n次方根）
     * @param data 指向数值数组的指针（只读），不能为空
     * @param length 数组长度（只读），必须大于0
     * @return 数组的几何均值（double类型）
     * @throw std::invalid_argument 空指针/长度<=0/数据包含负数（实数范围无几何均值）
     * @note 数据含0时几何均值为0；大数累乘可能存在精度溢出，适用于常规数值范围
     */
    double geometricMean(const double* data, const int length)
    {
        if (data == nullptr || length <= 0) {
            throw std::invalid_argument("geometricMean(): data is nullptr or length <= 0");
        }

        double product = 1.0;
        for (int i = 0; i < length; ++i) {
            if (doublelib::isLessByULP(data[i], 0.0)) {
                throw std::invalid_argument("geometricMean(): data contains negative value, no real geometric mean");
            }
            product *= data[i];
            if (std::isinf(product) || doublelib::isEqualByULP(product, 0.0)) {
                break;
            }
        }
        return pow(product, 1.0 / length);
    }

    /**
     * @brief 计算一维数值数组的调和均值（n / 各数据的倒数之和）
     * @param data 指向数值数组的指针（只读），不能为空
     * @param length 数组长度（只读），必须大于0
     * @return 数组的调和均值（double类型）
     * @throw std::invalid_argument 空指针/长度<=0/数据包含0（倒数无意义）/数据含负数（统计无意义）
     * @note 适用于平均速率、平均密度等正数比值场景
     */
    double harmonicMean(const double* data, const int length)
    {
        if (data == nullptr || length <= 0) {
            throw std::invalid_argument("harmonicMean(): data is nullptr or length <= 0");
        }

        double reciprocalSum = 0.0;
        for (int i = 0; i < length; ++i) {
            if (doublelib::isEqualByULP(data[i], 0.0)) {
                throw std::invalid_argument("harmonicMean(): data contains zero, reciprocal is undefined");
            }
            if (doublelib::isLessByULP(data[i], 0.0)) {
                throw std::invalid_argument("harmonicMean(): data contains negative value, harmonic mean is meaningless in statistics");
            }
            reciprocalSum += 1.0 / data[i];
        }
        return length / reciprocalSum;
    }

    /**
     * @brief 计算一维数值数组的四分位数（Q1/25%、Q3/75%）及四分位距（IQR）
     * @param data 指向数值数组的指针（只读），不能为空
     * @param length 数组长度（只读），必须大于0
     * @return QuartileResult结构体，包含Q1、Q3、IQR；长度<=3时Q1=Q3=中位数，IQR=0
     * @throw std::invalid_argument 如果data为空指针或length<=0
     * @note 内部采用线性插值法计算四分位数，不修改原数组
     */
    QuartileResult interQuartileRange(const double* data, const int length)
    {
        QuartileResult result{};
        if (data == nullptr || length <= 0) {
            throw std::invalid_argument("interQuartileRange(): data is nullptr or length <= 0");
        }

        if (length <= 3) {
            result.Q1 = median(data, length);
            result.Q3 = result.Q1;
            result.IQR = 0.0;
            return result;
        }

        std::vector<double> sortedData(data, data + length);
        std::sort(sortedData.begin(), sortedData.end());

        auto calculateQuantile = [&](double p) -> double {
            double pos = static_cast<double>(sortedData.size() - 1) * p;
            int64_t idx = static_cast<int64_t>(doublelib::floor(pos));
            double frac = doublelib::round(pos - idx);

            if (doublelib::isEqualByULP(frac, 0.0)) {
                return sortedData[idx];
            }
            else {
                double diff = sortedData[idx + 1] - sortedData[idx];
                return sortedData[idx] + frac * diff;
            }
        };

        result.Q1 = calculateQuantile(0.25);
        result.Q3 = calculateQuantile(0.75);
        result.IQR = result.Q3 - result.Q1;

        return result;
    }
} // namespace MetricLib