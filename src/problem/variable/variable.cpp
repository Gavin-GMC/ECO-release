// SPDX-License-Identifier: BSD-3-Clause

#include"evoflow-constant.h"

#include"variable.h"

using namespace EvoFlow;

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
