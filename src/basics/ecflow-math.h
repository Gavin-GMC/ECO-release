#pragma once
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <string>
#include <limits>
#include"ecflow-constant.h"

namespace ECFlow
{
	inline double EQACCURACY = 1e-8;   // inline: single definition across all TUs (C++17)

	/**
	* @brief 对输入数组进行归一化处理（Min-Max归一化，映射到[0,1]区间）
	*
	* 逐元素对输入数组执行归一化计算，结果存入输出数组，输入输出数组长度需一致；
	* 若数组所有元素值相同（分母为0），输出数组所有元素设为0.0。
	*
	* @param input 输入数组指针，指向待归一化的double类型数组首地址
	* @param output 输出数组指针，指向存储归一化结果的double类型数组首地址（需提前分配内存）
	* @param length 数组长度，需≥1（若length≤0，函数无操作）
	* @note 1. inline特性：避免多重定义，同时提示编译器内联展开以提升效率；
	*       2. 输入/输出数组需保证内存有效且长度匹配，否则会导致内存越界；
	*       3. 归一化公式：output[i] = (input[i] - min(input)) / (max(input) - min(input))
	*/
	inline void normalization(double* input, double* output, size_t length);

	/**
	 * @brief 计算Sigmoid激活函数值
	 *
	 * 实现标准Sigmoid函数计算，将输入值映射到(0,1)区间，常用于神经网络激活、概率转换等场景。
	 *
	 * @param value 输入的double类型数值（无取值范围限制）
	 * @return double Sigmoid计算结果，公式：1 / (1 + exp(-value))
	 * @note 1. inline特性：减少函数调用开销，适合高频调用的小函数；
	 *       2. 若value为极大正数，返回值趋近于1；为极小负数，返回值趋近于0；NaN/无穷抛异常
	 * @note 迁移修复:原为 `inline` 声明但定义在 ecflow-math.cpp(非 inline)→ 跨库(如 bpso)未解析。
	 *       改为头内真 inline 定义(定义已从 ecflow-math.cpp 移除)。忠实保留原 NaN/inf 抛错逻辑。
	 */
	inline double sigmoid(double value)
	{
		if (std::isnan(value))
			throw std::runtime_error("sigmoid: input value is NaN (not a number)");
		if (std::isinf(value))
			throw std::runtime_error("sigmoid: input value is infinite");

		if (value > 30.0)
			return 1.0;
		else if (value < -30.0)
			return 0.0;
		else
			return 1.0 / (1.0 + std::exp(-value));
	}

	/**
	 * @brief 设置全局浮点比较精度阈值
	 *
	 * 修改全局变量EQACCURACY的值，用于后续equal/large/less等函数的浮点比较逻辑。
	 *
	 * @param accuracy 新的精度阈值，建议取值范围：1e-15 ~ 1e-3（过小易受浮点误差影响，过大精度不足）
	 * @note 1. inline特性：简化函数调用，提升执行效率；
	 *       2. 若传入负数，精度阈值会被重置为默认值1e-8；
	 *       3. 该函数修改全局变量，多线程场景需加锁保护
	 */
	inline void set_accuracy(double accuracy);

	// is_nan(double) 已于 v1.4.6.6 删除:与 ecflow-constant.h 的 is_empty() 完全同义。
	//   哨兵判定用 is_empty();数学 NaN 校验直接用 std::isnan()。见文件末尾说明与 代码规范.md §3。

	/**
	 * @brief 计算两个数组的欧式距离（L2距离）
	 *
	 * 计算等长数组对应元素的欧式距离，默认计算二维坐标（size=2）的距离，适用于空间距离度量。
	 *
	 * @param a 第一个数组指针，指向double类型数组首地址
	 * @param b 第二个数组指针，指向double类型数组首地址
	 * @param size 数组维度（长度），默认值2，需≥1（若size≤0，返回0.0）
	 * @return double 欧式距离结果，公式：sqrt(Σ(a[i]-b[i])²)（i从0到size-1）
	 * @note 1. inline特性：提升高频调用场景的效率；
	 *       2. 数组a/b需保证长度≥size，否则内存越界；
	 *       3. 若数组元素包含NaN，返回NaN
	 */
	double eu_distance(double a[], double b[], size_t size = 2);

	/**
	 * @brief 计算两个数组的曼哈顿距离（L1距离）
	 *
	 * 计算等长数组对应元素的曼哈顿距离，默认计算二维坐标（size=2）的距离，适用于网格路径度量等场景。
	 *
	 * @param a 第一个数组指针，指向double类型数组首地址
	 * @param b 第二个数组指针，指向double类型数组首地址
	 * @param size 数组维度（长度），默认值2，需≥1（若size≤0，返回0.0）
	 * @return double 曼哈顿距离结果，公式：Σ|a[i]-b[i]|（i从0到size-1）
	 * @note 1. inline特性：减少函数调用开销，适合小维度高频计算；
	 *       2. 数组a/b需保证长度≥size，否则内存越界；
	 *       3. 若数组元素包含NaN，返回NaN
	 */
	double man_distance(double a[], double b[], size_t size = 2);

