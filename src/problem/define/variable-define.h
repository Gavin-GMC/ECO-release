//------------------------Description------------------------
// VariableDefine:问题变量域的定义切片(原单体 Problem 的变量部分)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <vector>
#include <string>
#include <stdexcept>
#include "variable.h"            // ElementNote
#include "variable-manager.h"     // VariableManager (compile product)

namespace ECFlow
{
    enum class VarGenType { constant, decision /*, calculated [未来特性] */ };

    class VariableDefine
    {
    private:
        struct Spec
        {
            ElementNote         note;
            VarGenType          type = VarGenType::decision;
            std::vector<double> const_values;   // constant only
        };
        std::vector<Spec> _specs;

        void _checkDuplicate(const std::string& name) const
        {
            for (const auto& s : _specs)
                if (s.note._name == name)
                    throw std::invalid_argument(
                        "VariableDefine: variable name duplicated - " + name);
        }

    public:
        // --- definition editing (same signatures as the original Problem.addXxx) ---
        int addConstant(const std::string& name, const double* values, int length, int height = 1)
        {
            _checkDuplicate(name);
            Spec s;
            s.note._name = name;
            s.note._length = length * height;
            s.note._shape[0] = length; s.note._shape[1] = height;
            s.type = VarGenType::constant;
            s.const_values.assign(values, values + length * height);
            int id = static_cast<int>(_specs.size());
            _specs.push_back(std::move(s));
            return id;
        }

        int addVariable(const std::string& name, double low_bound, double up_bound, double accuracy,
                        int length, int height = 1, VariableType type = VariableType::discrete)
        {
            _checkDuplicate(name);
            Spec s;
            s.note._name = name;
            s.note._lowbound = low_bound; s.note._upbound = up_bound; s.note._accuracy = accuracy;
            s.note._length = length * height;
            s.note._shape[0] = length; s.note._shape[1] = height;
            s.note._type = type;
            s.type = VarGenType::decision;
            int id = static_cast<int>(_specs.size());
            _specs.push_back(std::move(s));
            return id;
        }

        // --- queries on the definition (build-independent) ---
        void clear() { _specs.clear(); }   // reset definition state
        int getVariableNumber() const { return static_cast<int>(_specs.size()); }
        int getVariableId(const std::string& name) const
        {
            for (int i = 0; i < (int)_specs.size(); i++)
                if (_specs[i].note._name == name) return i;
            return -1;
        }
        // 供 compile 时"域进约束"用:取第 i 个变量名 / 是否为决策变量(常量无需域约束)
        const std::string& getVariableName(int i) const { return _specs[i].note._name; }
        bool isDecisionVariable(int i) const { return _specs[i].type == VarGenType::decision; }

        // --- compile: definition -> runtime engine ---
        VariableManager compile() const
        {
            VariableManager vm;
            const int n = static_cast<int>(_specs.size());
            vm.variable_number = n;
            vm.variables.resize(n);
            vm.owned_values.resize(n);
            vm.variable_map_index.assign(n, -1);

            for (int i = 0; i < n; i++)
            {
                const Spec& s = _specs[i];
                vm.variables[i].note = s.note;
                if (s.type == VarGenType::decision)
                {
                    vm.variable_map_index[i] = static_cast<int>(vm.decision_variable_index.size());
                    vm.decision_variable_index.push_back(i);
                    vm.decision_variable_offset.push_back(vm.problem_size);
                    for (int t = 0; t < s.note._length; t++)
                        vm.solution_belong_variable.push_back(i);
                    vm.problem_size += s.note._length;
                    vm.variables[i].address = nullptr;   // bound later by setResult
                }
                else // constant
                {
                    vm.owned_values[i] = s.const_values;
                    vm.variables[i].address =
                        vm.owned_values[i].empty() ? nullptr : vm.owned_values[i].data();
                }
            }
            vm.decision_variable_number = static_cast<int>(vm.decision_variable_index.size());
            return vm;
        }
    };
}
