//------------------------Description------------------------
// 符号回归 / 基因表达式编程 (Symbolic Regression / Gene Expression Programming) 问题模板。
//-------------------------Reference-------------------------
// Ferreira (2001) Gene Expression Programming（GEP 定长头/尾编码 + Karva 解码 + RNC 常量域）。
// DEAP gp 工程实践（primitive set + 保护算子 + ephemeral 随机常量）。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------
#pragma once
#include "pt-instance-path.h"   // v1.4.9:load 路径自适应(短名/完整路径)
#include "problem.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>
#include <cmath>
#include "logger.hpp"
#include "ecflow-rand.h"

namespace ECFlow
{
    class PT_SR
    {
    public:
        // 内置函数目录(预支持范围;扩展只需在 arityOf/applyFunc 各加一行)。
        enum class Func { Add, Sub, Mul, Div, Neg, Sin, Cos, Exp, Log, Sqrt };
        enum class ObjMode { Regression, Heuristic };

        static int arityOf(Func f)
        {
            return (f == Func::Add || f == Func::Sub || f == Func::Mul || f == Func::Div) ? 2 : 1;
        }
        // 保护算子(避免 inf/nan,DEAP 风格)。args 长度 = arity。
        static double applyFunc(Func f, const double* a)
        {
            switch (f)
            {
            case Func::Add:  return a[0] + a[1];
            case Func::Sub:  return a[0] - a[1];
            case Func::Mul:  return a[0] * a[1];
            case Func::Div:  return (std::fabs(a[1]) < 1e-9) ? 1.0 : a[0] / a[1];   // 保护除
            case Func::Neg:  return -a[0];
            case Func::Sin:  return std::sin(a[0]);
            case Func::Cos:  return std::cos(a[0]);
            case Func::Exp:  { double v = a[0]; if (v > 50) v = 50; if (v < -50) v = -50; return std::exp(v); }
            case Func::Log:  return std::log(std::fabs(a[0]) + 1e-9);               // 保护对数
            case Func::Sqrt: return std::sqrt(std::fabs(a[0]));                     // 保护开方
            }
            return 0.0;
        }

        // 解码后的可求值表达式(扁平节点 + 子下标),两模式公用。
        struct Expr
        {
            struct Node { int type; Func f; int var; double cval; int ch[2]; int nch; }; // type:0=func,1=var,2=const
            std::vector<Node> nodes;
            int size() const { return static_cast<int>(nodes.size()); }
            double eval(const double* feat) const { return nodes.empty() ? 0.0 : ev(0, feat); }
            double ev(int i, const double* feat) const
            {
                const Node& n = nodes[i];
                if (n.type == 1) return feat[n.var];
                if (n.type == 2) return n.cval;
                double a[2];
                for (int k = 0; k < n.nch; ++k) a[k] = ev(n.ch[k], feat);
                return applyFunc(n.f, a);
            }
        };

        // 解码所需的全部静态参数(值拷贝进各 functor)。
        struct GepSpec
        {
            std::vector<Func>   funcs;     // 活动函数集(顺序定 id)
            int                 nVar = 1;  // 终端变量/特征个数
            int                 head = 10; // h
            int                 tail = 11; // t
            std::vector<double> consts;    // 常量池 C[Kc]
            int F()   const { return static_cast<int>(funcs.size()); }
            int qId() const { return F() + nVar; }            // `?` 符号 id
            int S()   const { return F() + nVar + 1; }        // 符号总数
        };

        // Karva(BFS/层序)解码:g[head+tail] + dc[tail] → Expr。每个 `?` 终端按序取 consts[dc[...]]。
        static Expr decode(const GepSpec& sp, const double* g, const double* dc)
        {
            Expr e;
            int L = sp.head + sp.tail;
            e.nodes.reserve(L);
            int dcPtr = 0;
            int kc = sp.consts.empty() ? 1 : static_cast<int>(sp.consts.size());
            auto makeNode = [&](int sym) -> int {
                Expr::Node n{}; n.nch = 0; n.ch[0] = n.ch[1] = -1;
                if (sym < sp.F())                 { n.type = 0; n.f = sp.funcs[sym]; n.nch = arityOf(sp.funcs[sym]); }
                else if (sym < sp.qId())          { n.type = 1; n.var = sym - sp.F(); }
                else                              { n.type = 2; int idx = static_cast<int>(dc[dcPtr % sp.tail] + 0.5) % kc; if (idx < 0) idx += kc; n.cval = sp.consts.empty() ? 0.0 : sp.consts[idx]; ++dcPtr; }
                e.nodes.push_back(n);
                return static_cast<int>(e.nodes.size()) - 1;
            };
            auto symAt = [&](int pos) -> int { int s = static_cast<int>(g[pos] + 0.5); if (s < 0) s = 0; if (s >= sp.S()) s = sp.S() - 1; return s; };
            int pos = 0;
            int root = makeNode(symAt(pos++));
            std::queue<int> q; q.push(root);
            while (!q.empty())
            {
                int ni = q.front(); q.pop();
                int a = e.nodes[ni].nch;
                for (int c = 0; c < a && pos < L; ++c)
                {
                    int ci = makeNode(symAt(pos++));   // push_back 可能 realloc → 之后用下标回填
                    e.nodes[ni].ch[c] = ci;
                    q.push(ci);
                }
            }
            return e;
        }

