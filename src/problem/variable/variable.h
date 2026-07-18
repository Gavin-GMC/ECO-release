//------------------------Description------------------------
// This file defines the element object in ECFlow,
//  which consists of two parts: value and note 
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include<string>

namespace ECFlow
{

	// 对于离散优化问题，可大致分为三类，分配优化问题，序列优化问题和子集优化问题
	// 对于分配优化问题，其选择取决于当前的维度
	// 对于序列优化问题，其选择取决于之前选择
	// 对于子集优化问题，上述两种情况都有可能出现
	//
	// 变量类型:**由用户在 addVariable 时声明**,是"该变量是离散还是连续"的**唯一判定依据**
	//   (不靠 accuracy 之类的启发式猜测)。域层据此选择视图模式,见 ConstraintManager::demView。
	enum class VariableType
	{
		// —— 离散族:值域为网格点 lowbound + k*accuracy ——
		discrete,              // 普通离散变量明摆着按离散格点理解它 → 语义含混,故明确化)
		allocation,            // 指派(每维→一个类别)
		sequence_direction,    // 序列(有向)
		sequence_bidiagraph,   // 序列(双向图)
		sub_module,            // 子模块

		// —— 连续族——
		// 值域为**实区间**,不存在"格点"概念。accuracy 对其含义变为**输出精度**(见 ElementNote::_accuracy)。
		// ⚠ 与 SetPSO(集合速度表)/ACO(信息素矩阵)**结构性不兼容** —— 二者均要求"每维候选可枚举",
		//   连续变量上该前提不成立。装配期耦合检测见 未来规划 的 ASSEMBLY-COUPLING TODO(本轮不做)。
		continuous
	};

	// 元素注释对象，包含上下界，精确度等信息
	struct ElementNote
	{
	public:
		std::string _name;
		int _length;
		int _shape[2];  // 静态数组无需初始化，默认值由构造函数赋值
		double _upbound;
		double _lowbound;
		// ⚠ **含义随 _type 而变**:
		//   * 离散族 → **网格步长**:值域 = {lowbound + k*accuracy};候选可枚举;生成的值被吸附到格点。
		//   * continuous → **"多细算同一个值"的粒度**:值域是实区间、无格点概念。
		//     **不作用于存储值** —— 连续变量的决策值原样保留、不被量化(见 domain_view::snap)。
		//     它只用于:①界定"删掉一个值"的邻域(remove_point 删该粒度的半步胞 —— 连续域上删单点
		//     是零测度的空操作,约束传播会失效);②输出/展示的精度。
		double _accuracy;
		VariableType _type;

		// 构造函数声明
		ElementNote();

		bool operator==(const ElementNote& a)const
		{
			return _name == a._name && _length == a._length;
		}
	};
}