	/**
	 * @brief 计算两个数组的切比雪夫距离（L∞距离）
	 *
	 * 计算等长数组对应元素的切比雪夫距离，默认计算二维坐标（size=2）的距离，适用于棋盘距离度量等场景。
	 *
	 * @param a 第一个数组指针，指向double类型数组首地址
	 * @param b 第二个数组指针，指向double类型数组首地址
	 * @param size 数组维度（长度），默认值2，需≥1（若size≤0，返回0.0）
	 * @return double 切比雪夫距离结果，公式：max(|a[i]-b[i]|)（i从0到size-1）
	 * @note 1. inline特性：提升小函数的执行效率；
	 *       2. 数组a/b需保证长度≥size，否则内存越界；
	 *       3. 若数组元素包含NaN，返回NaN
	 */
	double che_distance(double a[], double b[], size_t size = 2);

	/**
	 * @brief 计算两个数组的汉明距离（数值相似度度量）
	 *
	 * 统计两个数组中“不相等”元素的数量占比（归一化到[0,1]），默认使用全局精度EQACCURACY判断元素是否相等。
	 *
	 * @param a 第一个数组指针，指向double类型数组首地址
	 * @param b 第二个数组指针，指向double类型数组首地址
	 * @param size 数组维度（长度），默认值2，需≥1（若size≤0，返回0.0）
	 * @param accuracy 浮点比较精度阈值，默认使用全局变量EQACCURACY，可自定义(暂未生效)
	 * @return double 汉明距离结果，公式：不相等元素数 / size（元素相等判断：|a[i]-b[i]| < accuracy）
	 * @note 1. 数组a/b需保证长度≥size，否则内存越界；
	 *       2. 若数组元素包含NaN，该位置判定为“不相等”；
	 *       3. 返回值越接近0，数组相似度越高
	 */
	double hamming_distance(double a[], double b[], size_t size = 2, double accuracy = EQACCURACY);

	/**
	 * @brief 计算两个数组的余弦相似度
	 *
	 * 计算等长数组的余弦相似度，衡量向量方向的相似度，取值范围[-1,1]，适用于特征相似度匹配。
	 *
	 * @param a 第一个数组指针，指向double类型数组首地址（向量a）
	 * @param b 第二个数组指针，指向double类型数组首地址（向量b）
	 * @param size 数组维度（长度），默认值2，需≥1（若size≤0，返回0.0）
	 * @return double 余弦相似度结果，公式：(a·b) / (||a|| * ||b||)（点积 / 模长乘积）
	 * @note 1. 若其中一个向量模长为0（所有元素为0），返回0.0；
	 *       2. 数组a/b需保证长度≥size，否则内存越界；
	 *       3. 若数组元素包含NaN，返回NaN
	 */
	double cos_similarity(double a[], double b[], size_t size = 2);

	/**
	 * @brief 带精度的浮点数值相等判断
	 *
	 * 避免直接使用==判断浮点数相等，通过绝对值差小于精度阈值判定相等，解决浮点误差问题。
	 *
	 * @param a 第一个待比较的double数值
	 * @param b 第二个待比较的double数值
	 * @param accuracy 比较精度阈值，默认使用全局变量EQACCURACY，可自定义
	 * @return bool 相等返回true，否则返回false（判断逻辑：|a - b| < accuracy）
	 * @note 1. 若a或b为NaN，直接返回false；
	 *       2. 精度阈值越小，判断越严格，建议根据业务场景调整
	 */
	bool equal(double a, double b, double accuracy = EQACCURACY);

	/**
	 * @brief 带精度的浮点数值大于判断（a > b）
	 *
	 * 考虑浮点误差的“大于”判断，避免直接使用>导致的误差问题。
	 *
	 * @param a 第一个待比较的double数值
	 * @param b 第二个待比较的double数值
	 * @param accuracy 比较精度阈值，默认使用全局变量EQACCURACY，可自定义
	 * @return bool a严格大于b（扣除精度）返回true，否则返回false（判断逻辑：a - b > accuracy）
	 * @note 1. 若a或b为NaN，直接返回false；
	 *       2. 该判断为“严格大于”，若a-b≤accuracy则判定为不大于
	 */
	bool large(double a, double b, double accuracy = EQACCURACY);

	/**
	 * @brief 带精度的浮点数值小于判断（a < b）
	 *
	 * 考虑浮点误差的“小于”判断，避免直接使用<导致的误差问题。
	 *
	 * @param a 第一个待比较的double数值
	 * @param b 第二个待比较的double数值
	 * @param accuracy 比较精度阈值，默认使用全局变量EQACCURACY，可自定义
	 * @return bool a严格小于b（扣除精度）返回true，否则返回false（判断逻辑：b - a > accuracy）
	 * @note 1. 若a或b为NaN，直接返回false；
	 *       2. 该判断为“严格小于”，若b-a≤accuracy则判定为不小于
	 */
	bool less(double a, double b, double accuracy = EQACCURACY);

