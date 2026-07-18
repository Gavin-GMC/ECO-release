#include"ecflow-calculator.h"

#include"ExprTree.h"
#include<stdexcept>

using namespace ECFlow;

TreeCalculator::TreeCalculator(ExpressionTree* tree, int parameter_number, int result_size)
	:Calculator(parameter_number, result_size)
{
	_tree = tree->copy();

}

TreeCalculator::TreeCalculator(std::string expression, ElementNote* notes, int parameter_number, int result_size)
	:Calculator(parameter_number, result_size)
{
	_tree = new ExpressionTree();
	_tree->BatchLinkVariables(notes, nullptr, parameter_number);
	if (!_tree->Compile(expression))
	{
		delete _tree;
		_tree = nullptr;
		throw std::invalid_argument("TreeCalculator: failed to compile expression \"" + expression + "\"");
	}
}

TreeCalculator::~TreeCalculator()
{
	delete _tree;
}

void TreeCalculator::run(double** input, double* output)
{
	_tree->UpdateVariables(const_cast<const double**>(input));
	_tree->Calculate();
	_tree->getResult(output);
}

Calculator* TreeCalculator::TreeCalculator::copy()
{
	return new TreeCalculator(_tree, _parameter_number, _result_size);
}
