//------------------------Description------------------------
// ProblemHandle:四域 XXManager 的组合 + 跨域编排,由 Problem::compile() 产出。
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
#include <cstring>
#include <memory>

#include "variable-manager.h"
#include "objective-manager.h"
#include "constraint-manager.h"
#include "heuristic-manager.h"
#include "solution.h"
#include "solution-decoder.h"
#include "ecflow-math.h"        // equal(), intelliTrunc()
#include "ecflow-constant.h"

namespace ECFlow
{
    class ProblemHandle final
    {
    private:
        std::string       _name;
        VariableManager   _var;
        ObjectiveManager  _obj;
        ConstraintManager _con;
        HeuristicManager  _heu;
        std::shared_ptr<SolutionDecoder> _decoder;

        void selfBuild()
        {
            std::vector<ElementNote> dnotes = _var.getDecisionVariableNotes();
            _decoder = std::make_shared<SolutionDecoder>(
                dnotes.data(), _var.decision_variable_number,
                _obj.objectives, _obj.object_number);
        }

    public:
        ProblemHandle(VariableManager&& var, ObjectiveManager&& obj,
                      ConstraintManager&& con, HeuristicManager&& heu,
                      const std::string& name)
            : _name(name), _var(std::move(var)), _obj(std::move(obj)),
              _con(std::move(con)), _heu(std::move(heu)), _decoder(nullptr)
        {
            selfBuild();
        }

        ProblemHandle(const ProblemHandle& s)
            : _name(s._name), _var(s._var), _obj(s._obj), _con(s._con), _heu(s._heu),
              _decoder(nullptr)
        {
            selfBuild();
        }
        ProblemHandle& operator=(const ProblemHandle&) = delete;
        ~ProblemHandle() = default;

        // ---------------- accessors ----------------
        std::string getName()    { return _name; }
        int getProblemSize()     { return _var.getProblemSize(); }
        int getObjectNumber()    { return _obj.objectNumber(); }
        int getVariableNumber()  { return _var.getVariableNumber(); }
        int getBelongVariableId(int dim) { return _var.getBelongVariableId(dim); }
        int getWithinVariableId(int dim) { return _var.getWithinVariableId(dim); }
        int getVariableOffset(int vid)   { return _var.getVariableOffset(vid); }
        int getVariableLength(int vid)   { return _var.getVariableLength(vid); }
        VariableType getVariableType(int vid) { return _var.getVariableType(vid); }
        double getVariableUpbound(int dim)  { return _var.getVariableUpbound(dim); }
        double getVariableLowbound(int dim) { return _var.getVariableLowbound(dim); }

        std::shared_ptr<SolutionDecoder> getSolutionDecoder() { return _decoder; }
        Comparer* getSolutionComparer() { return _obj.getComparer(); }

        // ---------------- objective manager interfaces (delegated) ----------------
        int  getObjectiveId(const std::string& name) const { return _obj.getObjectiveId(name); }
        int  getObjectivePriority(int oid)           const { return _obj.getObjectivePriority(oid); }
        bool changePriority(int oid, int p)                { return _obj.changePriority(oid, p); }
        bool changePriority(const std::string& n, int p)   { return _obj.changePriority(n, p); }
        void setEvaluateMode(bool full)                    { _obj.setEvaluateMode(full); }
        bool getEvaluateMode() const                       { return _obj.getEvaluateMode(); }

        // ---------------- value binding (variable domain) ----------------
        void setResult(const Solution& s) { _var.setResult(s); }
        void setResult(double* s)          { _var.setResult(s); }
        void changeEnv(const double* values, int variableId)
        {
            std::memcpy(_var.variables[variableId].address, values,
                        _var.variables[variableId].getLength() * sizeof(double));
        }

        // ---------------- constraint workflow (constraint domain) ----------------
        void constrainReset()                          { _con.constrainReset(); }
        bool constrainCheck(int dim, double value)     { return _con.constrainCheck(_var, dim, value); }
        void constrainChange(int dim, double value)    { _con.constrainChange(_var, dim, value); }

        // 违反度 = **全部已注册约束**的**原始** violation 之和(**不乘惩罚权重、不看目标绑定**)。
        //   与原版 ECFlow 逐行等价。平台有**两条**约束通路,勿混:
        //     * `constraintViolation` / `solutionLegality` —— **用户 API**,原始违反量,**优化器不调用**;
        //     * `getPenalty4Object`(见 solutionEvaluate) —— **评估路径**,`violation × 权重`,且**仅算绑定到该目标**的约束。
        //   该分工在原版即如此,非 v2 引入。
        // **含变量域**:`compile()` 自动为每个决策变量补的 range 约束**计入**本值 ——
        //   变量域是用户在 `addVariable(name, lo, hi, ...)` 时**亲手声明**的,域外解确实违反了用户的声明。
        //   原版报的数不含域,只因它**根本不检查域**(缺口),非有意语义;故此处不是"与原版分歧",而是原版漏检。
        //   自动补的域约束 `penalty_w=0` 且未绑定目标 → **只收缩可行域、不进任何目标的惩罚**,仅在本值中体现。
        //   用户为某变量显式声明 range 时**不再自动补**(否则同一处越界被算两遍),详见 Problem::compile()。
        double constraintViolation(const Solution& s) { setResult(s); return _con.constraintViolation(_var); }

