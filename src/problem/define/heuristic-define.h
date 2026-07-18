//------------------------Description------------------------
// HeuristicDefine:问题启发(inspiration)域的定义切片(原单体 Problem 的启发部分)。
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
#include "ecflow-inspiration.h"
#include "ecflow-calculator.h"
#include "ecflow-functor.hpp"
#include "ExprTree.h"
#include "variable.h"
#include "ecflow-basicfunc.h"   // stringSplit
#include "calc-spec.h"
#include "heuristic-manager.h"
#include "variable-manager.h"
#include "constraint-manager.h"

namespace ECFlow
{
    enum class HeuristicKind { random, boundary, func, matrix, expression };

    class HeuristicDefine
    {
    private:
        struct Spec
        {
            HeuristicKind kind = HeuristicKind::random;
            std::string   target_variable;
            std::vector<std::string> input_variables;
            bool          stable = false;
            CalcSpec      calc;
            std::string   formula;
            std::vector<double> matrix; int matrix_rows = 0, matrix_cols = 0;
        };
        std::vector<Spec> _specs;

        static std::vector<std::string> _split(const std::string& s)
        { std::vector<std::string> v; if (!s.empty()) stringSplit(s, ',', v); return v; }

        static Inspiration* _buildInspiration(const Spec& d)
        {
            switch (d.kind)
            {
            case HeuristicKind::random:   return new RandomInspiration();
            case HeuristicKind::boundary: return new BoundaryInspiration();
            case HeuristicKind::func:
            {
                const int n = static_cast<int>(d.input_variables.size()) + 2;
                Calculator* c = (d.calc.kind == CalcKind::functor)
                    ? static_cast<Calculator*>(new FunctorCalculator(d.calc.functor.get(), n, 1))
                    : static_cast<Calculator*>(new FuncCalculator(d.calc.func, n, 1));
                auto* ni = new NormalInspiration(c); delete c; return ni;
            }
            case HeuristicKind::matrix:
                return new StableInspiration(const_cast<double*>(d.matrix.data()), d.matrix_rows, d.matrix_cols);
            case HeuristicKind::expression:
                return new InspirationExpression(d.formula, static_cast<int>(d.input_variables.size()));
            default: return new RandomInspiration();
            }
        }

    public:
        void addInspirationRandom(const std::string& v)   { Spec d; d.kind = HeuristicKind::random;   d.target_variable = v; _specs.push_back(std::move(d)); }
        void addInspirationBoundary(const std::string& v) { Spec d; d.kind = HeuristicKind::boundary; d.target_variable = v; _specs.push_back(std::move(d)); }
        void addInspirationFunc(const std::string& v, const std::string& input, double (*func)(double**), bool stable = false)
        { Spec d; d.kind = HeuristicKind::func; d.target_variable = v; d.input_variables = _split(input); d.stable = stable; d.calc.kind = CalcKind::func_ptr; d.calc.func = func; _specs.push_back(std::move(d)); }
        void addInspirationFunc(const std::string& v, const std::string& input, eccalcul_functor* func, bool stable = false)
        { Spec d; d.kind = HeuristicKind::func; d.target_variable = v; d.input_variables = _split(input); d.stable = stable; d.calc.kind = CalcKind::functor; d.calc.functor.reset(func->copy()); _specs.push_back(std::move(d)); }
        void addInspirationMatrix(const std::string& v, double* matrix, int nDec, int nDim)
        { Spec d; d.kind = HeuristicKind::matrix; d.target_variable = v; d.matrix.assign(matrix, matrix + (size_t)nDec * nDim); d.matrix_rows = nDec; d.matrix_cols = nDim; _specs.push_back(std::move(d)); }
        void addInspirationExpr(const std::string& v, const std::string& input, const std::string& formula, bool stable = false)
        { Spec d; d.kind = HeuristicKind::expression; d.target_variable = v; d.input_variables = _split(input); d.stable = stable; d.formula = formula; _specs.push_back(std::move(d)); }

        void clear() { _specs.clear(); }   // reset definition state
        int getHeuristicNumber() const { return static_cast<int>(_specs.size()); }

        bool checkCompleteness(const VariableManager& var) const
        {
            for (const auto& d : _specs)
            {
                if (var.getVariableId(d.target_variable) < 0) return false;
                for (const auto& vn : d.input_variables) if (var.getVariableId(vn) < 0) return false;
            }
            return true;
        }

        HeuristicManager compile(const VariableManager& var, const ConstraintManager& con) const
        {
            HeuristicManager rt;
            const int n_var = var.variable_number;
            rt.variable_number = n_var;
            rt.inspirations = new Inspiration*[n_var];
            rt.inspiration_variable_number = new int[n_var];
            rt.inspiration_variable_index = new int*[n_var];
            rt.inspirate_buffer = new double*[n_var + 2];
            for (int i = 0; i < n_var; i++) { rt.inspirations[i] = nullptr; rt.inspiration_variable_number[i] = 0; rt.inspiration_variable_index[i] = nullptr; }

            for (int i = 0; i < n_var; i++)
            {
                bool is_dec = false;
                for (int k = 0; k < var.decision_variable_number; k++) if (var.decision_variable_index[k] == i) { is_dec = true; break; }
                if (!is_dec) continue;

                const std::string& vname = var.variables[i].note._name;
                bool connected = false;
                for (int j = 0; j < (int)_specs.size(); j++)
                {
                    const Spec& d = _specs[j];
                    if (d.target_variable != vname) continue;
                    const int iv = static_cast<int>(d.input_variables.size());
                    rt.inspiration_variable_number[i] = iv;
                    rt.inspiration_variable_index[i] = new int[iv];
                    for (int k = 0; k < iv; k++) rt.inspiration_variable_index[i][k] = var.getVariableId(d.input_variables[k]);

                    Inspiration* ins = _buildInspiration(d);
                    if (d.stable)
                    {
                        auto* ni = dynamic_cast<NormalInspiration*>(ins);
                        if (ni)
                        {
                            for (int k = 0; k < iv; k++) rt.inspirate_buffer[k] = var.variables[rt.inspiration_variable_index[i][k]].address;
                            int dvi = var.variable_map_index[i];
                            interval_set ini_dom = con.feasible_regions_ini[dvi];
                            domain_view dv(ini_dom, var.variables[i].getLowbound(), var.variables[i].getAccuracy(),
                                            viewModeOf(var.getVariableType(i)));
                            rt.inspirations[i] = ni->toStable(dv, var.variables[i].getLength(), rt.inspirate_buffer);
                            delete ins;
                        }
                        else rt.inspirations[i] = ins;
                    }
                    else rt.inspirations[i] = ins;
                    connected = true; break;
                }
                if (!connected)
                {
                    rt.inspirations[i] = new RandomInspiration();
                    rt.inspiration_variable_number[i] = 0;
                    rt.inspiration_variable_index[i] = nullptr;
                }
            }
            return rt;
        }
    };
}
