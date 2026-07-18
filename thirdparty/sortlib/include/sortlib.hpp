#ifndef SORTLIB_HPP
#define SORTLIB_HPP

#include <algorithm>   // 核心依赖：std::sort
#include <cassert>     // 入参合法性断言
#include <vector>      // 关联排序的索引数组、临时存储
#include <numeric>     // std::iota：快速初始化索引数组
#include <iterator>    // std::iterator_traits（迭代器值类型推导）

// 命名空间封装，避免全局命名污染
namespace sortlib {

    // 排列方向枚举：强类型，无修改
    enum class SortOrder {
        ASC,  // 升序（从小到大）
        DESC  // 降序（从大到小）
    };

    /**
     * @brief 普通数组排序（仅基于<运算符，支持任意实现<的类型）
     * @tparam T 数组元素类型（仅需重载<运算符，无需>）
     * @param arr 待排序数组的指针（非空）
     * @param len 数组长度（>0）
     * @param order 排列方向，默认升序
     */
    template <typename T>
    void sortArray(T* arr, size_t len, SortOrder order = SortOrder::ASC) {
        assert(arr != nullptr && "Array pointer cannot be null!");
        assert(len > 0 && "Array length must be greater than 0!");

        if (order == SortOrder::ASC) {
            // 升序：原生小于比较（a < b），直接用std::less（底层也是<）
            std::sort(arr, arr + len, [](const T& lhs, const T& rhs) {
                return lhs < rhs;
                });
        }
        else {
            // 降序：反向小于比较（b < a），全程不使用>
            std::sort(arr, arr + len, [](const T& lhs, const T& rhs) {
                return rhs < lhs;
                });
        }
    }

    /**
     * @brief 指针数组排序（按指向值排序，仅基于<运算符）
     * @tparam T 指针指向的元素类型（仅需重载<运算符）
     * @param ptrArr 指针数组的指针（元素类型为T*，非空）
     * @param len 指针数组长度（>0）
     * @param order 排列方向，默认升序
     * @note 确保所有指针都指向有效对象，避免野指针解引用！
     */
    template <typename T>
    void sortPointerArray(T** ptrArr, size_t len, SortOrder order = SortOrder::ASC) {
        assert(ptrArr != nullptr && "Pointer array cannot be null!");
        assert(len > 0 && "Pointer array length must be greater than 0!");

        if (order == SortOrder::ASC) {
            // 升序：解引用后原生小于比较（*lhs < *rhs）
            std::sort(ptrArr, ptrArr + len, [](const T* lhs, const T* rhs) {
                return *lhs < *rhs;
                });
        }
        else {
            // 降序：解引用后反向小于比较（*rhs < *lhs），无>运算符
            std::sort(ptrArr, ptrArr + len, [](const T* lhs, const T* rhs) {
                return *rhs < *lhs;
                });
        }
    }

    /**
     * @brief 两组关联数据排序（基于主数组a排序，仅依赖a的<运算符，b无比较要求）
     * @tparam T1 主数组a的元素类型（仅需重载<运算符，排序依据）
     * @tparam T2 副数组b的元素类型（任意类型，无任何比较要求）
     * @param a 主数组指针（排序依据，非空）
     * @param b 副数组指针（同步排序，非空）
     * @param len 两个数组的长度（必须相同且>0）
     * @param order 排列方向，默认升序
     */
    template <typename T1, typename T2>
    void sortAssociatedArrays(T1* a, T2* b, size_t len, SortOrder order = SortOrder::ASC) {
        assert(a != nullptr && "Main array pointer cannot be null!");
        assert(b != nullptr && "Associated array pointer cannot be null!");
        assert(len > 0 && "Array length must be greater than 0!");

        // 步骤1：创建原始索引数组（核心：通过索引关联两个数组，无修改）
        std::vector<size_t> indices(len);
        std::iota(indices.begin(), indices.end(), 0);

        // 步骤2：基于主数组a的<运算符排序索引，降序用反向小于
        if (order == SortOrder::ASC) {
            std::sort(indices.begin(), indices.end(), [&a](size_t i, size_t j) {
                return a[i] < a[j];  // 升序：原生小于
                });
        }
        else {
            std::sort(indices.begin(), indices.end(), [&a](size_t i, size_t j) {
                return a[j] < a[i];  // 降序：反向小于，无>运算符
                });
        }

        // 步骤3-4：根据排序后的索引整理原数组（无修改，保证关联关系）
        std::vector<T1> temp_a(len);
        std::vector<T2> temp_b(len);
        for (size_t k = 0; k < len; ++k) {
            temp_a[k] = a[indices[k]];
            temp_b[k] = b[indices[k]];
        }
        std::copy(temp_a.begin(), temp_a.end(), a);
        std::copy(temp_b.begin(), temp_b.end(), b);
    }