        // 模式二:用户为目标问题实现(可选)。把 inspiration 原始输入映射成问题语义特征。
        struct SrFeatureMap
        {
            virtual int  count() const = 0;
            virtual void extract(double** in, double* outFeat) const = 0;
            virtual ~SrFeatureMap() {}
        };

    private:
        std::string         _name = "sr";
        ObjMode             _mode = ObjMode::Regression;
        std::vector<Func>   _funcs;                 // 空 → 默认 {Add,Sub,Mul,Div}
        int                 _head = 10;             // 头长(默认;setHeadLength / 数据 HEAD: 覆盖)
        bool                _headSet = false;       // 显式 setHeadLength 优先于数据文件
        int                 _kc = 10;               // 常量池大小
        double              _clo = -1.0, _chi = 1.0;// 常量池区间
        std::vector<double> _constsExplicit;        // 显式常量池(非空则用之)
        double              _lambda = 0.0;          // 简约惩罚系数
        bool                _minimize = true;       // 目标方向
        // 回归数据
        int                 _dataVars = 0, _nPoints = 0;
        std::vector<double> _X, _Y;                 // X:[nPoints*dataVars], Y:[nPoints]
        // 启发式目标
        Problem*            _target = nullptr;
        std::string         _targetVar, _ruleInput;
        SrFeatureMap*       _fmap = nullptr;

        std::vector<Func> activeFuncs() const
        {
            if (!_funcs.empty()) return _funcs;
            return { Func::Add, Func::Sub, Func::Mul, Func::Div };
        }
        std::vector<double> buildConsts() const
        {
            if (!_constsExplicit.empty()) return _constsExplicit;
            std::vector<double> c(_kc > 0 ? _kc : 1);
            for (size_t i = 0; i < c.size(); ++i) c[i] = _clo + ECFlow::rand01() * (_chi - _clo);
            return c;
        }

        // 模式一:RMSE(+λ·size)。
        struct regObjFunc : eccalcul_functor
        {
            GepSpec spec; std::vector<double> X, Y; int nPoints; double lambda;
            regObjFunc(GepSpec s, std::vector<double> x, std::vector<double> y, int np, double lam)
                : spec(std::move(s)), X(std::move(x)), Y(std::move(y)), nPoints(np), lambda(lam) {}
            double operator()(double** a) const
            {
                Expr e = decode(spec, a[0], a[1]);
                double se = 0;
                for (int i = 0; i < nPoints; ++i)
                {
                    double pred = e.eval(&X[static_cast<size_t>(i) * spec.nVar]);
                    if (!std::isfinite(pred)) pred = 1e12;
                    double d = pred - Y[i]; se += d * d;
                }
                double rmse = std::sqrt(se / (nPoints > 0 ? nPoints : 1));
                return rmse + lambda * e.size();
            }
            eccalcul_functor* copy() { return new regObjFunc(*this); }
        };

        // 模式二的透传规则 functor:作为目标问题的 inspiration。共享 Expr,SR 每代更新。
        struct GepRuleFunc : eccalcul_functor
        {
            std::shared_ptr<Expr> rule; SrFeatureMap* fmap; int nFeat;
            GepRuleFunc(std::shared_ptr<Expr> r, SrFeatureMap* fm, int nf) : rule(std::move(r)), fmap(fm), nFeat(nf) {}
            double operator()(double** in) const
            {
                std::vector<double> feat(nFeat > 0 ? nFeat : 1, 0.0);
                if (fmap) fmap->extract(in, feat.data());
                else { feat[0] = in[1][0]; if (nFeat > 1) feat[1] = in[0][0]; }   // 默认 A:{候选值,维度}
                return rule ? rule->eval(feat.data()) : 0.0;
            }
            eccalcul_functor* copy() { return new GepRuleFunc(*this); }
        };

