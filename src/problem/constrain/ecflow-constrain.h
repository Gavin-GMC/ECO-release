//------------------------Description------------------------
// ECO 框架的约束类(约束族基类 Constrain + 通用约束:MinDistance / UserDefined / Expression 等)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <map>
#include <unordered_set>
#include <algorithm>
#include <cstring>
#include <stdexcept>

#include "ecflow-math.h"     // equal()
#include "ecflow-sort.h"     // sortHelper
#include "variable.h"
#include "ecflow-calculator.h"
#include "ExprTree.h"

// Unified domain: continuous interval_set + discrete view (replaces FeasibleLine).
#include "domain_view.h"
// cp engine: relational constraint propagation (ConstrainExpression backend).
#include "cp/cp.h"
#include <memory>

namespace ECFlow
{
    // ---------------------------------------------------------------
    // Constraint level enum
    // ---------------------------------------------------------------
    enum ConstrianLevel
    {
        constrains_non,
        constraints_range,
        constrains_variable,
        constraints_discrete,
        constraints_continue,
        constraints_customization
    };

    // ---------------------------------------------------------------
    // Abstract base class
    // ---------------------------------------------------------------
    class Constrain
    {
    protected:
        double _w; // penalty weight
    public:
        explicit Constrain(double penalty_w) : _w(penalty_w) {}
        virtual ~Constrain() {}

        virtual void ini() = 0;
        virtual bool meet(int demensionId, double value) = 0;
        virtual void update(int demensionId, double value) = 0;
        virtual double violation(double* variables, int size) = 0;
        virtual void regionReduction(int demensionId, domain_view& region) = 0;
        virtual void addCorresConstrain(Constrain*) {}
        virtual ConstrianLevel getConstrainLevel() = 0;
        virtual Constrain* clone() = 0;

        double getWeight() const { return _w; }
    };

    // ConstrainRange / ConstrainCompatibility —— 已迁移至 ecflow-domain.h(取值域限制族)。
    // ConstrainUnique / ConstrainUniqueLarge / ConstrainDistributed —— 见 ecflow-cardinality.h(基数/计数族)。

    // ---------------------------------------------------------------
    // Min-distance constraint: consecutive values must be >= gap apart
    // ---------------------------------------------------------------
    class ConstrainMinDistance final : public Constrain
    {
    private:
        double* _gap_width;
        double* _position_min;
        double* _position_max;
        int     _size;
        double  _start;

    public:
        ConstrainMinDistance(double penalty_w, double start, double end,
                             double gap_width, int size)
            : Constrain(penalty_w), _size(size), _start(start)
        {
            _gap_width    = new double[size];
            _position_min = new double[size];
            _position_max = new double[size];
            for (int i = 0; i < size; i++) _gap_width[i] = gap_width;
            _position_max[size - 1] = end;
            for (int i = size - 2; i >= 0; i--)
                _position_max[i] = _position_max[i + 1] - _gap_width[i + 1];
        }

        ConstrainMinDistance(double penalty_w, double start, double end,
                             double* gap_width, int size)
            : Constrain(penalty_w), _size(size), _start(start)
        {
            _gap_width    = new double[size];
            _position_min = new double[size];
            _position_max = new double[size];
            for (int i = 0; i < size; i++) _gap_width[i] = gap_width[i];
            _position_max[size - 1] = end;
            for (int i = size - 2; i >= 0; i--)
                _position_max[i] = _position_max[i + 1] - _gap_width[i + 1];
        }

        ~ConstrainMinDistance() override
        {
            delete[] _gap_width;
            delete[] _position_min;
            delete[] _position_max;
        }

        void ini() override
        {
            _position_min[0] = _start;
            for (int i = 1; i < _size; i++)
                _position_min[i] = _position_min[i - 1] + _gap_width[i];
        }

        bool meet(int demensionId, double value) override
        {
            return value >= _position_min[demensionId] &&
                   value <= _position_max[demensionId];
        }

        void update(int demensionId, double value) override
        {
            _position_min[demensionId] = value;
            for (int i = demensionId + 1; i < _size; i++)
                _position_min[i] = _position_min[i - 1] + _gap_width[i];
        }

