//------------------------Description------------------------
// Problem:问题定义总装类——持有四域 XXDefine,compile() 组合为 ProblemHandle。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <string>
#include "variable-define.h"
#include "objective-define.h"
#include "constrain-define.h"
#include "heuristic-define.h"
#include "problem-handle.h"

// -------------------------------------------------------
// 域进约束开关(风格沿用 ECFC原代码 ecflow-builder.h 的 ECFLOW_DISABLE_BUILD_VALIDATION)
// -------------------------------------------------------

// 打开此宏以禁用 compile() **自动为每个决策变量补一条域(range)约束**的行为。
//   该机制(DOMAIN-CLAMP)使生成路径中的出界值被 constrainCheck 判违 → 交 repairman 夹回。
//   **禁用的后果**:出界值不再被 constrainCheck 拦截,加性算子(如 Gauss 变异)产出的解可能停留在域外;
//   同时 `constraintViolation`/`solutionLegality` 将不再反映变量域越界。仅在自行接管域约束时禁用。
// #define ECFLOW_DISABLE_AUTO_DOMAIN_CONSTRAIN

namespace ECFlow
{
    class Problem
    {
    private:
        std::string     _name;
        VariableDefine  _varDef;
        ObjectiveDefine _objDef;
        ConstrainDefine _conDef;
        HeuristicDefine _heuDef;
        bool            _compile_check = true;

    public:
        explicit Problem(const std::string& problem_name) : _name(problem_name) {}

        // ---------------- variable ----------------
        int addConstant(const std::string& name, double* values, int length, int height = 1)
        { return _varDef.addConstant(name, values, length, height); }
        int addVariable(const std::string& name, double low_bound, double up_bound, double accuracy,
                        int length, int height = 1, VariableType type = VariableType::discrete)
        { return _varDef.addVariable(name, low_bound, up_bound, accuracy, length, height, type); }

        // ---------------- objective ----------------
        void addObjective(const std::string& name, int priority, bool min_is_better,
                          const std::string& input_elements, double (*f)(double**))
        { _objDef.addObjective(name, priority, min_is_better, input_elements, f); }
        void addObjective(const std::string& name, int priority, bool min_is_better,
                          const std::string& input_elements, eccalcul_functor* f)
        { _objDef.addObjective(name, priority, min_is_better, input_elements, f); }
        void addObjective(const std::string& name, int priority, bool min_is_better,
                          const std::string& input_elements, const std::string& formula)
        { _objDef.addObjective(name, priority, min_is_better, input_elements, formula); }
        // definition-side initial evaluation mode (carried into the handle by compile())
        void setEvaluateMode(bool full) { _objDef.setEvaluateMode(full); }
        bool getEvaluateMode() const    { return _objDef.getEvaluateMode(); }

