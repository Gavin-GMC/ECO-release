//------------------------Description------------------------
// ECO 框架的累积(cumulative)约束族:Capacity / SequenceAccumulate / ScheduleAccumulate / Accumulate。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <vector>
#include <memory>
#include <algorithm>
#include <limits>

#include "ecflow-constrain.h"   // Constrain base, ConstrianLevel, domain_view

namespace ECFlow
{
    // Out-of-bounds amount: distance of `level` outside [L, U] (0 when satisfied).
    inline double accumulate_excess(double level, double L, double U)
    {
        double e = 0.0;
        if (level > U) e += level - U;
        if (level < L) e += L - level;
        return e;
    }

    // ---------------------------------------------------------------
    // Cross-node transfer model (shared by schedule / time track).
    // comm(p,i,np,ni) = data[p][i] * rate[np][ni]; same node -> rate 0.
    // ---------------------------------------------------------------
    struct CommModel
    {
        std::vector<std::vector<double>> data;   // [p][i] data volume on edge p->i
        std::vector<std::vector<double>> rate;   // [a][b] unit transfer cost (a==b -> 0)

        double of(int p, int i, int np, int ni) const
        {
            return data[p][i] * rate[np][ni];
        }
    };

    // ---------------------------------------------------------------
    // Capacity constraint: bin-packing style (specialised accumulate).
    // Per container, remaining capacity decreases by each item's volume.
    // (Moved here from ecflow-constrain.h.)
    // ---------------------------------------------------------------
    class ConstrainCapacity final : public Constrain
    {
    private:
        double* _capacity_ini;
        double* _capacity_left;
        double* _capacity_require;
        int     _size;
        int     _item_number;
    public:
        ConstrainCapacity(double penalty_w,
                          double* capacitys, int container_number,
                          double* volumes,   int item_number)
            : Constrain(penalty_w), _size(container_number),
              _item_number(item_number)
        {
            _capacity_ini     = new double[container_number];
            _capacity_left    = new double[container_number];
            _capacity_require = new double[item_number];
            memcpy(_capacity_ini,     capacitys, container_number * sizeof(double));
            memcpy(_capacity_require, volumes,   item_number      * sizeof(double));
        }

        ~ConstrainCapacity() override
        {
            delete[] _capacity_ini;
            delete[] _capacity_left;
            delete[] _capacity_require;
        }

        void ini() override
        {
            memcpy(_capacity_left, _capacity_ini, _size * sizeof(double));
        }

        bool meet(int demensionId, double value) override
        {
            return _capacity_left[int(value)] >= _capacity_require[demensionId];
        }

        void update(int demensionId, double value) override
        {
            _capacity_left[int(value)] -= _capacity_require[demensionId];
        }

        double violation(double* variables, int size) override
        {
            ini();
            for (int did = 0; did < size; did++)
                update(did, variables[did]);
            double back = 0;
            for (int i = 0; i < _size; i++)
                if (_capacity_left[i] < 0) back -= _capacity_left[i];
            return back;
        }

        void regionReduction(int demensionId, domain_view& region) override
        {
            for (int i = 0; i < _size; i++)
                if (_capacity_left[i] < _capacity_require[demensionId])
                    region.remove_point(i);
        }

        ConstrianLevel getConstrainLevel() override { return constraints_discrete; }

        Constrain* clone() override
        {
            return new ConstrainCapacity(_w, _capacity_ini, _size,
                                         _capacity_require, _item_number);
        }
    };

    // ===============================================================
    // Resource-track kernel (full version)
    // ===============================================================

    // Abstract track: { state, update rule, bounds }, evolving along the sequence.
    // Protocol: reset() -> commit(k,v) in ascending k (returns step excess);
    //           probe(k,v) const = read-only trial excess given prior commits.
    class ResourceTrack
    {
    public:
        virtual ~ResourceTrack() {}
        virtual void reset() = 0;
        virtual double commit(int k, double v) = 0;        // advance state, return step excess
        virtual double probe(int k, double v) const = 0;   // read-only trial step excess
        virtual ResourceTrack* clone() const = 0;
    };

    // Capacity track (⊕ = +): scalar prefix sum, delta may be ±.
    class CapacityTrack final : public ResourceTrack
    {
    private:
        std::vector<double> _delta;     // [node] increment (may be ±)
        double _L, _U, _init;
        double _run;                    // fixed-prefix accumulated level
    public:
        CapacityTrack(std::vector<double> delta_by_value, double L, double U, double init)
            : _delta(std::move(delta_by_value)), _L(L), _U(U), _init(init), _run(init) {}

        void reset() override { _run = _init; }

        double commit(int, double v) override
        {
            _run += _delta[static_cast<int>(v)];
            return accumulate_excess(_run, _L, _U);
        }
        double probe(int, double v) const override
        {
            return accumulate_excess(_run + _delta[static_cast<int>(v)], _L, _U);
        }
        ResourceTrack* clone() const override { return new CapacityTrack(*this); }
    };

