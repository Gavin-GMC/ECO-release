//------------------------Description------------------------
// ECO 框架的启发式(Inspiration)类:Random / Boundary / Stable / Normal / Expression。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <algorithm>
#include <cstring>
#include <stdexcept>

#include "ecflow-rand.h"         // ECFlow::rand01(), ECFlow::shuffle()
#include "ecflow-sort.h"          // sortHelper
#include "ecflow-calculator.h"
#include "ecflow-functor.hpp"
#include "ExprTree.h"

// Unified domain: discrete view over interval_set (replaces FeasibleLine).
#include "domain_view.h"

namespace ECFlow
{
    // ---------------------------------------------------------------
    // Abstract base class
    // ---------------------------------------------------------------
    class Inspiration
    {
    public:
        Inspiration() {}
        virtual ~Inspiration() {}

        /// Get the recommended (highest priority) decision value for a dimension.
        virtual double getPrioriDecision(domain_view& feasible_region,
                                         int demension, double** inputs) = 0;

        /// Get an ordered list of feasible decisions, best first (caller owns returned array).
        virtual double* getPrioriOrder(domain_view& feasible_region,
                                       int demension, double** inputs,
                                       int& order_size) = 0;

        /// Get the heuristic score for a particular choice in a dimension.
        virtual double getHeuristic(domain_view& feasible_region,
                                    int demension, double choice,
                                    double** inputs) = 0;

        virtual Inspiration* clone() = 0;
    };

    // ---------------------------------------------------------------
    // Random heuristic: default random selection from feasible region
    // ---------------------------------------------------------------
    class RandomInspiration final : public Inspiration
    {
    public:
        RandomInspiration() : Inspiration() {}
        ~RandomInspiration() override {}

        double getPrioriDecision(domain_view& feasible_region,
                                 int, double**) override
        {
            return feasible_region.random();
        }

        double* getPrioriOrder(domain_view& feasible_region,
                               int, double**, int& order_size) override
        {
            // ⚠️ 连续/细精度变量风险：可行域未缩减时 enumerate_alloc 物化全部网格点，
            //    点数随精度暴涨（如 [0,30]@1e-5≈3e6），可能严重耗时甚至内存溢出；
            //    启发式需对全部候选排序，故此处刻意全枚举、不降采样，调用方需控制变量精度/规模。
            double* back = feasible_region.enumerate_alloc(order_size);
            ECFlow::shuffle(back, static_cast<size_t>(order_size)); // fixed: was std::shuffle with random_device
            return back;
        }

        double getHeuristic(domain_view&,int, double, double**) override
        {
            return ECFlow::rand01(); // fixed: was ::rand01() / rand01() without namespace
        }

        Inspiration* clone() override { return new RandomInspiration(); }
    };

    // ---------------------------------------------------------------
    // Boundary heuristic: prefer boundary values of feasible region
    // ---------------------------------------------------------------
    class BoundaryInspiration final : public Inspiration
    {
    public:
        BoundaryInspiration() : Inspiration() {}
        ~BoundaryInspiration() override {}

        double getPrioriDecision(domain_view& feasible_region,
                                 int, double**) override
        {
            return feasible_region.boundary(true);
        }

        double* getPrioriOrder(domain_view& feasible_region,
                               int, double**, int& order_size) override
        {
            // ⚠️ 连续/细精度变量风险：可行域未缩减时 enumerate_alloc 物化全部网格点，
            //    点数随精度暴涨，可能严重耗时甚至内存溢出（启发式需全量排序，故不降采样）。
            return feasible_region.enumerate_alloc(order_size);
        }

        double getHeuristic(domain_view& feasible_region,
                            int, double choice, double**) override
        {
            if (choice == feasible_region.boundary(true) ||
                choice == feasible_region.boundary(false))
                return 1.0;
            return 0.0;
        }

        Inspiration* clone() override { return new BoundaryInspiration(); }
    };

    // ---------------------------------------------------------------
    // StableInspiration: pre-computed ordered list (low runtime cost)
    // ---------------------------------------------------------------
    class StableInspiration : public Inspiration
    {
    private:
        double* _saved_order;
        int     _order_size[2]; // [0] = dimension count, [1] = choices per dimension

        double& orderTable(int demension, int rank)
        {
            return _saved_order[demension * _order_size[1] + rank];
        }

