//------------------------Description------------------------
// 基数/计数类约束:对"取值占用"计数并据此缩减域(CP 中的 cardinality 约束族)。
//-------------------------Reference-------------------------
// CP 对应:alldifferent(unique)、global cardinality / GCC(distributed)、
//          atmost-nvalue(distinct_cap)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------
#pragma once
#include <unordered_set>
#include <cstring>
#include <cmath>
#include <algorithm>
#include "ecflow-constrain.h"   // Constrain, ConstrianLevel, domain_view, EMPTYVALUE, equal(), sortHelper

namespace ECFlow
{
    // ===============================================================
    // 重数轴:每个值的出现次数上限
    // ===============================================================

    // ---------------------------------------------------------------
    // Unique constraint (small domain): each value appears at most once
    // ---------------------------------------------------------------
    class ConstrainUnique final : public Constrain
    {
    private:
        bool*   _not_had;
        double* _values;
        int     _size;
        double  _prevalue;

        int findIndex(double value) const
        {
            int left = 0, right = _size - 1;
            while (left <= right)
            {
                int mid = (left + right) / 2;
                if (equal(_values[mid], value)) return mid;
                if (_values[mid] < value)       left  = mid + 1;
                else                             right = mid - 1;
            }
            return -1;
        }

    public:
        ConstrainUnique(double penalty_w, double* element_list, int size)
            : Constrain(penalty_w), _size(size), _prevalue(EMPTYVALUE)
        {
            _not_had = new bool[_size];
            _values  = new double[_size];
            memcpy(_values, element_list, _size * sizeof(double));
            std::sort(_values, _values + _size);
        }

        ~ConstrainUnique() override { delete[] _not_had; delete[] _values; }

        void ini() override
        {
            for (int i = 0; i < _size; i++) _not_had[i] = true;
        }

        bool meet(int, double value) override
        {
            int id = findIndex(value);
            return id >= 0 && _not_had[id];
        }

        void update(int, double value) override
        {
            int id = findIndex(value);
            if (id < 0) return;
            _prevalue = value;
            _not_had[id] = false;
        }

        double violation(double* variables, int size) override
        {
            int back = 0;
            ini();
            for (int did = 0; did < size; did++)
            {
                int id = findIndex(variables[did]);
                if (id < 0 || !_not_had[id]) back++;
                else _not_had[id] = false;
            }
            return back;
        }

        void regionReduction(int, domain_view& region) override
        {
            region.remove_point(_prevalue);
        }

        ConstrianLevel getConstrainLevel() override { return constrains_variable; }

        Constrain* clone() override
        {
            return new ConstrainUnique(_w, _values, _size);
        }
    };

    // ---------------------------------------------------------------
    // Unique constraint (large domain): hash-set based
    // ---------------------------------------------------------------
    class ConstrainUniqueLarge final : public Constrain
    {
    private:
        std::unordered_set<double> _exist;
        double _prevalue;
    public:
        explicit ConstrainUniqueLarge(double penalty_w)
            : Constrain(penalty_w), _prevalue(EMPTYVALUE) {}

        void ini() override { _exist.clear(); }

        bool meet(int, double value) override
        {
            for (const auto& v : _exist)
                if (equal(v, value)) return false;
            return true;
        }

        void update(int, double value) override
        {
            _exist.insert(value);
            _prevalue = value;
        }

        double violation(double* variables, int size) override
        {
            int back = 0;
            ini();
            for (int did = 0; did < size; did++)
            {
                if (meet(did, variables[did])) update(did, variables[did]);
                else back++;
            }
            return back;
        }

        void regionReduction(int, domain_view& region) override
        {
            region.remove_point(_prevalue);
        }

        ConstrianLevel getConstrainLevel() override { return constrains_variable; }
        Constrain* clone() override { return new ConstrainUniqueLarge(_w); }
    };

    // ---------------------------------------------------------------
    // Distributed constraint: each value may appear at most N times (GCC).
    //   unique 的一般化(每值上限 k_v,而非恒 1)。用满即从域中删去该值。
    // ---------------------------------------------------------------
    class ConstrainDistributed final : public Constrain
    {
    private:
        int*    _appear_max;
        int*    _appear_number;
        double* _values;
        int     _size;
        double  _prevalue;

        int findIndex(double value) const
        {
            int left = 0, right = _size - 1;
            while (left <= right)
            {
                int mid = (left + right) / 2;
                if (equal(_values[mid], value)) return mid;
                if (_values[mid] < value)        right = mid - 1;
                else                              left  = mid + 1;
            }
            return -1;
        }

