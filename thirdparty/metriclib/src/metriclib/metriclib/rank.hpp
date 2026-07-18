#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <doublelib.hpp>

namespace MetricLib
{
    /**
     * @brief 判断浮点数是否为无效数（NaN/INF）
     * @param num 待判断的浮点数
     * @return true=无效数，false=有效数
     */
    bool isInvalid(double num) {
        return std::isnan(num) || std::isinf(num);
    }

    /**
     * @brief 计算数组中每个元素的排名
     * @param data 输入数据指针（只读），不能为空
     * @param length 输入数据长度（只读），必须大于0
     * @param min_is_better 排序方向：true=数值越小排名越靠前，false=数值越大排名越靠前（默认true）
     * @return 每个元素对应的排名（double类型）
     * @throw std::invalid_argument 如果data为空指针或length<=0
     */
    std::vector<double> calculateRanks(const double* data, const int length, const bool min_is_better = true) {
        // 边界检查：空指针或无效长度
        if (data == nullptr) {
            throw std::invalid_argument("calculateRanks(): data is nullptr");
        }
        if (length <= 0) {
            throw std::invalid_argument("calculateRanks(): length must be greater than 0");
        }

        // 步骤1：分离有效数和无效数，记录原始索引
        std::vector<std::pair<double, int>> validData; // <数值, 原始索引>
        std::vector<int> invalidIndices;               // 无效数的原始索引

        for (int i = 0; i < length; ++i) {
            if (isInvalid(data[i])) {
                invalidIndices.push_back(i);
            }
            else {
                validData.emplace_back(data[i], i);
            }
        }
        int validCount = validData.size();
        int invalidCount = invalidIndices.size();

        // 步骤2：初始化排名数组（长度与输入一致）
        std::vector<double> ranks(length, 0.0);

        // 步骤3：处理有效数的排名（根据min_is_better切换排序方向）
        if (validCount > 0) {
            // 按指定方向排序（使用doublelib的ULP比较保证精度）
            std::sort(validData.begin(), validData.end(),
                [min_is_better](const std::pair<double, int>& a, const std::pair<double, int>& b) {
                    if (min_is_better) {
                        // 数值越小越靠前：a < b 则排在前面
                        return doublelib::isLessByULP(a.first, b.first);
                    }
                    else {
                        // 数值越大越靠前：a > b 则排在前面
                        return doublelib::isGreaterByULP(a.first, b.first);
                    }
                });

            // 遍历排序后的有效数，计算相同值的平均排名
            int currentPos = 1; // 初始排名（从1开始）
            while (currentPos <= validCount) {
                double currentVal = validData[currentPos - 1].first;
                int startPos = currentPos;
                int endPos = currentPos;

                // 找到连续相同值的结束位置
                while (endPos < validCount &&
                    doublelib::isEqualByULP(validData[endPos].first, currentVal)) {
                    endPos++;
                }
                endPos--; // 修正为最后一个相同值的位置

                // 计算平均排名：(起始名次 + 结束名次) / 2
                double avgRank = (startPos + endPos) / 2.0;

                // 将平均排名赋值给对应原始索引
                for (int i = startPos - 1; i <= endPos - 1; ++i) {
                    int originalIdx = validData[i].second;
                    ranks[originalIdx] = avgRank;
                }

                // 移动到下一组不同值的位置
                currentPos = endPos + 1;
            }
        }

        // 步骤4：处理无效数的排名（全部排最后，取平均排名）
        if (invalidCount > 0) {
            int invalidStartRank = validCount + 1;
            int invalidEndRank = length;
            double invalidAvgRank = (invalidStartRank + invalidEndRank) / 2.0;

            for (int idx : invalidIndices) {
                ranks[idx] = invalidAvgRank;
            }
        }

        return ranks;
    }
} // namespace MetricLib