        double violation(double* variables, int size) override
        {
            double back = 0, pre = variables[0];
            for (int did = 1; did < size; did++)
            {
                double def = _gap_width[did - 1] - (variables[did] - pre);
                if (def > 0) back += def;
                pre = variables[did];
            }
            return back;
        }

        void regionReduction(int demensionId, domain_view& region) override
        {
            region.restrict(_position_min[demensionId], _position_max[demensionId]);
        }

        ConstrianLevel getConstrainLevel() override { return constraints_discrete; }

        Constrain* clone() override
        {
            return new ConstrainMinDistance(_w, _start,
                                            _position_max[_size - 1],
                                            _gap_width, _size);
        }
    };

    // NOTE: ConstrainCapacity moved to ecflow-accumulate.h (it is a specialised
    // accumulate/cumulative constraint — per-container capacity tracking).

    // ConstrainDistributed —— 已迁移至 ecflow-cardinality.h(基数/计数约束族)。

    // ---------------------------------------------------------------
    // User-defined constraint: supplied via function pointers
    // ---------------------------------------------------------------
    class ConstrainUserDefined final : public Constrain
    {
    private:
        void   (*_ini_func)(void);
        double (*_check_func)(int, double);
        void   (*_change_func)(int, double);
    public:
        ConstrainUserDefined(double penalty_w,
                             void   (*ini_func)(void),
                             double (*check_func)(int, double),
                             void   (*change_func)(int, double))
            : Constrain(penalty_w),
              _ini_func(ini_func), _check_func(check_func),
              _change_func(change_func) {}

        void ini() override { _ini_func(); }

        bool meet(int demensionId, double value) override
        {
            return equal(_check_func(demensionId, value), 0.0);
        }

        void update(int demensionId, double value) override
        {
            _change_func(demensionId, value);
        }

        double violation(double* variables, int size) override
        {
            double back = 0;
            _ini_func();
            for (int did = 0; did < size; did++)
            {
                back += _check_func(did, variables[did]);
                _change_func(did, variables[did]);
            }
            return back;
        }

        void regionReduction(int, domain_view&) override {}

        ConstrianLevel getConstrainLevel() override { return constraints_customization; }

        Constrain* clone() override
        {
            return new ConstrainUserDefined(_w, _ini_func, _check_func, _change_func);
        }
    };

    // ---------------------------------------------------------------
    // Expression-based constraint (NEW)
    // Uses TreeCalculator / ExpressionTree (muparserx).
    // The formula should return 0 when the constraint is satisfied,
    // and a positive value proportional to the degree of violation.
    // Input layout: input[0] = pointer to current variable array
    // ---------------------------------------------------------------
    // ---------------------------------------------------------------
    // Expression constraint, backed by the cp relational-propagation engine.
    //
    // This IS the original framework's planned "arithmetic constraint"
    // (ConstrainArithmetic in the legacy constrain.h — declaration-only, meant
    // to do arithmetic-expression evaluation + feasible-region reduction). It is
    // unified here under the Expression name; there is no separate Arithmetic
    // class or API.
    //
    // The planned "logical constraint" (ConstrainLogical) is also subsumed:
    // relational sub-expressions are valid indicator factors, e.g. "(x[0]>5)"
    // evaluates to {0,1}, so logic is expressed as indicator arithmetic
    //   NOT a:"(a)"  a AND b:"2-(a)-(b)"  a OR b:"1-(a)-(b)"  a->b:"(a)-(b)"
    // Violation is exact at point values; reverse region reduction through
    // relational nodes is sound but conservative at the boundary (use a plain
    // arithmetic relation when tight pruning is needed). See REFACTOR_PLAN.md.
    //
    // The formula F is an ARITHMETIC violation-degree expression over the
    // elements of ONE variable, written as x[0], x[1], ... (e.g. "x[0]-5",
    // "x[0]+x[1]-10"). Semantics: the constraint is "F <= 0"; violation = max(0, F).
    //
    // Unlike the old TreeCalculator backend (which only computed a penalty at
    // the customization level), this version participates in feasible-region
    // reduction: it is a `constraints_discrete` constraint, so during
    // constructive generation it reverse-propagates "F <= 0" (cp mode A) to
    // narrow the queried dimension's domain given the already-fixed dimensions.
    //
    // cp layout: the single variable base occupies slots [0, L) in index order,
    // so dimension `did` maps directly to cp slot `did`. L = max formula index+1
    // (so a formula that only uses x[0] gives L=1; "0" gives L=0).
    // ---------------------------------------------------------------
    class ConstrainExpression final : public Constrain
    {
    private:
        std::string              _formula;
        int                      _var_length;
        std::shared_ptr<Program> _prog;       // immutable, shareable; cp constraint "F <= 0"
        std::unique_ptr<State>   _state;       // per-instance domains (references *_prog)
        int                      _xlen = 0;    // cp slot count = formula variable length
        std::vector<double>      _fixed;       // fixed value per slot (set via update())
        std::vector<char>        _is_fixed;    // which slots are fixed in current construction

