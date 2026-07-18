//------------------------Description------------------------
// ObjectiveDefine:问题目标域的定义切片(原单体 Problem 的目标部分)。
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
#include "objective.h"
#include "ecflow-calculator.h"
#include "ecflow-functor.hpp"
#include "ExprTree.h"
#include "variable.h"
#include "ecflow-basicfunc.h"   // stringSplit
#include "calc-spec.h"
#include "objective-manager.h"

namespace ECFlow
{
    class ObjectiveDefine
    {
    private:
        struct Spec
        {
            std::string name;
            int  priority = 0;
            bool min_is_better = true;
            std::vector<std::string> input_variables;
            CalcSpec calc;
        };
        std::vector<Spec> _specs;
        bool _full_evaluate = true;   // initial evaluation mode carried into the compiled manager

        static std::vector<std::string> _split(const std::string& s)
        {
            std::vector<std::string> v;
            if (!s.empty()) stringSplit(s, ',', v);
            return v;
        }
        void _checkDuplicate(const std::string& name) const
        {
            for (const auto& s : _specs)
                if (s.name == name)
                    throw std::invalid_argument("ObjectiveDefine: duplicate objective name - " + name);
        }
        static Calculator* _buildCalculator(const CalcSpec& spec, const std::vector<std::string>& names)
        {
            const int n = static_cast<int>(names.size());
            switch (spec.kind)
            {
            case CalcKind::func_ptr: return new FuncCalculator(spec.func, n, 1);
            case CalcKind::functor:  return new FunctorCalculator(spec.functor.get(), n, 1);
            case CalcKind::formula:
            {
                std::vector<ElementNote> notes(n);
                for (int i = 0; i < n; i++) { notes[i]._name = names[i]; notes[i]._length = 1; }
                return new TreeCalculator(spec.formula, notes.data(), n, 1);
            }
            default: return nullptr;
            }
        }

        void _push(const std::string& name, int priority, bool min, const std::string& inputs, const CalcSpec& calc)
        {
            _checkDuplicate(name);
            Spec s; s.name = name; s.priority = priority; s.min_is_better = min;
            s.input_variables = _split(inputs); s.calc = calc;
            _specs.push_back(std::move(s));
        }

    public:
        void addObjective(const std::string& name, int priority, bool min_is_better,
                          const std::string& input_elements, double (*func)(double**))
        { CalcSpec c; c.kind = CalcKind::func_ptr; c.func = func; _push(name, priority, min_is_better, input_elements, c); }

        void addObjective(const std::string& name, int priority, bool min_is_better,
                          const std::string& input_elements, eccalcul_functor* functor)
        { CalcSpec c; c.kind = CalcKind::functor; c.functor.reset(functor->copy()); _push(name, priority, min_is_better, input_elements, c); }

        void addObjective(const std::string& name, int priority, bool min_is_better,
                          const std::string& input_elements, const std::string& formula)
        { CalcSpec c; c.kind = CalcKind::formula; c.formula = formula; _push(name, priority, min_is_better, input_elements, c); }

        void clear() { _specs.clear(); _full_evaluate = true; }   // reset specs + own flags
        int getObjectiveNumber() const { return static_cast<int>(_specs.size()); }
        std::string getObjectiveName(int oid) const { return _specs[oid].name; }

        // Optional definition-side evaluation mode (2.5): set on the definition and
        // carried into the manager by compile(); the manager can also change it later.
        void setEvaluateMode(bool full) { _full_evaluate = full; }
        bool getEvaluateMode() const    { return _full_evaluate; }

        // Every referenced variable must exist in the (already compiled) variable engine.
        bool checkCompleteness(const VariableManager& var) const
        {
            for (const auto& s : _specs)
                for (const auto& vn : s.input_variables)
                    if (var.getVariableId(vn) < 0) return false;
            return true;
        }

        // compile: definition -> runtime engine (builds Objective + Calculator from spec).
        ObjectiveManager compile(const VariableManager& var) const
        {
            const int n = static_cast<int>(_specs.size());

            // A valid problem must have at least one objective that participates in
            // comparison (priority >= 0). No objectives at all, or every objective with
            // priority < 0, leaves nothing to optimize -> illegal problem definition.
            // Enforced unconditionally (independent of setCompileCheck), as a structural
            // invariant rather than an optional completeness check.
            int participating = 0;
            for (const auto& s : _specs) if (s.priority >= 0) ++participating;
            if (participating == 0)
                throw std::invalid_argument(
                    "ObjectiveDefine::compile: illegal problem definition - no objective "
                    "participates in comparison (objectives empty or all priority < 0)");

            ObjectiveManager om;
            om.object_number = n;
            om.objectives = new Objective[n];   // default-constructed (calc=null) -> exception-safe
            om.objective_variable_index.resize(n);
            om.evaluate_buffer.assign(var.getVariableNumber(), nullptr);

            for (int i = 0; i < n; i++)
            {
                const Spec& s = _specs[i];
                Objective tmp(s.name, s.priority, s.min_is_better);
                tmp.setCalculator(_buildCalculator(s.calc, s.input_variables));   // may throw (bad formula)
                om.objectives[i].copy(&tmp);
                om.objective_variable_index[i].resize(s.input_variables.size());
                for (int j = 0; j < (int)s.input_variables.size(); j++)
                    om.objective_variable_index[i][j] = var.getVariableId(s.input_variables[j]);
            }
            if (n > 0) om.comparer = new Comparer(om.objectives, n);   // Comparer sorts; 0 objectives -> none
            om.full_evaluate = _full_evaluate;       // carry initial evaluation mode (2.5)
            om.rebuildPositiveIndex();               // participating set (priority >= 0)
            return om;
        }
    };
}