        // 模式二目标:解码→发布到共享规则→目标句柄 getGreedyResult→评估,取 fitness[0]。
        struct heurObjFunc : eccalcul_functor
        {
            GepSpec spec; std::shared_ptr<Expr> rule; std::shared_ptr<ProblemHandle> tgt; double lambda;
            mutable Solution sol; mutable bool sized = false;
            heurObjFunc(GepSpec s, std::shared_ptr<Expr> r, std::shared_ptr<ProblemHandle> t, double lam)
                : spec(std::move(s)), rule(std::move(r)), tgt(std::move(t)), lambda(lam) {}
            double operator()(double** a) const
            {
                *rule = decode(spec, a[0], a[1]);                 // 发布给目标 inspiration 的所有副本
                if (!sized) { sol.setSize(tgt->getProblemSize(), tgt->getObjectNumber()); sized = true; }
                tgt->getGreedyResult(sol);
                tgt->solutionEvaluate(sol);
                double f = sol.fitness[0];
                if (!std::isfinite(f)) f = 1e12;
                return f + lambda * rule->size();
            }
            eccalcul_functor* copy() { return new heurObjFunc(*this); }
        };

    public:
        PT_SR() {}
        void setName(std::string name) { _name = name; }
        int  getProblemSize() { GepSpec sp = makeSpecSizes(); return (sp.head + sp.tail) + sp.tail; } // |g| + |dc|
        ObjMode getMode() const { return _mode; }

        // ---- 函数集(内置目录内逐个/批量添加;不加则默认 {+,−,×,÷}) ----
        void addFunction(Func f) { _funcs.push_back(f); }
        void addFunctions(std::initializer_list<Func> fs) { for (Func f : fs) _funcs.push_back(f); }
        void clearFunctions() { _funcs.clear(); }

        // ---- 头长(默认 10;显式 set 优先于数据 HEAD:) ----
        void setHeadLength(int h) { if (h > 0) { _head = h; _headSet = true; } }
        int  getHeadLength() const { return _head; }

        // ---- 常量(GEP-RNC) ----
        void setConstantPool(int count, double lo, double hi) { _kc = count; _clo = lo; _chi = hi; _constsExplicit.clear(); }
        void setConstants(std::vector<double> c) { _constsExplicit = std::move(c); _kc = static_cast<int>(_constsExplicit.size()); }

        void setParsimony(double lambda) { _lambda = lambda; }    // 简约惩罚(默认 0)
        void setMinimize(bool m) { _minimize = m; }               // 目标方向(回归恒 min;启发式按目标问题)

        // ---- 模式一:回归数据 ----
        void setData(int nVars, std::vector<double> X, std::vector<double> Y)
        { _mode = ObjMode::Regression; _dataVars = nVars; _X = std::move(X); _Y = std::move(Y); _nPoints = static_cast<int>(_Y.size()); }

        void save(bool overwrite = false)
        {
            std::string path = "_pdata/sr/" + _name + ".sr";
            if (!overwrite) { std::ifstream ex(path); if (ex.good()) { sys_logger.error("SR save: file exists (use overwrite): " + path); return; } }
            std::ofstream out(path);
            if (!out) { sys_logger.error("SR save: cannot write " + path); return; }
            out << "NAME: " << _name << "\nTYPE: SR\nVARIABLES: " << _dataVars << "\nPOINTS: " << _nPoints << "\nHEAD: " << _head << "\nDATA_SECTION\n";
            for (int p = 0; p < _nPoints; ++p)
            {
                for (int j = 0; j < _dataVars; ++j) out << _X[(size_t)p * _dataVars + j] << " ";
                out << _Y[p] << "\n";
            }
        }

        // ---- 模式二:注入目标问题(透传规则 + 复用 getGreedyResult) ----
        void setTarget(Problem* target, const std::string& var, const std::string& ruleInput = "")
        { _mode = ObjMode::Heuristic; _target = target; _targetVar = var; _ruleInput = ruleInput; }
        void setFeatureMap(SrFeatureMap* fm) { _fmap = fm; }

