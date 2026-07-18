#include"ecflow-calculator.h"
#include "objective.h"

using namespace ECFlow;

// Ĭ�Ϲ��캯������ʼ��Ŀ�����Ĭ������
Objective::Objective()
{
    _name = "none";
    _priority = -1;
    _min_is_better = true;
    _calculator = nullptr;
}

// ���������캯������ʼ���������ԣ�������Ĭ�Ͽ�ָ��
Objective::Objective(std::string name, int priority, bool min_is_better)
{
    _name = name;
    _priority = priority;
    _min_is_better = min_is_better;
    _calculator = nullptr;
}

// �����������ͷż������ڴ棬�����ڴ�й©
Objective::~Objective()
{
    delete _calculator;
}

// ��ȡ�Ż����򣺷��ؼ�С��/���󻯱��
bool Objective::IsMin() const
{
    return _min_is_better;
}

// ��ȡĿ�����ƣ����������ַ���
std::string Objective::getName() const
{
    return _name;
}

// �������ȼ�������Ŀ�����ȼ���ֵ
void Objective::setPriority(int priority)
{
    _priority = priority;
}

// ��ȡ���ȼ������ص�ǰ���ȼ���ֵ
int Objective::priority() const
{
    return _priority;
}

// �󶨼��������ӹܴ���ָ�������Ȩ
void Objective::setCalculator(Calculator* c_pointer)
{
    _calculator = c_pointer;
}

// ������Ӧ�ȣ����ü�������run����������
double Objective::getFitness(double** solution)
{
    double result;
    _calculator->run(solution, &result);
    return result;
}

// ����������������ԣ�������������⹲��ʵ��
void Objective::copy(Objective* source)
{
    _name = source->_name;
    _min_is_better = source->_min_is_better;
    _priority = source->_priority;
    _calculator = source->_calculator->copy();
}

// ����<������������ȼ��������򣨵�ǰ���ȼ�>�������ȼ�����true��
bool Objective::operator<(const Objective& a) const
{
    return _priority > a._priority;
}