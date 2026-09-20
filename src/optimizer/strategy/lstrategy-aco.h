//------------------------Description------------------------
// 蚁群类学习策略:AntSystem(蚂蚁系统)与 AntColonySystem(蚁群系统)。基于信息素矩阵实现个体间的间接通信
//   与群体知识累积(stigmergy)。属"群体模型型"策略(向整个群体学习,见 docs/算子组件模型.md §6),忽略 learning_object、配 Isolate 拓扑。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <cmath>
#include <memory>
#include "evoflow-constant.h"
#include "evoflow-basicfunc.h"
#include "solution.h"
#include "individual.h"
#include "individual-array.h"
#include "problem-handle.h"
#include "solution-decoder.h"
#include "learning-strategy.h"
#include "best-archive.h"
#include "registry.h"

namespace EvoFlow
{
    struct Pheromone
    {
    private:
        double _low_bound;
        double _accuracy;
        double _threshold;

        bool validIndex(int did, int cid) const
        {
            return matrix && did >= 0 && did < demension && cid >= 0 && cid < choice;
        }

    public:
        std::string name;
        int demension;
        int choice;
        double* matrix;
        bool pre_base;
        bool bidiagraph;

        Pheromone()
        {
            _low_bound = 0;
            _accuracy = 0;
            _threshold = 0;
            name = "";
            demension = 0;
            choice = 0;
            matrix = nullptr;
            pre_base = false;
            bidiagraph = false;
        }

        ~Pheromone() { delete[] matrix; }

        void setVariable(const ElementNote& note, int variable_size)   // 只读 note → const&(适配 getNote 返回 const&)
        {
            switch (note._type)
            {
            case VariableType::discrete:
            case VariableType::allocation:
            {
                name = note._name;
                pre_base = false;
                bidiagraph = false;
                demension = variable_size;
                choice = int((note._upbound - note._lowbound) / note._accuracy) + 1;
                int size = demension * choice;
                delete[] matrix;
                matrix = new double[size];
                _low_bound = note._lowbound;
                _accuracy = note._accuracy;
                break;
            }
            case VariableType::sequence_direction:
            case VariableType::sub_module:
            {
                name = note._name;
                pre_base = true;
                bidiagraph = false;
                demension = int((note._upbound - note._lowbound) / note._accuracy) + 1;
                choice = int((note._upbound - note._lowbound) / note._accuracy) + 1;
                int size = demension * choice;
                delete[] matrix;
                matrix = new double[size];
                _low_bound = note._lowbound;
                _accuracy = note._accuracy;
                break;
            }
            case VariableType::sequence_bidiagraph:
            {
                name = note._name;
                pre_base = true;
                bidiagraph = true;
                demension = int((note._upbound - note._lowbound) / note._accuracy) + 1;
                choice = int((note._upbound - note._lowbound) / note._accuracy) + 1;
                int size = demension * choice;
                delete[] matrix;
                matrix = new double[size];
                _low_bound = note._lowbound;
                _accuracy = note._accuracy;
                break;
            }
            default:
                break;
            }
        }

        void setini(double value)
        {
            int count = 0;
            for (int i = 0; i < demension; i++)
                for (int j = 0; j < choice; j++)
                    matrix[count++] = value;
            _threshold = value * 1e-100;
        }

        double getChoice(int choiceid) { return _low_bound + choiceid * _accuracy; }   // id → 实际值

        bool tryGetCid(double choice_, int& cid) const   // 实际值 → 已验证 id
        {
            cid = -1;
            if (!std::isfinite(choice_) || !std::isfinite(_low_bound) ||
                !std::isfinite(_accuracy) || _accuracy <= 0.0 || choice <= 0)
                return false;

            const double high = _low_bound + _accuracy * (choice - 1);
            if (!std::isfinite(high) || choice_ < _low_bound || choice_ > high)
                return false;

            const double scaled = (choice_ - _low_bound) / _accuracy;
            if (!std::isfinite(scaled) || scaled < 0.0 || scaled > static_cast<double>(choice - 1))
                return false;

            cid = static_cast<int>(scaled);
            return cid >= 0 && cid < choice;
        }

        int getCid(double choice_)
        {
            int cid;
            return tryGetCid(choice_, cid) ? cid : -1;
        }