        Problem* getProblem()
        {
            GepSpec sp;
            sp.funcs = activeFuncs();
            int nMax = 1; for (Func f : sp.funcs) nMax = std::max(nMax, arityOf(f));
            sp.head = _head;
            sp.tail = sp.head * (nMax - 1) + 1;
            sp.consts = buildConsts();
            int nVar;
            if (_mode == ObjMode::Regression) nVar = (_dataVars > 0) ? _dataVars : 1;
            else                              nVar = _fmap ? _fmap->count() : 2;     // 默认 A:{候选值,维度}
            sp.nVar = nVar;

            int L = sp.head + sp.tail, t = sp.tail, S = sp.S(), F = sp.F();
            int kc = static_cast<int>(sp.consts.size());

            Problem* back = new Problem(_name);
            back->addVariable("g",  0, S - 1, 1, L);                 // 头/尾符号
            back->addVariable("dc", 0, (kc > 0 ? kc - 1 : 0), 1, t); // 常量下标(RNC),默认随机→ephemeral

            // 目标
            if (_mode == ObjMode::Regression)
            {
                if (_nPoints == 0) { delete back; return nullptr; }
                regObjFunc of(sp, _X, _Y, _nPoints, _lambda);
                back->addObjective("fitness", 1, true, "g,dc", &of);  // RMSE 恒最小化
            }
            else
            {
                if (!_target) { delete back; return nullptr; }
                auto rule = std::make_shared<Expr>();                 // 共享可变规则(初始空树)
                GepRuleFunc rf(rule, _fmap, nVar);
                _target->addInspirationFunc(_targetVar, _ruleInput, &rf);   // 透传进目标
                ProblemHandle* th = _target->compile();
                if (!th) { delete back; return nullptr; }
                std::shared_ptr<ProblemHandle> tgt(th);
                heurObjFunc of(sp, rule, tgt, _lambda);
                back->addObjective("fitness", 1, _minimize, "g,dc", &of);
            }

            // ConstrainEligible:头任意、尾仅终端(保证生成解可解码;违反兜底惩罚)。
            std::vector<double> allSyms, termSyms;
            for (int s = 0; s < S; ++s) allSyms.push_back(s);
            for (int s = F; s < S; ++s) termSyms.push_back(s);
            std::vector<std::vector<double>> allowed(L);
            for (int p = 0; p < L; ++p) allowed[p] = (p < sp.head) ? allSyms : termSyms;
            back->addConstrainEligible("g", allowed, 1e6, "fitness");
            return back;
        }

        void load(std::string name)
        {
            setName(instanceName(name));
            std::ifstream f(resolveInstancePath(name, "sr", "sr"));
            if (!f) { sys_logger.error("SR instance not found: " + name); return; }
            int d = 0, np = 0, hd = 0; bool ok = true; std::string tok;
            std::vector<double> X, Y;
            while (ok && f >> tok)
            {
                if      (tok == "VARIABLES:") f >> d;
                else if (tok == "POINTS:")    f >> np;
                else if (tok == "HEAD:")      f >> hd;
                else if (tok == "DATA_SECTION")
                {
                    if (d <= 0 || np <= 0) { sys_logger.error("SR '" + name + "': DATA_SECTION before VARIABLES/POINTS"); return; }
                    for (int i = 0; i < np && ok; ++i)
                    {
                        for (int j = 0; j < d && ok; ++j) { double v; if (!(f >> v)) { ok = false; break; } X.push_back(v); }
                        double y; if (ok && !(f >> y)) { ok = false; break; }
                        if (ok) Y.push_back(y);
                    }
                }
                else if (!tok.empty() && tok.back() == ':') { std::string rest; std::getline(f, rest); }
            }
            if (!ok) { sys_logger.error("SR '" + name + "': DATA_SECTION truncated"); return; }
            if (d <= 0 || np <= 0) { sys_logger.error("SR '" + name + "': missing/invalid VARIABLES/POINTS"); return; }
            if (hd > 0 && !_headSet) _head = hd;   // 数据文件 HEAD: 设头长(显式 setHeadLength 优先)
            setData(d, std::move(X), std::move(Y));
        }

    private:
        // 仅为 getProblemSize 计算 L/t,不构造问题。
        GepSpec makeSpecSizes() const
        {
            GepSpec sp; sp.funcs = activeFuncs();
            int nMax = 1; for (Func f : sp.funcs) nMax = std::max(nMax, arityOf(f));
            sp.head = _head; sp.tail = sp.head * (nMax - 1) + 1;
            return sp;
        }
    };
}