    // Time track (⊕ = max-plus, parallelism = 1): DAG critical path + serial nodes.
    class TimeTrack final : public ResourceTrack
    {
    private:
        int _n, _node_count;
        std::vector<std::vector<int>>    _pred;
        std::vector<std::vector<double>> _exec;       // [i][node]
        CommModel                        _comm;
        std::vector<double>              _deadline;
        std::vector<double> _finish;
        std::vector<int>    _node;
        std::vector<double> _avail;

        double finish_at(int i, int ni) const
        {
            double ready = 0.0;
            for (int p : _pred[i])
                ready = std::max(ready, _finish[p] + _comm.of(p, i, _node[p], ni));
            return std::max(ready, _avail[ni]) + _exec[i][ni];
        }
    public:
        TimeTrack(int node_count,
                  std::vector<std::vector<int>>    pred,
                  std::vector<std::vector<double>> exec,
                  CommModel                        comm,
                  std::vector<double>              deadline)
            : _n(static_cast<int>(exec.size())), _node_count(node_count),
              _pred(std::move(pred)), _exec(std::move(exec)), _comm(std::move(comm)),
              _deadline(std::move(deadline)),
              _finish(_n, 0.0), _node(_n, -1), _avail(node_count, 0.0) {}

        void reset() override
        {
            std::fill(_finish.begin(), _finish.end(), 0.0);
            std::fill(_node.begin(),   _node.end(),   -1);
            std::fill(_avail.begin(),  _avail.end(),  0.0);
        }

        double commit(int i, double v) override
        {
            const int ni = static_cast<int>(v);
            const double f = finish_at(i, ni);
            _finish[i] = f; _node[i] = ni; _avail[ni] = f;
            return accumulate_excess(f, -std::numeric_limits<double>::infinity(), _deadline[i]);
        }
        double probe(int i, double v) const override
        {
            return accumulate_excess(finish_at(i, static_cast<int>(v)),
                                     -std::numeric_limits<double>::infinity(), _deadline[i]);
        }
        ResourceTrack* clone() const override { return new TimeTrack(*this); }
    };

    // ===============================================================
    // Simplified single-scenario constraints (independent of the kernel)
    // ===============================================================

    // VRP load / inventory: capacity-type, ⊕ = +.
    // delta[node] = increment when visiting node (pickup +, delivery -);
    // level_k = level_{k-1} + delta[v_k], bound L <= level_k <= U.
    class ConstrainSequenceAccumulate final : public Constrain
    {
    private:
        std::vector<double> _delta;   // [node] per-value increment (may be ±)
        double _L, _U, _init;
        double _run;                  // accumulated level of the fixed prefix
    public:
        ConstrainSequenceAccumulate(double penalty_w, std::vector<double> delta_by_value,
                                    double lower, double upper, double init = 0.0)
            : Constrain(penalty_w), _delta(std::move(delta_by_value)),
              _L(lower), _U(upper), _init(init), _run(init) {}

        void ini() override { _run = _init; }

        bool meet(int, double v) override
        {
            return accumulate_excess(_run + _delta[static_cast<int>(v)], _L, _U) <= 0.0;
        }

        void update(int, double v) override
        {
            _run += _delta[static_cast<int>(v)];     // advance prefix (relies on in-order update)
        }

        void regionReduction(int, domain_view& region) override
        {
            for (double v : region.enumerate())
                if (accumulate_excess(_run + _delta[static_cast<int>(v)], _L, _U) > 0.0)
                    region.remove_point(v);
        }

        double violation(double* x, int n) override
        {
            double level = _init, tot = 0.0;
            for (int k = 0; k < n; ++k)
            {
                level += _delta[static_cast<int>(x[k])];
                tot   += accumulate_excess(level, _L, _U);
            }
            return tot;
        }

        ConstrianLevel getConstrainLevel() override { return constraints_discrete; }
        Constrain* clone() override { return new ConstrainSequenceAccumulate(*this); }
    };

    // DAG workflow w/ deadlines: time-type, ⊕ = max-plus, parallelism = 1.
    // v_i = node assigned to task i. finish_i = max(preds+comm, avail)+exec;
    // bound finish_i <= deadline_i. Tasks numbered in topological order.
    class ConstrainScheduleAccumulate final : public Constrain
    {
    private:
        int _n, _node_count;
        std::vector<std::vector<int>>    _pred;
        std::vector<std::vector<double>> _exec;      // [i][node]
        CommModel                        _comm;
        std::vector<double>              _deadline;

        std::vector<double> _finish;   // [i]
        std::vector<int>    _node;      // [i], -1 = unassigned
        std::vector<double> _avail;     // [node]

