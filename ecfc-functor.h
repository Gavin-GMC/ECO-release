#pragma once
#include<functional>

namespace ECFC
{
// 想要的是什么？
// 在问题模板中，作为参数传入的函数必须是全局、静态的
// 但这样会导致全局环境下只能存在一个同类问题
// （函数只有一个，重复的定义会导致错误）
// 因此能不能通过对象使同一个问题模板的不同示例间实现数据、评估函数。启发式函数等的分离
// 
// 实现思路：
// 基于包含函数指针的仿函数？不行的，仿函数之间只实现了数据的分离，但还是要传入函数指针？
// 不传函数指针，直接在仿函数内写死
// 但这样仿函数类型不同，怎么处理呢？
// 在基础模块里做一个仿函数的基类，后续的都从这里继承
// 
// 其余细节：
// 是做成模板还是一个普通的类呢，模板吧，方便后续的统一
// 算了，模板中间加
// 
// 所以就有后面这个类啦！
// 这个类又没有啦！因为直接用STL的模板啦！
// 这个类又有啦，因为STL的模板太烂啦！
// 
//
	/*
	// 一元仿函数
	template <class _Arg, class _Result>
	struct unary_ecfunctor : std::unary_function<_Arg, _Result>
	{
		unary_ecfunctor() {}

		virtual _Result operator()(const _Arg& a) const = 0;

		virtual unary_ecfunctor* copy() = 0;
	};
	*/

	// 供calculator使用的仿函数
	struct eccalcul_functor : std::unary_function<double, double>
	// struct eccalcul_functor
	{
		eccalcul_functor() {}

		virtual ~eccalcul_functor()
		{

		}

		virtual double operator()(double** a) const = 0;

		virtual eccalcul_functor* copy() = 0;
	};
	
}