        void _ini(domain_view& region_pointer, Calculator* calculator, double** inputs)
        {
            double decision_pair[2]; // [0]=dimension, [1]=choice
            int paras = calculator->getParameterNumber();
            double** calc_buf = new double*[paras];
            int feasible_list_size;
            sortHelper<double, double>* sort_buf =
                new sortHelper<double, double>[_order_size[1]];

            // ⚠️ 连续/细精度变量风险：可行域未缩减时 enumerate_alloc 物化全部网格点，
            //    点数随精度暴涨，可能严重耗时甚至内存溢出（stable 预算需全量，故不降采样）。
            double* feasible_list = region_pointer.enumerate_alloc(feasible_list_size);
            calc_buf[0] = decision_pair;
            calc_buf[1] = decision_pair + 1;
            std::memcpy(calc_buf + 2, inputs, (paras - 2) * sizeof(double*));

            for (int d = 0; d < _order_size[0]; d++)
            {
                decision_pair[0] = static_cast<double>(d);
                for (int i = 0; i < feasible_list_size; i++)
                {
                    sort_buf[i].id = feasible_list[i];
                    decision_pair[1] = feasible_list[i];
                    calculator->run(calc_buf, &sort_buf[i].value);
                }
                std::sort(sort_buf, sort_buf + feasible_list_size);
                for (int i = 0; i < feasible_list_size; i++)
                    orderTable(d, i) = sort_buf[i].id;
            }

            delete[] feasible_list;
            delete[] sort_buf;
            delete[] calc_buf;
        }

        StableInspiration() {}

    public:
        StableInspiration(domain_view& region_pointer, Calculator* calculator,
                          int demension_number, double** inputs)
            : Inspiration()
        {
            _order_size[0] = demension_number;
            _order_size[1] = region_pointer.count();
            _saved_order = new double[_order_size[0] * _order_size[1]];
            _ini(region_pointer, calculator, inputs);
        }

        StableInspiration(double* priori_matrix, int size, int demension_number)
            : Inspiration()
        {
            _order_size[0] = demension_number;
            _order_size[1] = size;
            _saved_order = new double[_order_size[0] * _order_size[1]];
            std::memcpy(_saved_order, priori_matrix,
                        size * demension_number * sizeof(double));
        }

        ~StableInspiration() override { delete[] _saved_order; }

        double* getPrioriOrder(domain_view& feasible_region,
                               int demension, double**, int& order_size) override
        {
            order_size = feasible_region.count();
            double* back = new double[order_size];
            int counter = 0;
            for (int i = 0; i < _order_size[1] && counter < order_size; i++)
            {
                if (feasible_region.contains_value(orderTable(demension, i)))
                    back[counter++] = orderTable(demension, i);
            }
            order_size = counter;
            return back;
        }

        double getPrioriDecision(domain_view& feasible_region,
                                 int demension, double**) override
        {
            for (int i = 0; i < _order_size[1]; i++)
                if (feasible_region.contains_value(orderTable(demension, i)))
                    return orderTable(demension, i);
            return EMPTYVALUE;
        }

        double getHeuristic(domain_view&,int demension, double choice, double**) override
        {
            int base = demension * _order_size[1];
            for (int i = 0; i < _order_size[1]; i++)
                if (_saved_order[base + i] == choice) return static_cast<double>(i);
            return static_cast<double>(_order_size[1] + 1);
        }

        Inspiration* clone() override
        {
            StableInspiration* back = new StableInspiration();
            back->_order_size[0] = _order_size[0];
            back->_order_size[1] = _order_size[1];
            back->_saved_order = new double[_order_size[0] * _order_size[1]];
            std::memcpy(back->_saved_order, _saved_order,
                        _order_size[0] * _order_size[1] * sizeof(double));
            return back;
        }

        friend class NormalInspiration;
    };

    // ---------------------------------------------------------------
    // NormalInspiration: dynamic heuristic via Calculator
    // ---------------------------------------------------------------
    class NormalInspiration : public Inspiration
    {
    private:
        Calculator* _calculator;
        double      _decision_pair[2]; // [0]=dimension, [1]=choice
        int         _parameter_number;
        double**    _calc_buf;

    public:
        explicit NormalInspiration(Calculator* calculator) : Inspiration()
        {
            _calculator = calculator->copy();
            _parameter_number = calculator->getParameterNumber();
            if (_parameter_number < 2) return;
            _calc_buf    = new double*[_parameter_number];
            _calc_buf[0] = _decision_pair;
            _calc_buf[1] = _decision_pair + 1;
        }

        ~NormalInspiration() override
        {
            delete _calculator;
            if (_parameter_number >= 2)
                delete[] _calc_buf;
        }

        double* getPrioriOrder(domain_view& feasible_region,
                               int demension, double** inputs,
                               int& order_size) override
        {
            // ⚠️ 连续/细精度变量风险：可行域未缩减时 enumerate_alloc 物化全部网格点，
            //    点数随精度暴涨（如 [0,30]@1e-5≈3e6），可能严重耗时甚至内存溢出；
            //    启发式需对全部候选排序，故此处刻意全枚举、不降采样，调用方需控制变量精度/规模。
            double* back = feasible_region.enumerate_alloc(order_size);
            sortHelper<double, double>* sort_buf =
                new sortHelper<double, double>[order_size];
            std::memcpy(_calc_buf + 2, inputs,
                        sizeof(double*) * (_parameter_number - 2));

            _decision_pair[0] = static_cast<double>(demension);
            for (int i = 0; i < order_size; i++)
            {
                sort_buf[i].id = back[i];
                _decision_pair[1] = back[i];
                _calculator->run(_calc_buf, &sort_buf[i].value);
            }
            std::sort(sort_buf, sort_buf + order_size,
                      std::greater<sortHelper<double, double>>());
            for (int i = 0; i < order_size; i++)
                back[i] = sort_buf[i].id;

            delete[] sort_buf;
            return back;
        }

