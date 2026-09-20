// SPDX-License-Identifier: BSD-3-Clause

#include"evoflow-calculator.h"
#include"evoflow-functor.hpp"

using namespace EvoFlow;

// 构造函数实现
FuncCalculator::FuncCalculator(double (*function)(double** input), int parameter_number, int result_size)
    : Calculator(parameter_number, result_size)  // 调用基类构造
{
    _function = function;
}

// 析构函数实现
FuncCalculator::~FuncCalculator()
{
    // 空实现（函数指针无需释放）
}

// run方法实现
void FuncCalculator::run(double** input, double* output)
{
    *output = _function(input);
}

// copy方法实现
Calculator* FuncCalculator::copy()
{
    return new FuncCalculator(_function, _parameter_number, _result_size);
}

FunctorCalculator::FunctorCalculator(eccalcul_functor* function, int parameter_number, int result_size)
	:Calculator(parameter_number, result_size)
{
	_function = function->copy();
}

FunctorCalculator::~FunctorCalculator()
{
	delete _function;
}

void FunctorCalculator::run(double** input, double* output)
{
	*output = (*_function)(input);
}

Calculator* FunctorCalculator::copy()
{
	// 注:构造函数本身就 `_function->copy()` 深拷 functor,故此处传裸指针即得独立副本——
	//   无共享、无 double-free。真正需要处理的是 functor 内部经 shared_ptr 持有的**有状态**子对象
	//   (如 CEC 的 CecFunction/Benchmarks),那一层由各 functor 的 copy()/clone() 负责(见 evaluateFunc)。
	return new FunctorCalculator(_function, _parameter_number, _result_size);
}