        // ---------------- constraint ----------------
        void changeConstrainDeal(bool no_check) { _conDef.changeConstrainDeal(no_check); }
        void addConstrain(const std::string& input, void (*ini)(), double (*chk)(int,double),
                          void (*chg)(int,double), double w = 1, const std::string& objs = "")
        { _conDef.addConstrain(input, ini, chk, chg, w, objs); }
        void addConstrainRange(const std::string& input, double left = EMPTYVALUE, double right = EMPTYVALUE,
                               double w = 1, const std::string& objs = "")
        { _conDef.addConstrainRange(input, left, right, w, objs); }
        void addConstrainCompatibility(const std::string& input, const double* value, int length,
                                       double w = 1, const std::string& objs = "")
        { _conDef.addConstrainCompatibility(input, value, length, w, objs); }
        void addConstrainEligible(const std::string& input, std::vector<std::vector<double>> allowed,
                                  double w = 1, const std::string& objs = "")
        { _conDef.addConstrainEligible(input, std::move(allowed), w, objs); }
        void addConstrainUnique(const std::string& input, double w = 1, const std::string& objs = "")
        { _conDef.addConstrainUnique(input, w, objs); }
        void addConstrainDistinctCap(const std::string& input, int p, double w = 1, const std::string& objs = "")
        { _conDef.addConstrainDistinctCap(input, p, w, objs); }
        void addConstrainMinDistance(const std::string& input, double gap, double w = 1, const std::string& objs = "")
        { _conDef.addConstrainMinDistance(input, gap, w, objs); }
        void addConstrainMinDistance(const std::string& input, double* gap, double w = 1, const std::string& objs = "")
        { _conDef.addConstrainMinDistance(input, gap, w, objs); }
        void addConstrainCapacity(const std::string& input, double* caps, int nC, double* vols, int nI,
                                  double w = 1, const std::string& objs = "")
        { _conDef.addConstrainCapacity(input, caps, nC, vols, nI, w, objs); }
        void addConstrainDistributed(const std::string& input, double* vals, int size, int* nums,
                                     double w = 1, const std::string& objs = "")
        { _conDef.addConstrainDistributed(input, vals, size, nums, w, objs); }
        void addConstrainExpr(const std::string& input_var, const std::string& input_elements,
                              const std::string& formula, double w = 1, const std::string& objs = "")
        { _conDef.addConstrainExpr(input_var, input_elements, formula, w, objs); }
        void addConstrainSequenceAccumulate(const std::string& input_var, std::vector<double> delta,
                                            double lower, double upper, double init = 0.0,
                                            double w = 1, const std::string& objs = "")
        { _conDef.addConstrainSequenceAccumulate(input_var, std::move(delta), lower, upper, init, w, objs); }
        void addConstrainScheduleAccumulate(const std::string& input_var, int node_count,
                                            std::vector<std::vector<int>> pred, std::vector<std::vector<double>> exec,
                                            CommModel comm, std::vector<double> deadline,
                                            double w = 1, const std::string& objs = "")
        { _conDef.addConstrainScheduleAccumulate(input_var, node_count, std::move(pred), std::move(exec), std::move(comm), std::move(deadline), w, objs); }
        ConstrainAccumulate* addConstrainAccumulate(const std::string& input_var, double w = 1, const std::string& objs = "")
        { return _conDef.addConstrainAccumulate(input_var, w, objs); }
        void addConstrainGraphIndependent(const std::string& input_var, const std::vector<int>& edges,
                                          double w = 1, const std::string& objs = "")
        { _conDef.addConstrainGraphIndependent(input_var, edges, w, objs); }
        void addConstrainGraphDistinct(const std::string& input_var, const std::vector<int>& edges,
                                       double w = 1, const std::string& objs = "")
        { _conDef.addConstrainGraphDistinct(input_var, edges, w, objs); }
        void addConstrainGraphClique(const std::string& input_var, const std::vector<int>& edges,
                                     double w = 1, const std::string& objs = "")
        { _conDef.addConstrainGraphClique(input_var, edges, w, objs); }
        void addConstrainGraphConflict(const std::string& input_var, const std::vector<int>& edges,
                                       bool (*conflict)(double, double), double w = 1, const std::string& objs = "")
        { _conDef.addConstrainGraphConflict(input_var, edges, conflict, w, objs); }
        void addConstrainGraphDominating(const std::string& input_var, const std::vector<int>& edges,
                                         double w = 1, const std::string& objs = "")
        { _conDef.addConstrainGraphDominating(input_var, edges, w, objs); }
        void addConstrainGraphVertexCover(const std::string& input_var, const std::vector<int>& edges,
                                          double w = 1, const std::string& objs = "")
        { _conDef.addConstrainGraphVertexCover(input_var, edges, w, objs); }
        void addConstrainNodeSum(const std::string& input_var, int n_vertices, const std::vector<int>& edges,
                                 bool directed, const std::vector<double>& lo, const std::vector<double>& hi,
                                 const std::vector<double>& cap, double w = 1, const std::string& objs = "")
        { _conDef.addConstrainNodeSum(input_var, n_vertices, edges, directed, lo, hi, cap, w, objs); }
        void addConstrainGraphConnectivity(const std::string& input_var, int n_vertices, const std::vector<int>& edges,
                                           const std::vector<int>& terminals = {}, double w = 1, const std::string& objs = "")
        { _conDef.addConstrainGraphConnectivity(input_var, n_vertices, edges, terminals, w, objs); }

        // ---------------- heuristic ----------------
        void addInspirationRandom(const std::string& v)   { _heuDef.addInspirationRandom(v); }
        void addInspirationBoundary(const std::string& v) { _heuDef.addInspirationBoundary(v); }
        void addInspirationFunc(const std::string& v, const std::string& input, double (*f)(double**), bool stable = false)
        { _heuDef.addInspirationFunc(v, input, f, stable); }
        void addInspirationFunc(const std::string& v, const std::string& input, eccalcul_functor* f, bool stable = false)
        { _heuDef.addInspirationFunc(v, input, f, stable); }
        void addInspirationMatrix(const std::string& v, double* matrix, int nDec, int nDim)
        { _heuDef.addInspirationMatrix(v, matrix, nDec, nDim); }
        void addInspirationExpr(const std::string& v, const std::string& input, const std::string& formula, bool stable = false)
        { _heuDef.addInspirationExpr(v, input, formula, stable); }