    public:
        ConstrainDistributed(double penalty_w,
                             double* feasible_values, int size,
                             double* appear_numbers)
            : Constrain(penalty_w), _size(size), _prevalue(EMPTYVALUE)
        {
            _appear_max    = new int   [size];
            _appear_number = new int   [size];
            _values        = new double[size];

            sortHelper<int, double>* buf = new sortHelper<int, double>[size];
            for (int i = 0; i < size; i++)
            {
                buf[i].id    = static_cast<int>(appear_numbers[i]);
                buf[i].value = feasible_values[i];
            }
            std::sort(buf, buf + size);
            for (int i = 0; i < size; i++)
            {
                _values[i]      = buf[i].value;
                _appear_max[i]  = buf[i].id;
            }
            delete[] buf;
        }

        ~ConstrainDistributed() override
        {
            delete[] _appear_max;
            delete[] _appear_number;
            delete[] _values;
        }

        void ini() override
        {
            for (int i = 0; i < _size; i++) _appear_number[i] = 0;
            _prevalue = EMPTYVALUE;
        }

        bool meet(int, double value) override
        {
            int id = findIndex(value);
            return id >= 0 && _appear_number[id] < _appear_max[id];
        }

        void update(int, double value) override
        {
            int id = findIndex(value);
            if (id < 0) return;
            _appear_number[id]++;
            if (_appear_number[id] == _appear_max[id]) _prevalue = value;
        }

        double violation(double* solution, int size) override
        {
            double back = 0;
            ini();
            for (int i = 0; i < size; i++) update(i, solution[i]);
            for (int i = 0; i < _size; i++)
                if (_appear_number[i] > _appear_max[i])
                    back += _appear_number[i] - _appear_max[i];
            return back;
        }

        void regionReduction(int, domain_view& region) override
        {
            if (!is_empty(_prevalue))   // 仅在上一值已设置时才剔除(修复:原 != EMPTYVALUE 对 NaN 恒真 → 守卫永远放行,未设置时也 remove_point(NaN))
            {
                region.remove_point(_prevalue);
                _prevalue = EMPTYVALUE;
            }
        }

        ConstrianLevel getConstrainLevel() override { return constrains_variable; }

        Constrain* clone() override
        {
            double* ap = new double[_size];
            for (int i = 0; i < _size; i++) ap[i] = _appear_max[i];
            auto* back = new ConstrainDistributed(_w, _values, _size, ap);
            delete[] ap;
            return back;
        }
    };

    // ===============================================================
    // 基数轴:不同值的个数上限
    // ===============================================================

    // ---------------------------------------------------------------
    // Distinct-count cap: 至多 p 个不同值(CPMP 选 p 个中位的"选址层")。
    //   记录已现值集合 S。|S|<p 时任意值合法(可开新值);|S|==p 后只允许 S 中的值,
    //   regionReduction 把 S 外的点全部删去(收缩到已开的 p 个值)。
    //   violation = |不同值数 − p|(软兜底,亦覆盖 <p——构造期不阻止用少于 p 个)。
    //   值按整数量化(allocation 域 lowbound=0/accuracy=1)。
    // ---------------------------------------------------------------
    class ConstrainDistinctCap final : public Constrain
    {
    private:
        int _p;
        std::unordered_set<long long> _seen;
        static long long key(double v) { return static_cast<long long>(std::llround(v)); }
    public:
        ConstrainDistinctCap(double penalty_w, int p) : Constrain(penalty_w), _p(p) {}

        void ini() override { _seen.clear(); }

        bool meet(int, double value) override
        {
            if (static_cast<int>(_seen.size()) < _p) return true;
            return _seen.count(key(value)) > 0;
        }

        void update(int, double value) override { _seen.insert(key(value)); }

        void regionReduction(int, domain_view& region) override
        {
            if (static_cast<int>(_seen.size()) < _p) return;          // 未满 p → 不缩减
            for (double v : region.enumerate())
                if (_seen.count(key(v)) == 0) region.remove_point(v); // 满 p → 收缩到已开集合
        }

        double violation(double* variables, int size) override
        {
            std::unordered_set<long long> s;
            for (int i = 0; i < size; i++) s.insert(key(variables[i]));
            int diff = static_cast<int>(s.size()) - _p;
            return diff < 0 ? -diff : diff;
        }

        ConstrianLevel getConstrainLevel() override { return constraints_discrete; }
        Constrain* clone() override { return new ConstrainDistinctCap(_w, _p); }
    };
}