    // -------------------------- 标准迭代器接口的排序算法（对齐std::sort）--------------------------
/**
 * @brief 冒泡排序（优化版，稳定，O(n²)）
 * @tparam RandomIt 随机访问迭代器 | Compare 比较器（默认std::less，仅基于<）
 */
    template <class RandomIt, class Compare = std::less<>>
    void bubbleSort(RandomIt first, RandomIt last, Compare comp = Compare{}) {
        if (first == last) return;
        auto n = last - first;
        for (decltype(n) i = 0; i < n - 1; ++i) {
            bool swapped = false;
            for (decltype(n) j = 0; j < n - 1 - i; ++j) {
                if (comp(*(first + j + 1), *(first + j))) {
                    std::swap(*(first + j), *(first + j + 1));
                    swapped = true;
                }
            }
            if (!swapped) break;
        }
    }

    /**
     * @brief 选择排序（不稳定，O(n²)，交换次数少）
     * @tparam RandomIt 随机访问迭代器 | Compare 比较器（默认std::less，仅基于<）
     */
    template <class RandomIt, class Compare = std::less<>>
    void selectionSort(RandomIt first, RandomIt last, Compare comp = Compare{}) {
        if (first == last) return;
        auto n = last - first;
        for (decltype(n) i = 0; i < n - 1; ++i) {
            auto min_idx = i;
            for (decltype(n) j = i + 1; j < n; ++j) {
                if (comp(*(first + j), *(first + min_idx))) {
                    min_idx = j;
                }
            }
            if (min_idx != i) std::swap(*(first + i), *(first + min_idx));
        }
    }

    /**
     * @brief 插入排序（优化版，稳定，O(n²)，小规模数据高效）
     * @tparam RandomIt 随机访问迭代器 | Compare 比较器（默认std::less，仅基于<）
     */
    template <class RandomIt, class Compare = std::less<>>
    void insertionSort(RandomIt first, RandomIt last, Compare comp = Compare{}) {
        if (first == last) return;
        auto n = last - first;
        using ValueType = typename std::iterator_traits<RandomIt>::value_type;
        for (decltype(n) i = 1; i < n; ++i) {
            ValueType temp = std::move(*(first + i));
            decltype(i) j = i;
            for (; j > 0 && comp(temp, *(first + j - 1)); --j) {
                *(first + j) = std::move(*(first + j - 1));
            }
            *(first + j) = std::move(temp);
        }
    }

    /**
    * @brief BinSort（箱排序）- 接口严格对齐std::sort，仅支持数值类型（int/double/float等）
    * @tparam RandomIt 随机访问迭代器（vector/array/裸数组，仅数值类型）
    * @tparam Compare  比较器类型，默认std::less<>（仅基于<运算符，无>）
    * @param first     待排序区间起始迭代器（包含），与std::sort一致
    * @param last      待排序区间结束迭代器（不包含），与std::sort一致
    * @param comp      自定义比较器，返回true表示a应排在b前，默认a < b
    * @note 1. 核心流程：找数值最值→自动分箱→元素入箱→按序合并，箱内天然有序，无需二次排序
    * @note 2. 内部自动推导分箱步长：整数=1，浮点型基于数值范围/元素数量动态计算
    * @note 3. 稳定排序、线性时间O(n+M)、仅基于<运算符，降序通过comp(b,a)实现
    * @note 4. 专为数值类型设计，非数值类型（string/自定义）请使用bucketSort
    */
    template <class RandomIt, class Compare = std::less<>>
    void binSort(RandomIt first, RandomIt last, Compare comp = Compare{}) {
        // 空区间直接返回，与std::sort行为一致
        if (first == last) return;
        using ValueType = typename std::iterator_traits<RandomIt>::value_type;
        auto elemCount = last - first; // 元素数量，随机访问迭代器支持减法

        // 步骤1：获取数值范围[minVal, maxVal]（仅基于<运算符，std::min/max_element底层用<）
        RandomIt minIt = std::min_element(first, last, comp);
        RandomIt maxIt = std::max_element(first, last, comp);
        ValueType minVal = *minIt;
        ValueType maxVal = *maxIt;

        // 所有元素相同，无需排序，直接返回
        if (!comp(minVal, maxVal) && !comp(maxVal, minVal)) return;

        // 步骤2：内部自动推导分箱步长（binStep），无外部参数，核心适配
        ValueType binStep = 1; // 整数默认步长1（最优，箱内天然有序）
        // 浮点型动态计算步长：(最大值-最小值)/元素数量，保证分箱数量合理，避免箱过多/过少
        if constexpr (std::is_floating_point_v<ValueType>) {
            binStep = (maxVal - minVal) / static_cast<ValueType>(elemCount);
            // 处理浮点步长过小的边界，避免分箱数量过多
            if (binStep < std::numeric_limits<ValueType>::epsilon()) {
                binStep = static_cast<ValueType>(1);
            }
        }

        // 步骤3：计算箱数量（向上取整，避免遗漏最大值）
        size_t binCount = static_cast<size_t>((maxVal - minVal) / binStep) + 1;
        // 初始化箱：vector嵌套vector，保证稳定排序（相同值按原始顺序入箱）
        std::vector<std::vector<ValueType>> bins(binCount);

        // 步骤4：元素按数值入箱（移动构造，减少拷贝，无排序操作）
        for (RandomIt it = first; it != last; ++it) {
            size_t binIdx = static_cast<size_t>((*it - minVal) / binStep);
            // 边界保护：避免浮点精度问题导致索引越界
            binIdx = std::min(binIdx, binCount - 1);
            bins[binIdx].push_back(std::move(*it));
        }

        // 步骤5：按比较器规则合并箱（仅基于<判断，升序正序合并，降序逆序合并）
        RandomIt resIt = first;
        if (comp(minVal, maxVal)) {
            // 升序（默认）：从最小箱到最大箱依次合并
            for (auto& bin : bins) {
                for (auto& val : bin) {
                    *resIt++ = std::move(val);
                }
            }
        }
        else {
            // 降序：从最大箱到最小箱逆序合并，无需>运算符
            for (auto binIt = bins.rbegin(); binIt != bins.rend(); ++binIt) {
                for (auto& val : *binIt) {
                    *resIt++ = std::move(val);
                }
            }
        }
    }

