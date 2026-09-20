//------------------------Description------------------------
// ObjectiveDefine:问题目标域的定义切片(原单体 Problem 的目标部分)。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <vector>
#include <string>
#include <stdexcept>
#include "objective.h"
#include "evoflow-calculator.h"
#include "evoflow-functor.hpp"
#include "ExprTree.h"
#include "variable.h"
#include "evoflow-basicfunc.h"   // stringSplit
#include "calc-spec.h"
#include "objective-manager.h"
#include "problem-info.h"

namespace EvoFlow
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
        static Calculator* _buildCalculator(const CalcSpec& spec,
                                            const std::vector<std::string>& names,
                                            const VariableManager& var)
        {
            const int n = static_cast<int>(names.size());
            switch (spec.kind)
            {
            case CalcKind::func_ptr: return new FuncCalculator(spec.func, n, 1);
            case CalcKind::functor:  return new FunctorCalculator(spec.functor.get(), n, 1);
            case CalcKind::formula:
            {
                std::vector<ElementNote> notes(n);
                for (int i = 0; i < n; i++)
                {
                    int vid = var.getVariableId(names[i]);
                    if (vid < 0)
                        throw std::invalid_argument(
                            "ObjectiveDefine::compile: formula input variable not found - "
                            + names[i]);
                    // TreeCalculator uses ElementNote shape while compiling indexed
                    // variables. Copy the real note so x[1], matrices and constants
                    // retain their declared extents; the old synthetic length=1 note
                    // made every index above zero fail before evaluation.
                    notes[i] = var.variables[vid].note;
                }
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
        std::vector<ProblemObjectiveInfo> describeObjectives() const
        {
            std::vector<ProblemObjectiveInfo> out;
            out.reserve(_specs.size());
            for (const Spec& s : _specs)
            {
                ProblemObjectiveInfo o;
                o.name = s.name;
                o.variables = s.input_variables;
                o.minimize = s.min_is_better;
                o.priority = s.priority;
                out.push_back(std::move(o));
            }
            return out;
        }

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
                tmp.setCalculator(_buildCalculator(s.calc, s.input_variables, var));   // may throw (bad formula)
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
