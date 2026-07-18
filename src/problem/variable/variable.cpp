#include"ecflow-constant.h"

#include"variable.h"

using namespace ECFlow;

ElementNote::ElementNote()
    : _name("none"),
    _length(0),
    _upbound(EMPTYVALUE),
    _lowbound(EMPTYVALUE),
    _accuracy(0),
    _type(VariableType::discrete)
{
    _shape[0] = 0;
    _shape[1] = 0;
}