        // 单元素蒸发:did/cid 为矩阵下标(int)。守卫已上移到调用处 choice 层(is_empty),此处不再判 EMPTYVALUE。
        void evaporation(int did, int cid, double rate)
        {
            if (validIndex(did, cid)) matrix[did * choice + cid] *= rate;
        }

        void evaporation(double rate)   // 全矩阵蒸发
        {
            int size = demension * choice;
            for (int i = 0; i < size; i++)
                matrix[i] *= rate;
        }

        void release(int did, int cid, double value)
        {
            if (!validIndex(did, cid)) return;
            matrix[did * choice + cid] += value;
            if (bidiagraph)
                matrix[cid * choice + did] += value;
        }

        double& getValue(int did, int cid)
        {
            static thread_local double invalid_value = 0.0;
            if (!validIndex(did, cid))
            {
                invalid_value = 0.0;
                return invalid_value;
            }
            return matrix[did * choice + cid];
        }

        void clamp(double lo, double hi)   // MMAS:信息素夹到 [τ_min, τ_max]
        {
            int size = demension * choice;
            for (int i = 0; i < size; i++)
            {
                if (matrix[i] < lo) matrix[i] = lo;
                else if (matrix[i] > hi) matrix[i] = hi;
            }
        }
    };

    // 蚂蚁系统(Ant System)
    class AntSystem : public LearningStrategy
    {
    protected:   // protected:供 MaxMinAntSystem(MMAS)继承复用 pheromone/setProblem/nextDecision
        int variable_number;
        Pheromone** _pheromones;
        double _tao_ini;
        bool setted_tao;
        double _alpha;
        double _belta;
        double _rho;
        double* _h_list;
        int* variable_length;

        double priority(int dimension, double choice, int vid, int decision_base, int choiceid, ProblemHandle* handle)
        {
            double h = handle->getChoiceHeuristic(dimension, choice);
            return pow(_pheromones[vid]->getValue(decision_base, choiceid), _alpha) * pow(h, _belta);
        }

        void _deletePheromones()
        {
            for (int i = 0; i < variable_number; i++)
                delete _pheromones[i];
            delete[] _pheromones;
        }

    public:
        AntSystem(double alpha = 2, double belta = 2, double rho = 0.5, double tao_ini = EMPTYVALUE) : LearningStrategy()
        {
            variable_number = 0;
            _pheromones = nullptr;
            setted_tao = !is_empty(tao_ini);   // 修复:原 tao_ini==EMPTYVALUE 恒假
            _tao_ini = tao_ini;
            _alpha = alpha;
            _belta = belta;
            _rho = rho;
            _h_list = nullptr;
            variable_length = nullptr;
        }

        ~AntSystem()
        {
            _deletePheromones();
            delete[] _h_list;
            delete[] variable_length;
        }