	/**
	 * @brief 带精度的浮点数值不大于判断（a ≤ b）
	 *
	 * 等价于“!large(a, b, accuracy)”，即a小于或等于b（扣除精度）。
	 *
	 * @param a 第一个待比较的double数值
	 * @param b 第二个待比较的double数值
	 * @param accuracy 比较精度阈值，默认使用全局变量EQACCURACY，可自定义
	 * @return bool a不大于b返回true，否则返回false（判断逻辑：a - b ≤ accuracy）
	 * @note 1. 若a或b为NaN，直接返回false；
	 *       2. 该判断包含“等于”场景，适合非严格大于的业务需求
	 */
	bool notlarge(double a, double b, double accuracy = EQACCURACY);

	/**
	 * @brief 带精度的浮点数值不小于判断（a ≥ b）
	 *
	 * 等价于“!less(a, b, accuracy)”，即a大于或等于b（扣除精度）。
	 *
	 * @param a 第一个待比较的double数值
	 * @param b 第二个待比较的double数值
	 * @param accuracy 比较精度阈值，默认使用全局变量EQACCURACY，可自定义
	 * @return bool a不小于b返回true，否则返回false（判断逻辑：b - a ≤ accuracy）
	 * @note 1. 若a或b为NaN，直接返回false；
	 *       2. 该判断包含“等于”场景，适合非严格小于的业务需求
	 */
	bool notless(double a, double b, double accuracy = EQACCURACY);

	/**
	 * @brief 智能截断浮点数为整数（截断向零 + 浮点噪声修正）
	 *
	 * 先判断该值是否“在精度内已是整数”，是则四舍五入（修正浮点噪声，如 2.9999999999 → 3），
	 * 否则**截断向零**。
	 *
	 * @param value 待截断的double类型数值
	 * @return int 截断后的整数结果
	 * @note 1. **是截断、不是四舍五入**：intelliTrunc(2.4)→2、intelliTrunc(2.6)→**2**、
	 *          intelliTrunc(2.9999999999)→3（噪声修正）。
	 *       2. **NaN/Inf 抛 std::invalid_argument；超出 int 范围抛 std::out_of_range**（非返回极值）。
	 *
	 * @warning v1.4.7 订正文档：原注释称“优先按四舍五入”“intelliTrunc(2.5)→3”“NaN 返回 0”
	 *          “超范围返回极值”，**四条与实现均不符**（2.5 → 2；NaN/越界皆抛）。
	 */
	int intelliTrunc(double value);


	//====================== 头内 inline 定义(MATH-INLINE 修复) ======================
	// 原 normalization / set_accuracy 均"inline 声明 + ecflow-math.cpp 非 inline 定义" → 跨库未解析。
	// 统一移入头内真 inline(定义已从 ecflow-math.cpp 移除);置于末尾以便引用前置声明(equal)。忠实保留原逻辑。

	//   (int 版的商在细步长下会越范围抛异常)。snap 改用 std::llround(就近)后本函数零调用;
	//   且就近天然处理浮点噪声(llround(2.9999999999)=3),不再需要"截断 + 噪声特判"那套。
	//   构成平台第二个 NaN 判定入口(第三个是 ecflow-graph-constrain.h 的 _decided,已一并归并)。
	//   三个同名异形的判定函数正是"哨兵判定写法混乱"的土壤。全仓收敛:
	//     * **哨兵判定**(某值是否为 EMPTYVALUE/未设置)→ `is_empty()`(ecflow-constant.h,与哨兵同处);
	//     * **数学 NaN**(输入校验 / 模块自用 N/A 标记)→ 直接 `std::isnan()`,并注明理由。
	//   其唯一调用方 set_accuracy 属后者,已改用 std::isnan(与其紧邻的 std::isinf 对称)。详见 代码规范.md §3。

	inline void set_accuracy(double accuracy)
	{
		// 数学 NaN 的输入校验(非"未设置哨兵"判定)→ 用 std::isnan,与紧邻的 std::isinf 对称。
		if (std::isnan(accuracy))
			throw std::invalid_argument("set_accuracy: accuracy is NaN (not a number)");
		if (std::isinf(accuracy))
			throw std::invalid_argument("set_accuracy: accuracy is infinite");
		if (accuracy <= 0.0)
			throw std::invalid_argument("set_accuracy: accuracy must be positive (>" + std::to_string(0.0) + ")");
		if (accuracy > std::numeric_limits<double>::max())
			throw std::invalid_argument("set_accuracy: accuracy exceeds maximum double value");
		EQACCURACY = accuracy;
	}

	inline void normalization(double* input, double* output, size_t length)
	{
		if (input == nullptr)
			throw std::invalid_argument("normalization: input pointer cannot be nullptr");
		if (output == nullptr)
			throw std::invalid_argument("normalization: output pointer cannot be nullptr");
		if (length == 0)
			throw std::invalid_argument("normalization: length cannot be 0");

		double min_value = *std::min_element(input, input + length);
		double max_value = *std::max_element(input, input + length);

		// 最小值与最大值相等(所有元素相同)→ 全置 0
		if (equal(min_value, max_value)) {
			std::fill(output, output + length, 0.0);
			return;
		}

		const double denominator = max_value - min_value;
		for (size_t i = 0; i < length; i++)
			output[i] = (input[i] - min_value) / denominator;
	}
}