        double finish_at(int i, int ni) const
        {
            double ready = 0.0;
            for (int p : _pred[i])
                ready = std::max(ready, _finish[p] + _comm.of(p, i, _node[p], ni));
            return std::max(ready, _avail[ni]) + _exec[i][ni];
        }

    public:
        ConstrainScheduleAccumulate(double penalty_w, int node_count,
                                    std::vector<std::vector<int>>    pred,
                                    std::vector<std::vector<double>> exec,
                                    CommModel                        comm,
                                    std::vector<double>              deadline)
            : Constrain(penalty_w),
              _n(static_cast<int>(exec.size())), _node_count(node_count),
              _pred(std::move(pred)), _exec(std::move(exec)), _comm(std::move(comm)),
              _deadline(std::move(deadline)),
              _finish(_n, 0.0), _node(_n, -1), _avail(node_count, 0.0) {}

        void ini() override
        {
            std::fill(_finish.begin(), _finish.end(), 0.0);
            std::fill(_node.begin(),   _node.end(),   -1);
            std::fill(_avail.begin(),  _avail.end(),  0.0);
        }

        bool meet(int i, double v) override
        {
            return finish_at(i, static_cast<int>(v)) <= _deadline[i];
        }

        void update(int i, double v) override
        {
            const int ni = static_cast<int>(v);
            const double f = finish_at(i, ni);
            _finish[i] = f; _node[i] = ni; _avail[ni] = f;
        }

        void regionReduction(int i, domain_view& region) override
        {
            for (double v : region.enumerate())
                if (finish_at(i, static_cast<int>(v)) > _deadline[i])
                    region.remove_point(v);
        }

        double violation(double* x, int n) override
        {
            ini();
            double tot = 0.0;
            for (int i = 0; i < n; ++i)
            {
                const int ni = static_cast<int>(x[i]);
                const double f = finish_at(i, ni);
                _finish[i] = f; _node[i] = ni; _avail[ni] = f;
                tot += accumulate_excess(f, -std::numeric_limits<double>::infinity(), _deadline[i]);
            }
            return tot;
        }

        ConstrianLevel getConstrainLevel() override { return constraints_discrete; }
        Constrain* clone() override { return new ConstrainScheduleAccumulate(*this); }
    };

    // ===============================================================
    // Full multi-track constraint (multi-resource / multi-vehicle)
    // ===============================================================
    // Holds several ResourceTracks evolving in parallel along the sequence.
    // Configure tracks after construction via addCapacityTrack / addTimeTrack.
    class ConstrainAccumulate final : public Constrain
    {
    private:
        std::vector<std::unique_ptr<ResourceTrack>> _tracks;

    public:
        explicit ConstrainAccumulate(double penalty_w) : Constrain(penalty_w) {}

        // Capacity track: prefix accumulation along the sequence (delta may be ±).
        void addCapacityTrack(std::vector<double> delta_by_value,
                              double lower, double upper, double init = 0.0)
        {
            _tracks.emplace_back(new CapacityTrack(std::move(delta_by_value), lower, upper, init));
        }

        // Time track: DAG max-plus time advance + serial nodes + deadlines.
        void addTimeTrack(int node_count,
                          std::vector<std::vector<int>>    pred,
                          std::vector<std::vector<double>> exec,
                          CommModel                        comm,
                          std::vector<double>              deadline)
        {
            _tracks.emplace_back(new TimeTrack(node_count, std::move(pred), std::move(exec),
                                               std::move(comm), std::move(deadline)));
        }

        void ini() override
        {
            for (auto& t : _tracks) t->reset();
        }

        bool meet(int k, double v) override
        {
            for (const auto& t : _tracks)
                if (t->probe(k, v) > 0.0) return false;
            return true;
        }

        void update(int k, double v) override
        {
            for (auto& t : _tracks) t->commit(k, v);
        }

        double violation(double* x, int n) override
        {
            for (auto& t : _tracks) t->reset();
            double tot = 0.0;
            for (int k = 0; k < n; ++k)
                for (auto& t : _tracks)
                    tot += t->commit(k, x[k]);
            return tot;
        }

        void regionReduction(int k, domain_view& region) override
        {
            for (double v : region.enumerate())
            {
                bool infeasible = false;
                for (const auto& t : _tracks)
                    if (t->probe(k, v) > 0.0) { infeasible = true; break; }
                if (infeasible) region.remove_point(v);
            }
        }

        ConstrianLevel getConstrainLevel() override { return constraints_discrete; }

        Constrain* clone() override
        {
            auto* c = new ConstrainAccumulate(_w);
            c->_tracks.reserve(_tracks.size());
            for (const auto& t : _tracks)
                c->_tracks.emplace_back(t->clone());
            return c;
        }
    };

} // namespace ECFlow