    /**
    * @brief 快速排序（纯手动实现，无std::sort调用）- 接口严格对齐std::sort
    * @tparam RandomIt 随机访问迭代器（vector/array/裸数组）
    * @tparam Compare  比较器类型，默认std::less<>（仅基于<运算符，无>）
    * @param first     待排序区间起始迭代器（包含）
    * @param last      待排序区间结束迭代器（不包含）
    * @param comp      自定义比较器，返回true表示a应排在b前，默认a < b
    * @note 1. 核心：分治思想（选基准→分区→递归子区间），纯手动实现无标准库依赖
    * @note 2. 优化：三数取中法选基准（避免有序数组退化为O(n²)）+ Hoare高效分区
    * @note 3. 仅基于<运算符，所有大小判断通过comp，降序通过外部比较器实现
    * @note 4. 时间复杂度O(n log n)（平均）/O(n²)（最坏），空间复杂度O(log n)（递归栈）
    * @note 5. 不稳定排序，随机访问迭代器专属，与其他算法接口完全统一
    */
    template <class RandomIt, class Compare = std::less<>>
    void quickSort(RandomIt first, RandomIt last, Compare comp = Compare{}) {
        // 边界条件：空区间或单元素区间，直接返回（递归终止）
        if (first >= last - 1) return;
        using ValueType = typename std::iterator_traits<RandomIt>::value_type;
        auto len = last - first;

        // 步骤1：三数取中法选基准（首、中、尾），避免有序/逆序数组的最坏情况
        RandomIt mid = first + len / 2;
        // 排序首、中、尾三个元素，将中间值交换到first位置作为基准（仅3次比较，代价极小）
        if (comp(*mid, *first)) std::swap(*mid, *first);
        if (comp(*(last - 1), *first)) std::swap(*(last - 1), *first);
        if (comp(*(last - 1), *mid)) std::swap(*(last - 1), *mid);
        ValueType pivot = *first; // 基准值：三数取中的中间值

        // 步骤2：Hoare分区法（高效，交换次数少），返回基准的正确分割位置
        RandomIt left = first + 1;
        RandomIt right = last - 1;
        while (true) {
            // 左指针右移：找到第一个不小于基准的元素（按comp规则）
            while (left <= right && comp(*left, pivot)) ++left;
            // 右指针左移：找到第一个不大于基准的元素（按comp规则）
            while (right >= left && comp(pivot, *right)) --right;
            // 指针相遇，分区结束
            if (left > right) break;
            // 交换左右指针元素，继续分区
            std::swap(*left, *right);
            ++left;
            --right;
        }
        // 将基准值交换到正确的分割位置（right为左区间最后一个元素）
        std::swap(*first, *right);

        // 步骤3：递归处理左、右子区间（分治）
        quickSort(first, right, comp);  // 左区间：[first, right)
        quickSort(right + 1, last, comp); // 右区间：[right+1, last)
    }
}  // namespace CustomSort

#endif  // SORTLIB_HPP