//------------------------Description------------------------
// 取值域限制类约束:把变量取值限定到区间/集合(可逐维不同),并据此缩减域(CP 的 domain/unary 约束)。
//-------------------------Reference-------------------------
// CP 对应:in-interval、in-set(unary domain 约束)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------
#pragma once
#include <vector>
#include <cstring>
#include "ecflow-constrain.h"   // Constrain, ConstrianLevel, domain_view, interval_set, equal()

namespace ECFlow
{
    // ---------------------------------------------------------------
    // Range constraint: value must be in [left, right]
    // ---------------------------------------------------------------
    class ConstrainRange final : public Constrain
    {
    private:
        double _left, _right;
    public:
        ConstrainRange(double penalty_w, double left, double right)
            : Constrain(penalty_w), _left(left), _right(right) {}

        void ini() override {}

        bool meet(int, double value) override
        {
            return value >= _left && value <= _right;
        }

        void update(int, double) override {}

        double violation(double* variables, int size) override
        {
            double back = 0;
            for (int j = 0; j < size; j++)
            {
                if (variables[j] < _left)       back += _left - variables[j];
                else if (variables[j] > _right) back += variables[j] - _right;
            }
            return back;
        }

        void regionReduction(int, domain_view& region) override
        {
            region.restrict(_left, _right);
        }

        ConstrianLevel getConstrainLevel() override { return constraints_range; }
        Constrain* clone() override { return new ConstrainRange(_w, _left, _right); }
    };

    // ---------------------------------------------------------------
    // Compatibility constraint: value must be one of a set of allowed values (所有维同一集合)
    // ---------------------------------------------------------------
    class ConstrainCompatibility final : public Constrain
    {
    private:
        double* _values;
        int     _length;
    public:
        ConstrainCompatibility(double penalty_w, const double* values, int length)
            : Constrain(penalty_w), _length(length)
        {
            _values = new double[_length];
            memcpy(_values, values, _length * sizeof(double));
        }

        ~ConstrainCompatibility() override { delete[] _values; }

        void ini() override {}

        bool meet(int, double value) override
        {
            for (int i = 0; i < _length; i++)
                if (equal(value, _values[i])) return true;
            return false;
        }

        void update(int, double) override {}

        double violation(double* variables, int size) override
        {
            double back = 0;
            for (int j = 0; j < size; j++)
                if (!meet(j, variables[j])) back++;
            return back;
        }

        void regionReduction(int, domain_view& region) override
        {
            // 收窄到允许值集合：每个允许值是一个单点区间，与当前域求交。
            interval_set allowed;
            for (int i = 0; i < _length; i++)
                allowed.add_interval(interval(_values[i], _values[i]));
            region.set = region.set.intersect(allowed);
        }

        ConstrianLevel getConstrainLevel() override { return constraints_range; }

        Constrain* clone() override
        {
            return new ConstrainCompatibility(_w, _values, _length);
        }
    };

    // ---------------------------------------------------------------
    // Eligible constraint: 逐维允许值集合(Compatibility 的逐维推广)。
    //   _allowed[dim] = 该维允许取的值集合;regionReduction(dim) 把域与该集合求交。
    //   FJSP:每工序的允许机器。维度超出 _allowed 范围 → 不限制(放行)。
    // ---------------------------------------------------------------
    class ConstrainEligible final : public Constrain
    {
    private:
        std::vector<std::vector<double>> _allowed;   // [dim] -> 允许值
    public:
        explicit ConstrainEligible(double penalty_w, std::vector<std::vector<double>> allowed)
            : Constrain(penalty_w), _allowed(std::move(allowed)) {}

        void ini() override {}

        bool meet(int dim, double value) override
        {
            if (dim < 0 || dim >= static_cast<int>(_allowed.size())) return true;
            for (double v : _allowed[dim]) if (equal(v, value)) return true;
            return false;
        }

        void update(int, double) override {}

        double violation(double* variables, int size) override
        {
            double back = 0;
            for (int j = 0; j < size; j++)
                if (!meet(j, variables[j])) back++;
            return back;
        }

        void regionReduction(int dim, domain_view& region) override
        {
            if (dim < 0 || dim >= static_cast<int>(_allowed.size())) return;
            interval_set allowed;
            for (double v : _allowed[dim]) allowed.add_interval(interval(v, v));
            region.set = region.set.intersect(allowed);
        }

        // 逐维不同 → 必须走 dem 路径(每维单独缩减),不能用 constraints_range(build 期按维0静态烘焙)。
        ConstrianLevel getConstrainLevel() override { return constraints_discrete; }
        Constrain* clone() override { return new ConstrainEligible(_w, _allowed); }
    };
}
