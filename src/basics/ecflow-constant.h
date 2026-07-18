//------------------------Description------------------------
// This file is consisted of some basic, necessary, widely used
//  constants and control variables.
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once

#include <cmath> 
#include <limits>
namespace ECFlow
{
	// ===================== 基础常量/宏定义 =====================
	/**
	 * @brief ECFlow相关整数的最大值（对应int类型的最大值）
	 * 取值为2147483647，是32位有符号整型（int）的上限值，用于限制ECFlow模块中整数变量的最大值
	 */
#define ECFLOW_MAX 2147483647

	 /**
	  * @brief ECFlow相关整数的最小值
	  * 取值为-2147483647（注：32位有符号整型的理论最小值为-2147483648，此处根据项目需求调整），
	  * 用于限制ECFlow模块中整数变量的最小值
	  */
#define ECFLOW_MIN -2147483647

	  /**
	   * @brief 自然常数e的常量定义（高精度）
	   * 取值保留15位小数（2.718281828459045），用于ECFlow模块中的指数运算、对数运算等数学计算
	   */
#define E_CONST 2.718281828459045

	   /**
		* @brief 圆周率π的常量定义（高精度）
		* 取值保留13位小数（3.1415926535624），用于ECFlow模块中的几何计算、角度转换等场景
		*/
#define PI_CONST 3.1415926535624

		/**
		 * @brief 系统允许定义的最大变量数目
		 * 全局宏定义，限制用户可自定义变量的总数，超出该数值的变量定义会被拒绝
		 */
#define MAXVARIABLE 100 // 简化注释（行内）：可定义的变量数目

		 /**
		  * @brief 配置项中允许的最大参数数量
		  * 全局常量，限制单次配置操作中可传入的参数总数，用于参数列表的边界校验
		  */
	const int PARANUM = 100; // 设置中允许的参数数目

	/**
	 * @brief 子参数的最大数量
	 * 全局常量，用于限制单个主参数下可包含的子参数数目，如PARANUM下每个参数的子参数上限
	 */
	const int SUBPARA = 25;

	/**
	 * @brief PK模块的缓冲区/数据长度上限
	 * 全局常量，限制PK相关数据结构的大小，防止内存溢出或数据截断
	 */
	const int PKSIZE = 100;

	/**
	 * @brief PF模块的缓冲区/数据长度上限
	 * 全局常量，限制PF相关数据结构的大小，适配PF模块的业务数据存储需求
	 */
	const int PFSIZE = 200;

	/**
	 * @brief 空值标记（无效值）——标记变量"未设置/未决定/无效"的状态。
	 *
	 * @warning **判定必须用 is_empty()（见下），严禁裸 == / != 比较。**
	 *   本哨兵取值为 quiet_NaN，而 NaN 参与的一切比较运算均为假：
	 *     `v == EMPTYVALUE` **恒假**（守卫永不触发）、`v != EMPTYVALUE` **恒真**（守卫永远放行）。
	 *   两者都不会报错，只是行为**静默退化** —— 这正是本哨兵最危险之处。
	 *
	 * @note 历史沿革（务必知悉，否则会重蹈覆辙）：本常量**曾经是普通哨兵整数 -2139062144**，
	 *   那时 `v == EMPTYVALUE` 是**正确**写法。改为 quiet_NaN 后，全仓所有裸 == 比较**在同一刻静默失效**，
	 *   且旧注释长期未同步、持续诱导新代码照旧写 ==。已知伤员（均为事后逐个发现修复）：
	 *   ACO `tao_ini`（信息素全 NaN，整策略坏）、Difference `factor`（自适应永不启用）、
	 *   BPSO/PSO/SetPSO `w_attenuation`（随机惯性分支死）、约束检查/比较器/基数约束。
	 *   **若再次变更哨兵实现，必须同步全仓判定点。**
	 */
	const double EMPTYVALUE = std::numeric_limits<double>::quiet_NaN();

	/**
	 * @brief 判定 double 是否为 EMPTYVALUE（未设置哨兵）。**这是唯一正确的判定方式。**
	 *   与 EMPTYVALUE 定义在一起，确保"凡能看见哨兵的地方就能看见判定函数"。
	 */
	inline bool is_empty(double v) { return std::isnan(v); }

	/**
	 * @brief 等式计算的精度阈值
	 * 全局变量（建议改为const），用于判断两个浮点数是否相等（如a-b < EQACCURACY则视为相等），
	 * 取值1e-8（10的-8次方），兼顾计算精度和性能
	 */
	extern double EQACCURACY;
}