        double getPrioriDecision(domain_view& feasible_region,
                                 int demension, double** inputs) override
        {
            int order_size;
            double* order = getPrioriOrder(feasible_region, demension, inputs, order_size);
            double back = order[0];
            delete[] order;
            return back;
        }

        double getHeuristic(domain_view&,int demension, double choice,
                            double** inputs) override
        {
            double back;
            _decision_pair[0] = static_cast<double>(demension);
            _decision_pair[1] = choice;
            std::memcpy(_calc_buf + 2, inputs,
                        sizeof(double*) * (_parameter_number - 2));
            _calculator->run(_calc_buf, &back);
            return back;
        }

        Inspiration* clone() override { return new NormalInspiration(_calculator); }

        StableInspiration* toStable(domain_view& region_pointer,
                                    int demension_number, double** inputs)
        {
            return new StableInspiration(region_pointer, _calculator,
                                         demension_number, inputs);
        }
    };

    // ---------------------------------------------------------------
    // InspirationExpression (NEW): expression-string based heuristic
    // ---------------------------------------------------------------
    /// Uses a formula string to compute heuristic scores.
    /// The formula receives (dimension, choice, v0[0], v1[0], ...) as x[0], x[1], ...
    /// A higher returned value means a more preferred choice.
    class InspirationExpression final : public Inspiration
    {
    private:
        std::string     _formula;
        int             _n_extra;     // number of extra input variables beyond (dim, choice)
        TreeCalculator* _calculator;  // owns

        // The formula sees the whole parameter buffer as a single array "x":
        //   x[0]=dimension, x[1]=choice, x[2+k]=inputs[k][0].
        TreeCalculator* buildCalc() const
        {
            ElementNote note;
            note._name     = "x";
            note._length   = _n_extra + 2;
            note._shape[0] = _n_extra + 2;   // ExprTree sizes the array from _shape
            note._shape[1] = 1;
            return new TreeCalculator(_formula, &note, 1, 1);
        }

    public:
        /// @param formula  expression string using x[0]=dim, x[1]=choice, x[2..]=inputs.
        /// @param n_extra  number of extra input variable arrays.
        explicit InspirationExpression(const std::string& formula, int n_extra)
            : Inspiration(), _formula(formula), _n_extra(n_extra), _calculator(nullptr)
        {
            _calculator = buildCalc();
        }

        ~InspirationExpression() override { delete _calculator; }

        double* getPrioriOrder(domain_view& feasible_region,
                               int demension, double** inputs,
                               int& order_size) override
        {
            // ⚠️ 连续/细精度变量风险：可行域未缩减时 enumerate_alloc 物化全部网格点，
            //    点数随精度暴涨（如 [0,30]@1e-5≈3e6），可能严重耗时甚至内存溢出；
            //    启发式需对全部候选排序，故此处刻意全枚举、不降采样，调用方需控制变量精度/规模。
            double* back = feasible_region.enumerate_alloc(order_size);
            sortHelper<double, double>* sort_buf =
                new sortHelper<double, double>[order_size];

            std::vector<double> xbuf(_n_extra + 2);
            xbuf[0] = static_cast<double>(demension);
            for (int k = 0; k < _n_extra; k++) xbuf[2 + k] = inputs[k][0];
            double* xp = xbuf.data();

            for (int i = 0; i < order_size; i++)
            {
                sort_buf[i].id = back[i];
                xbuf[1] = back[i];
                double result;
                _calculator->run(&xp, &result);
                sort_buf[i].value = result;
            }
            std::sort(sort_buf, sort_buf + order_size,
                      std::greater<sortHelper<double, double>>());
            for (int i = 0; i < order_size; i++)
                back[i] = sort_buf[i].id;

            delete[] sort_buf;
            return back;
        }

        double getPrioriDecision(domain_view& feasible_region,
                                 int demension, double** inputs) override
        {
            int order_size;
            double* order = getPrioriOrder(feasible_region, demension, inputs, order_size);
            double back = (order_size > 0) ? order[0] : EMPTYVALUE;
            delete[] order;
            return back;
        }

        double getHeuristic(domain_view&,int demension, double choice,
                            double** inputs) override
        {
            std::vector<double> xbuf(_n_extra + 2);
            xbuf[0] = static_cast<double>(demension);
            xbuf[1] = choice;
            for (int k = 0; k < _n_extra; k++) xbuf[2 + k] = inputs[k][0];
            double* xp = xbuf.data();

            double result;
            _calculator->run(&xp, &result);
            return result;
        }

        Inspiration* clone() override
        {
            return new InspirationExpression(_formula, _n_extra);
        }
    };

} // namespace ECFlow
