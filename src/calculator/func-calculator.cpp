#include"ecflow-calculator.h"
#include"ecflow-functor.hpp"

using namespace ECFlow;

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
	return new FunctorCalculator(_function, _parameter_number, _result_size);
}