        // 是否完全不违反约束。**或逻辑短路**:撞到第一条违反即返回,不再计算其余约束(与原版 ECFlow 同构)。
        //   v2 曾改写为 `equal(constraintViolation(s), 0.0)` —— 在 violation 恒非负时**结果等价**,
        //   但**总要算完全部约束**;v1.4.6.5 改回短路(非法解常在头几条即可判定)。约束集合与
        //   constraintViolation 完全一致(含自动补的变量域约束),故二者结论恒一致:legality ⟺ violation==0。
        bool solutionLegality(const Solution& s)
        {
            setResult(s);
            return _con.solutionLegality(_var);
        }

        // ---------------- evaluation (cross-domain: var + obj + con) ----------------
        void solutionEvaluate(Solution& solution)
        {
            setResult(solution);
            bool full = _obj.getEvaluateMode();
            for (int oid = 0; oid < _obj.objectNumber(); oid++)
            {
                // partial mode: non-participating (priority < 0) objectives are left
                // empty entirely -- no fitness, no constraint penalty (see docs F.2).
                if (!full && _obj.getObjectivePriority(oid) < 0)
                { solution.fitness[oid] = EMPTYVALUE; continue; }
                double fitness = _obj.getFitness(oid, _var);
                double penalty = _con.getPenalty4Object(_var, oid);
                if (!_obj.isMin(oid)) penalty *= -1.0;
                solution.fitness[oid] = fitness + penalty;
            }
        }

        // ---------------- feasible-choice queries (constraint domain) ----------------
        // ⚠️ 长度提醒：getChoiceNumber 返回真实可行点数；getFeasibleList 受全局上限
        //    FEASIBLE_LIST_LIMIT 降采样，真实点数超限时其长度 < getChoiceNumber。
        //    二者长度不必一致，不要用 getChoiceNumber 当 getFeasibleList 的数组长度。
        int     getChoiceNumber(int dim)            { return _con.getChoiceNumber(_var, dim); }
        double* getFeasibleList(int dim)            { return _con.getFeasibleList(_var, dim); }
        // by-ref overload: fills list_buffer (caller owns -> delete[]) AND returns its size.
        // 注意：size 是降采样后的实际长度，可能 < getChoiceNumber(dim)（见上）。
        void    getFeasibleList(int dim, double*& list_buffer, int& size)
        { _con.ensureDem(_var, dim); list_buffer = _con.demView(_var, dim).enumerate_alloc_capped(FEASIBLE_LIST_LIMIT, size); }
        double  getCloseChoice(int dim, double v)   { return _con.getCloseChoice(_var, dim, v); }
        double  getBoundaryChoice(int dim, bool left = true) { return _con.getBoundaryChoice(_var, dim, left); }
        double  getRandomChoice(int dim)            { return _con.getRandomChoice(_var, dim); }
        double  getRandomChoiceInspace(int dim)     { return _con.getRandomChoiceInspace(_var, dim); }
        // 把决策值落到该维的合法取值上(生成器每写一个决策都经此)。
        //   落点**唯一在视图**(domain_view::snap):离散→吸附到格点、连续→原值返回。
        double  choiceDiscretized(int dim, double value) { return _con.snapValue(_var, dim, value); }

        // ---------------- heuristic choices (cross-domain: con region + heu) ----------------
        double getChoiceHeuristic(int dim, double choice)
        {
            _con.ensureDem(_var, dim);
            domain_view dv = _con.demView(_var, dim);
            return _heu.getHeuristic(_var.getBelongVariableId(dim), dv, _var.getWithinVariableId(dim), choice, _var);
        }
        double getPrioriChoice(int dim)
        {
            _con.ensureDem(_var, dim);
            domain_view dv = _con.demView(_var, dim);
            return _heu.getPrioriDecision(_var.getBelongVariableId(dim), dv, _var.getWithinVariableId(dim), _var);
        }
        void getPrioriChoice(int dim, int size, double* output)
        {
            _con.ensureDem(_var, dim);
            domain_view dv = _con.demView(_var, dim);
            int bufsize;
            double* buf = _heu.getPrioriOrder(_var.getBelongVariableId(dim), dv, _var.getWithinVariableId(dim), bufsize, _var);
            if (bufsize < size) size = bufsize;
            std::memcpy(output, buf, size * sizeof(double));
            delete[] buf;
        }

        // ---------------- greedy construction (cross-domain) ----------------
        void getGreedyResult(Solution& solution)
        {
            setResult(solution);
            constrainReset();
            for (int i = 0; i < _var.getProblemSize(); i++)
            {
                solution[i] = getPrioriChoice(i);
                constrainChange(i, solution[i]);
            }
        }

        // ---------------- subproblem extraction (TODO: 未来) ----------------
        // 原 ProblemHandle 提供按变量 / 按目标抽取子问题的能力，迁移中暂未实现。
        // 与 parked 的变量结构编辑 / 问题分解同方向，留作未来一并落地。
        //   void getSubproblemForVariable(int* remove_variableId, int variable_number);
        //   void getSubproblemForObject(int* remove_objectId, int object_number);
    };
}