        static void preAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 0, MatchType::notLessButNotice)); // 不需要学习目标
        }
        static void postAssert(AssertList& list, double* paras) { list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert)); }

        void ini(ProblemHandle* problem_handle) override
        {
            for (int i = 0; i < variable_number; i++)
                _pheromones[i]->setini(_tao_ini);
        }

        void setProblem(ProblemHandle* problem_handle) override
        {
            auto decoder = problem_handle->getSolutionDecoder();   // shared_ptr<SolutionDecoder>
            if (!setted_tao)
            {
                Solution solution;
                solution.setSize(problem_handle->getProblemSize(), problem_handle->getObjectNumber());
                problem_handle->getGreedyResult(solution);
                problem_handle->solutionEvaluate(solution);
                _tao_ini = 1 / (solution.fitness[0]);
            }

            if (variable_number != decoder->getVariableNumber())
            {
                _deletePheromones();
                variable_number = decoder->getVariableNumber();
                _pheromones = new Pheromone * [variable_number];
                for (int i = 0; i < variable_number; i++)
                    _pheromones[i] = new Pheromone();
                delete[] variable_length;
                variable_length = new int[variable_number];
            }

            for (int i = 0; i < variable_number; i++)
            {
                _pheromones[i]->setVariable(decoder->getNote(i), decoder->getVariableSize(i));   // decoder 访问器
                variable_length[i] = decoder->getVariableSize(i);
            }

            int max_size = -1;
            for (int i = 0; i < variable_number; i++)
                if (_pheromones[i]->choice > max_size)
                    max_size = _pheromones[i]->choice;
            delete[] _h_list;
            _h_list = new double[max_size + 1];
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual* child) override
        {
            double total = 0;
            double* feasible_raw = nullptr;
            int feasible_number = 0;
            problem_handle->getFeasibleList(decision_d, feasible_raw, feasible_number);
            std::unique_ptr<double[]> feasible_list(feasible_raw);

            if (feasible_number == 0)
                return EMPTYVALUE;

            int vid = problem_handle->getBelongVariableId(decision_d);
            int cid;

            int decision_base;
            if (_pheromones[vid]->pre_base)
            {
                double pre_decision;
                int id_in_variable = problem_handle->getWithinVariableId(decision_d);
                if (id_in_variable == 0)
                    pre_decision = problem_handle->getRandomChoiceInspace(decision_d);
                else
                    pre_decision = child->solution.result[decision_d - 1];
                if (!_pheromones[vid]->tryGetCid(pre_decision, decision_base))
                    return EMPTYVALUE;
            }
            else
            {
                decision_base = decision_d;
            }

            for (int j = 0; j < feasible_number; j++)
            {
                if (!_pheromones[vid]->tryGetCid(feasible_list[j], cid))
                {
                    _h_list[j] = 0.0;
                    continue;
                }
                _h_list[j] = priority(decision_d, feasible_list[j], vid, decision_base, cid, problem_handle);
                total += _h_list[j];
            }
            if (total == 0)
            {
                return feasible_list[EvoFlow::get_int(0, feasible_number - 1)];   // 修复:rand()%N
            }
            else
            {
                for (int j = 0; j < feasible_number; j++)
                    _h_list[j] /= total;
                total = rand01();
                for (int j = 0; j < feasible_number; j++)
                {
                    total -= _h_list[j];
                    if (notlarge(total, 0))
                        return feasible_list[j];
                }
            }
            return EMPTYVALUE;
        }

        void update_s(IndividualArray& population, IndividualArray& offspring, BestArchive* archive) override
        {
            // 信息素蒸发(全矩阵)
            for (int vid = 0; vid < variable_number; vid++)
                _pheromones[vid]->evaporation(1 - _rho);

            // 信息素释放(全体)
            int p_size = population.getSize();
            for (int i = 0; i < p_size; i++)
            {
                Solution* solution = &population[i].solution;
                double added = 1 / solution->fitness[0];
                int counter = 0;
                for (int vid = 0; vid < variable_number; vid++)
                {
                    for (int d = 0; d < variable_length[vid]; d++)
                    {
                        double cur = (*solution)[counter];
                        int cid;
                        if (!_pheromones[vid]->tryGetCid(cur, cid)) { counter++; continue; }
                        int did;
                        if (_pheromones[vid]->pre_base)
                        {
                            double base = (d == 0) ? (*solution)[counter + variable_length[vid] - 1] : (*solution)[counter - 1];
                            if (!_pheromones[vid]->tryGetCid(base, did)) { counter++; continue; }
                        }
                        else
                        {
                            did = d;
                        }
                        _pheromones[vid]->release(did, cid, added);
                        counter++;
                    }
                }
            }
        }
    };

    // 蚁群系统(Ant Colony System)
    class AntColonySystem : public LearningStrategy
    {
    private:
        int variable_number;
        Pheromone** _pheromones;
        double _tao_ini;
        bool setted_tao;
        int* variable_length;
        double _alpha;
        double _belta;
        double _rho_g;
        double _rho_l;
        double _q0;
        bool _use_global;   // 全局最优(archive getElite)释放开关
        bool _use_local;    // 当代种群最优(iteration-best)释放开关
        double* _h_list;

        double priority(int dimension, double choice, int vid, int decision_base, int choiceid, ProblemHandle* handle)
        {
            double h = handle->getChoiceHeuristic(dimension, choice);
            return pow(_pheromones[vid]->getValue(decision_base, choiceid), _alpha) * pow(h, _belta);
        }

        void _deletePheromones()
        {
            for (int i = 0; i < variable_number; i++)
                delete _pheromones[i];
            delete[] _pheromones;
        }

        // 全局信息素更新(从一条解释放):带 choice 层守卫
        void globalReleaseFrom(Solution* solution, double added)
        {
            int counter = 0;
            for (int vid = 0; vid < variable_number; vid++)
            {
                for (int d = 0; d < variable_length[vid]; d++)
                {
                    double cur = (*solution)[counter];
                    int cid;
                    if (!_pheromones[vid]->tryGetCid(cur, cid)) { counter++; continue; }
                    int did;
                    if (_pheromones[vid]->pre_base)
                    {
                        double base = (d == 0) ? (*solution)[counter + variable_length[vid] - 1] : (*solution)[counter - 1];
                        if (!_pheromones[vid]->tryGetCid(base, did)) { counter++; continue; }
                    }
                    else
                    {
                        did = d;
                    }
                    _pheromones[vid]->evaporation(did, cid, 1 - _rho_g);
                    _pheromones[vid]->release(did, cid, added);
                    counter++;
                }
            }
        }

    public:
        AntColonySystem(double alpha = 1, double belta = 2, double rho_g = 0.1, double rho_l = 0.1, double q0 = 0.9, int use_global = 1, int use_local = 1, double tao_ini = EMPTYVALUE) : LearningStrategy()
        {
            variable_number = 0;
            _pheromones = nullptr;
            variable_length = nullptr;
            setted_tao = !is_empty(tao_ini);   // 修复:原 equal(tao_ini,EMPTYVALUE) 恒假
            _tao_ini = tao_ini;
            _alpha = alpha;
            _belta = belta;
            _rho_g = rho_g;
            _rho_l = rho_l;
            _q0 = q0;
            _use_global = (use_global != 0);
            _use_local = (use_local != 0);
            _h_list = nullptr;
        }

        ~AntColonySystem()
        {
            _deletePheromones();
            delete[] _h_list;
            delete[] variable_length;
        }

        static void preAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 0, MatchType::notLessButNotice)); // 不需要学习目标
        }
        static void postAssert(AssertList& list, double* paras) { list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert)); }

        void ini(ProblemHandle* problem_handle) override
        {
            for (int i = 0; i < variable_number; i++)
                _pheromones[i]->setini(_tao_ini);
        }

        void setProblem(ProblemHandle* problem_handle) override
        {
            auto decoder = problem_handle->getSolutionDecoder();   // shared_ptr<SolutionDecoder>
            if (!setted_tao)
            {
                Solution solution;
                solution.setSize(problem_handle->getProblemSize(), problem_handle->getObjectNumber());
                problem_handle->getGreedyResult(solution);
                problem_handle->solutionEvaluate(solution);
                _tao_ini = 1.0 / solution.fitness[0] / solution.getSolutionSize();
            }

            if (variable_number != decoder->getVariableNumber())
            {
                _deletePheromones();
                variable_number = decoder->getVariableNumber();
                _pheromones = new Pheromone * [variable_number];
                for (int i = 0; i < variable_number; i++)
                    _pheromones[i] = new Pheromone();
                delete[] variable_length;
                variable_length = new int[variable_number];
            }

            for (int i = 0; i < variable_number; i++)
            {
                _pheromones[i]->setVariable(decoder->getNote(i), decoder->getVariableSize(i));   // decoder 访问器
                variable_length[i] = decoder->getVariableSize(i);
            }

            int max_size = -1;
            for (int i = 0; i < variable_number; i++)
                if (_pheromones[i]->choice > max_size)
                    max_size = _pheromones[i]->choice;
            delete[] _h_list;
            _h_list = new double[max_size + 1];
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual* child) override
        {
            double* feasible_raw = nullptr;
            int feasible_number = 0;
            problem_handle->getFeasibleList(decision_d, feasible_raw, feasible_number);
            std::unique_ptr<double[]> feasible_list(feasible_raw);

            if (feasible_number == 0)
                return EMPTYVALUE;

            int vid = problem_handle->getBelongVariableId(decision_d);

            int decision_base;
            if (_pheromones[vid]->pre_base)
            {
                double pre_decision;
                int id_in_variable = problem_handle->getWithinVariableId(decision_d);
                if (id_in_variable == 0)
                    pre_decision = problem_handle->getRandomChoiceInspace(decision_d);
                else
                    pre_decision = child->solution.result[decision_d - 1];
                if (!_pheromones[vid]->tryGetCid(pre_decision, decision_base))
                    return EMPTYVALUE;
            }
            else
            {
                decision_base = decision_d;
            }

            int cid;
            for (int j = 0; j < feasible_number; j++)
            {
                if (!_pheromones[vid]->tryGetCid(feasible_list[j], cid))
                {
                    _h_list[j] = 0.0;
                    continue;
                }
                _h_list[j] = priority(decision_d, feasible_list[j], vid, decision_base, cid, problem_handle);
            }

            if (rand01() < _q0)   // q0 贪心:选最优
            {
                int backid = 0;
                double max_p = _h_list[0];
                for (int j = 1; j < feasible_number; j++)
                    if (_h_list[j] > max_p) { max_p = _h_list[j]; backid = j; }
                return feasible_list[backid];
            }
            else   // 轮盘赌
            {
                double total = 0;
                for (int j = 0; j < feasible_number; j++)
                    total += _h_list[j];
                if (total == 0)
                {
                    return feasible_list[EvoFlow::get_int(0, feasible_number - 1)];   // 修复:rand()%N
                }
                else
                {
                    for (int j = 0; j < feasible_number; j++)
                        _h_list[j] /= total;
                    total = rand01();
                    for (int j = 0; j < feasible_number; j++)
                    {
                        total -= _h_list[j];
                        if (notlarge(total, 0))
                            return feasible_list[j];
                    }
                }
            }
            return EMPTYVALUE;
        }

        void update_d(Individual* child, const int decision_d) override
        {
            // 局部更新:定位 vid / 变量内维度 did
            int vid = 0;
            int offset = variable_length[0];
            while (offset < decision_d) { vid++; offset += variable_length[vid]; }
            offset -= variable_length[vid];
            int did = decision_d - offset;

            if (_pheromones[vid]->pre_base)
            {
                if (did == 0) return;   // 首维无前决策
                double pre = child->solution[decision_d - 1];
                double cur = child->solution[decision_d];
                int preid, cid;
                if (!_pheromones[vid]->tryGetCid(pre, preid) ||
                    !_pheromones[vid]->tryGetCid(cur, cid)) return;
                _pheromones[vid]->evaporation(preid, cid, 1 - _rho_l);
                _pheromones[vid]->release(preid, cid, _rho_l * _tao_ini);
            }
            else
            {
                double cur = child->solution[decision_d];
                int cid;
                if (!_pheromones[vid]->tryGetCid(cur, cid)) return;
                _pheromones[vid]->evaporation(did, cid, 1 - _rho_l);
                _pheromones[vid]->release(did, cid, _rho_l * _tao_ini);
            }
        }

        void update_s(IndividualArray& population, IndividualArray& offspring, BestArchive* archive) override
        {
            // 全局更新:当代种群最优(use_local) 与/或 archive 全局最优(use_global,走 archive 路线不自持 gbest)
            if (_use_local)
            {
                int bestid = 0;
                int p_size = population.getSize();
                for (int i = 1; i < p_size; i++)
                    if (population[i] < population[bestid])
                        bestid = i;
                if (p_size > 0)
                {
                    Solution* best = &population[bestid].solution;
                    if (best->fitness[0] > 0.0)
                        globalReleaseFrom(best, _rho_g / best->fitness[0]);
                }
            }

            if (_use_global && archive && archive->getBestSize() > 0)
            {
                Solution* elite = archive->getElite();
                if (elite && elite->fitness[0] > 0.0)
                    globalReleaseFrom(elite, _rho_g / elite->fitness[0]);
            }
        }
    };

    // 最大最小蚂蚁系统(Max-Min Ant System, Stützle & Hoos):信息素上下界 [τ_min,τ_max] clamp + best-only 更新,防早熟停滞。
    //   继承 AntSystem(复用 pheromone/setProblem/nextDecision 轮盘赌);override ini(初始 τ_max)/update_s(best-only+clamp)。
    //   τ_max=1/(ρ·f(best)) 动态;τ_min=τ_max·(1-ⁿ√p_best)/((avg-1)·ⁿ√p_best)(Stützle,n=维数、avg=平均可选数)。
    //   释放开关 use_local(当代最优 iteration-best)、use_global(全局最优 archive getElite),两者可同时开(仿 ACS 双释放);τ_max 取释放集合中更优者。
    class MaxMinAntSystem : public AntSystem
    {
    private:
        double _p_best;       // Stützle τ_min 参数
        bool   _use_global;   // 全局最优(archive getElite)释放开关
        bool   _use_local;    // 当代种群最优(iteration-best)释放开关
        double _tau_max, _tau_min;
        int    _n;            // 决策维数
        double _avg;          // 平均可选数

        void _recomputeBounds(double best_fitness)
        {
            if (best_fitness <= 0.0) return;
            _tau_max = 1.0 / (_rho * best_fitness);
            int nn = (_n < 1) ? 1 : _n;
            double root = std::pow(_p_best, 1.0 / nn);                         // ⁿ√p_best
            double denom = (_avg - 1.0) * root; if (denom < 1e-12) denom = 1e-12;
            _tau_min = _tau_max * (1.0 - root) / denom;                        // Stützle
            if (_tau_min < 0.0) _tau_min = 0.0;
            if (_tau_min > _tau_max) _tau_min = _tau_max;
        }

        // 从一条解释放(逐维,仿 AntSystem,带 pre_base/empty 守卫)
        void _releaseFrom(Solution* best, double added)
        {
            int counter = 0;
            for (int vid = 0; vid < variable_number; vid++)
                for (int d = 0; d < variable_length[vid]; d++)
                {
                    double cur = (*best)[counter];
                    int cid;
                    if (!_pheromones[vid]->tryGetCid(cur, cid)) { counter++; continue; }
                    int did;
                    if (_pheromones[vid]->pre_base)
                    {
                        double base = (d == 0) ? (*best)[counter + variable_length[vid] - 1] : (*best)[counter - 1];
                        if (!_pheromones[vid]->tryGetCid(base, did)) { counter++; continue; }
                    }
                    else did = d;
                    _pheromones[vid]->release(did, cid, added);
                    counter++;
                }
        }

    public:
        MaxMinAntSystem(double alpha = 2, double belta = 2, double rho = 0.5, double p_best = 0.05, int use_global = 0, int use_local = 1)
            : AntSystem(alpha, belta, rho, EMPTYVALUE), _p_best(p_best), _use_global(use_global != 0), _use_local(use_local != 0),
              _tau_max(1.0), _tau_min(0.0), _n(1), _avg(2.0) {}

        void setProblem(ProblemHandle* problem_handle) override
        {
            AntSystem::setProblem(problem_handle);   // 分配 pheromones + 自算 _tao_ini=1/f(greedy_best)
            _n = problem_handle->getProblemSize(); if (_n < 1) _n = 1;
            double sum = 0; for (int i = 0; i < variable_number; i++) sum += _pheromones[i]->choice;
            _avg = (variable_number > 0) ? sum / variable_number : 2.0; if (_avg < 2.0) _avg = 2.0;
            _recomputeBounds(1.0 / _tao_ini);        // f(greedy_best)=1/_tao_ini → 初始 τ_max=_tao_ini/ρ
        }

        void ini(ProblemHandle*) override
        {
            for (int i = 0; i < variable_number; i++) _pheromones[i]->setini(_tau_max);   // 初始 τ_max(强探索)
        }

        void update_s(IndividualArray& population, IndividualArray& offspring, BestArchive* archive) override
        {
            for (int vid = 0; vid < variable_number; vid++) _pheromones[vid]->evaporation(1 - _rho);   // 全矩阵蒸发

            // 候选来源:当代种群最优(iteration-best) 与 archive 全局最优(global-best)
            Solution* iter_best = nullptr;
            {
                int ps = population.getSize();
                if (ps > 0)
                {
                    int bid = 0;
                    for (int i = 1; i < ps; i++) if (population[i] < population[bid]) bid = i;
                    iter_best = &population[bid].solution;
                }
            }
            Solution* glob_best = (archive && archive->getBestSize() > 0) ? archive->getElite() : nullptr;

            // 按 use_local/use_global 选定释放集合(global 缺失时回退 iter);两者可同时释放(仿 ACS)
            Solution* rel_iter = _use_local ? iter_best : nullptr;
            Solution* rel_glob = _use_global ? (glob_best ? glob_best : iter_best) : nullptr;

            // τ_max 用释放集合中更优的一条(fitness 更小者)动态计算
            Solution* bound_best = nullptr;
            if (rel_iter && rel_iter->fitness[0] > 0.0 && (!bound_best || rel_iter->fitness[0] < bound_best->fitness[0])) bound_best = rel_iter;
            if (rel_glob && rel_glob->fitness[0] > 0.0 && (!bound_best || rel_glob->fitness[0] < bound_best->fitness[0])) bound_best = rel_glob;
            if (!bound_best) return;
            _recomputeBounds(bound_best->fitness[0]);

            // 逐条释放(当代/全局,视模式;避免同一指针重复释放)
            if (rel_iter && rel_iter->fitness[0] > 0.0) _releaseFrom(rel_iter, 1.0 / rel_iter->fitness[0]);
            if (rel_glob && rel_glob != rel_iter && rel_glob->fitness[0] > 0.0) _releaseFrom(rel_glob, 1.0 / rel_glob->fitness[0]);

            for (int vid = 0; vid < variable_number; vid++) _pheromones[vid]->clamp(_tau_min, _tau_max);   // clamp [τ_min,τ_max]
        }
    };

    inline Registry<LearningStrategy>::Entry antSystemEntry()
    {
        return { "AntSystem", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"alpha",   ParamKind::Real, 1.0, 10.0, false, 1, 3},
                                 {"belta",   ParamKind::Real, 1.0, 10.0, false, 2, 5},
                                 {"rho",     ParamKind::Real, 0.0, 1.0,  false, 0.1, 0.5},
                                 {"tao_ini", ParamKind::Real, 0.0, 0x3f3f3f3f, true, 0, 0} } }, sizeof(AntSystem),
            [](const double* p) -> LearningStrategy* {
                return p ? new AntSystem(p[0], p[1], p[2], p[3]) : new AntSystem();
            },
            [](AssertList& L, const double* p) { AntSystem::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { AntSystem::postAssert(L, const_cast<double*>(p)); } };
    }
    inline Registry<LearningStrategy>::Entry antColonySystemEntry()
    {
        return { "AntColonySystem", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"alpha",      ParamKind::Real, 0.0, 10.0, false, 1, 2},
                                 {"belta",      ParamKind::Real, 0.0, 10.0, false, 2, 5},
                                 {"rho_g",      ParamKind::Real, 0.0, 1.0,  false, 0.05, 0.2},
                                 {"rho_l",      ParamKind::Real, 0.0, 1.0,  false, 0.05, 0.2},
                                 {"q0",         ParamKind::Real, 0.0, 1.0,  false, 0.7, 0.95},
                                 {"use_global", ParamKind::Enum, 0, 1,      false, 1, 1},   // 全局最优(archive)释放开关
                                 {"use_local",  ParamKind::Enum, 0, 1,      false, 1, 1},   // 当代最优(iteration-best)释放开关
                                 {"tao_ini",    ParamKind::Real, 0.0, 0x3f3f3f3f, true, 0, 0} } }, sizeof(AntColonySystem),
            [](const double* p) -> LearningStrategy* {
                return p ? new AntColonySystem(p[0], p[1], p[2], p[3], p[4], (int)p[5], (int)p[6], p[7]) : new AntColonySystem();
            },
            [](AssertList& L, const double* p) { AntColonySystem::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { AntColonySystem::postAssert(L, const_cast<double*>(p)); } };
    }
    inline Registry<LearningStrategy>::Entry maxMinAntSystemEntry()
    {
        return { "MaxMinAntSystem", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"alpha",      ParamKind::Real, 1.0, 10.0, false, 1, 3},
                                 {"belta",      ParamKind::Real, 1.0, 10.0, false, 2, 5},
                                 {"rho",        ParamKind::Real, 0.0, 1.0,  false, 0.02, 0.2},
                                 {"p_best",     ParamKind::Real, 0.0, 1.0,  false, 0.02, 0.1},   // Stützle τ_min 参数
                                 {"use_global", ParamKind::Enum, 0, 1,      false, 0, 1},   // 全局最优(archive)释放开关
                                 {"use_local",  ParamKind::Enum, 0, 1,      false, 1, 1} } }, sizeof(MaxMinAntSystem),  // 当代最优(iteration-best)释放开关
            [](const double* p) -> LearningStrategy* {
                return p ? new MaxMinAntSystem(p[0], p[1], p[2], p[3], (int)p[4], (int)p[5]) : new MaxMinAntSystem();
            },
            [](AssertList& L, const double* p) { MaxMinAntSystem::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { MaxMinAntSystem::postAssert(L, const_cast<double*>(p)); } };
    }
    EVOFLOW_REGISTER(lstrat_antsystem,       LearningStrategy, antSystemEntry());
    EVOFLOW_REGISTER(lstrat_antcolonysystem, LearningStrategy, antColonySystemEntry());
    EVOFLOW_REGISTER(lstrat_maxminant,       LearningStrategy, maxMinAntSystemEntry());
}