        // ---------------- compile (point-to-point composition) ----------------
        ProblemHandle* compile()
        {
            VariableManager vm = _varDef.compile();
            if (_compile_check && !_objDef.checkCompleteness(vm)) return nullptr;
            ObjectiveManager om = _objDef.compile(vm);
            if (_compile_check && (!_conDef.checkCompleteness(vm, om) || !_heuDef.checkCompleteness(vm))) return nullptr;

            // 域进约束(修 DOMAIN-CLAMP):为每个决策变量补一条 range 约束,使 constrainCheck 覆盖变量域界
            //   → 生成路径中出界值(如加性变异 Gauss)被 constrainCheck 判违 → repairman 夹回。
            //   在 _conDef 的**副本**上加、compile 时生成:不污染定义态、重编不累积;range 默认参数自动填当前 [lo,hi];
            //   常量无需域约束,跳过。
            //   ① **查重** —— 用户已为该变量声明 range 时**不再自动补**。原先"与用户已加的更紧 range 共存"看似无害
            //      (constrainCheck 取交=更紧者),但 `constraintViolation` 是**遍历全部已注册约束累加原始违反量**,
            //      两条 range 会让**同一处越界被算两遍**(consistency-original 的 s2:x 越界 10 → 报 20.6)。
            //   ② **penalty_w 传 0** —— 自动补的约束**仅收缩可行域、不施加惩罚**。注:它 objs 为空、未绑定任何目标,
            //      故本就不进 getPenalty4Object;传 0 是把该意图**写进代码**,避免日后有人给它绑目标时意外双重惩罚。
            //   ③ 违反度**照常计入**(不排除):变量域是用户在 `addVariable(name, lo, hi, ...)` 时**亲手声明**的,
            //      域外解确实违反了用户的声明。原版 ECFlow 不计入,只因它**根本不检查域**(缺口),非有意语义。
            ConstrainDefine conDefWithDomain = _conDef;
#ifndef ECFLOW_DISABLE_AUTO_DOMAIN_CONSTRAIN
            for (int i = 0; i < _varDef.getVariableNumber(); i++)
            {
                if (!_varDef.isDecisionVariable(i)) continue;
                const std::string& vname = _varDef.getVariableName(i);
                if (conDefWithDomain.hasRangeConstrain(vname)) continue;   // ① 用户已声明 → 不重复添加
                conDefWithDomain.addConstrainRange(vname, EMPTYVALUE, EMPTYVALUE, 0);   // 边界自动填 [lo,hi];② 惩罚系数 0
            }
#endif

            ConstraintManager cm = conDefWithDomain.compile(vm, om);
            HeuristicManager  hm = _heuDef.compile(vm, cm);
            return new ProblemHandle(std::move(vm), std::move(om), std::move(cm), std::move(hm), _name);
        }
        void setCompileCheck(bool enable) { _compile_check = enable; }

        // Reset all definition data (variables/objectives/constraints/heuristics) by
        // delegating to each define's in-place clear(); keeps the problem name.
        void clear()
        {
            _varDef.clear();
            _objDef.clear();
            _conDef.clear();
            _heuDef.clear();
            _compile_check = true;
        }

        // ---------------- accessors (on the definition) ----------------
        // [已移除] 定义态查询为不必要的新增：原 Problem 无此接口；尺寸/编号/变量 id
        // 等应在 compile 后经 ProblemHandle 获取。按接口审查移除，保留注释以记录。
        // int getProblemSize()      const { return 0; }
        // int getVariableNumber()   const { return _varDef.getVariableNumber(); }
        // int getObjectiveNumber()  const { return _objDef.getObjectiveNumber(); }
        // int getConstraintNumber() const { return _conDef.getConstraintNumber(); }
        // int getHeuristicNumber()  const { return _heuDef.getHeuristicNumber(); }
        // int getVariableId(const std::string& name) const { return _varDef.getVariableId(name); }
    };
}