        // Compile "(formula) <= 0" into a cp Program + fresh State.
        void buildProgram()
        {
            ConstraintSystemBuilder b;
            b.add("(" + _formula + ") <= 0");
            try {
                _prog = std::make_shared<Program>(b.compile());
            } catch (const parse_error& e) {
                throw std::invalid_argument(
                    std::string("ConstrainExpression: invalid formula '") +
                    _formula + "': " + e.what());
            }
            _xlen  = static_cast<int>(_prog->variable_count());
            _state.reset(new State(*_prog));
            _fixed.assign(_xlen, 0.0);
            _is_fixed.assign(_xlen, 0);
        }

    public:
        /// @param penalty_w    penalty weight
        /// @param formula      arithmetic violation expression, e.g. "x[0]-10"
        /// @param notes        (unused; cp parses the formula itself)
        /// @param param_count  (unused)
        /// @param var_length   length of the variable array
        ConstrainExpression(double penalty_w,
                            const std::string& formula,
                            ElementNote* /*notes*/,
                            int /*param_count*/,
                            int var_length)
            : Constrain(penalty_w), _formula(formula), _var_length(var_length)
        {
            buildProgram();
        }

        ~ConstrainExpression() override = default;

        void ini() override
        {
            std::fill(_is_fixed.begin(), _is_fixed.end(), static_cast<char>(0));
        }

        bool meet(int, double) override
        {
            // Feasibility is enforced by region reduction (regionReduction) and
            // measured by violation(); meet() never rejects.
            return true;
        }

        // Record a fixed dimension value (used as a point in reverse propagation).
        void update(int did, double value) override
        {
            if (did >= 0 && did < _xlen) { _fixed[did] = value; _is_fixed[did] = 1; }
        }

        double violation(double* variables, int size) override
        {
            // All variables become points; evaluate F (= lhs of "F <= 0") and clamp.
            const NodeId root = _prog->constraints()[0].root;
            for (int s = 0; s < _xlen; ++s)
                _state->assign(s, s < size ? variables[s] : 0.0);
            double f = node_value(*_state, _prog->node(root).a);
            return f > 0.0 ? f : 0.0;
        }

        // Reverse-propagate "F <= 0" to narrow dimension `did`'s feasible region,
        // holding already-fixed dimensions at their values (cp mode A).
        void regionReduction(int did, domain_view& region) override
        {
            if (_xlen == 0 || did < 0 || did >= _xlen) return;
            for (int s = 0; s < _xlen; ++s)
            {
                if (s == did)          _state->set_domain(s, region.set);
                else if (_is_fixed[s]) _state->assign(s, _fixed[s]);
                else                   _state->set_domain(s, interval_set::whole());
            }
            solve(*_state, /*force_full=*/true);
            region.set = region.set.intersect(_state->domain(did));
        }

        ConstrianLevel getConstrainLevel() override { return constraints_discrete; }

        Constrain* clone() override { return new ConstrainExpression(*this); }

    private:
        // Copy constructor for clone(): rebuild the cp program from the formula
        // (Program is immutable; a fresh State avoids cross-clone aliasing).
        ConstrainExpression(const ConstrainExpression& src)
            : Constrain(src._w), _formula(src._formula), _var_length(src._var_length)
        {
            buildProgram();
        }
    };

} // namespace ECFlow
