//------------------------Description------------------------
// 带容量设施选址 (Capacitated Facility Location Problem) 问题模板。
//-------------------------Reference-------------------------
// Avella P., Boccia M. (2009) 等的 SSCFLP test bed(FrontierCO 的 CFLP 基准来源)。
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
#include "logger.hpp"

namespace ECFlow
{
    class PT_CFLP
    {
    private:
        std::string         _name = "cflp";
        int                 _F = 0, _M = 0;
        std::vector<double> _cap, _fcost, _dem, _c;   // cap[F], fcost[F], dem[M], c[M*F]
        double              _penalty = 0;

        // 目标:Σ 分配成本 + Σ 已开设施开设成本。容量改由 ConstrainCapacity 硬约束承担。
        struct costFunc : eccalcul_functor
        {
            int F, M; std::vector<double> fcost, c;
            costFunc(int f, int m, std::vector<double> fc, std::vector<double> cc)
                : F(f), M(m), fcost(std::move(fc)), c(std::move(cc)) {}
            double operator()(double** a) const
            {
                std::vector<char> open(F, 0);
                double cost = 0;
                for (int i = 0; i < M; ++i) { int j = static_cast<int>(a[0][i] + 0.5); cost += c[i * F + j]; open[j] = 1; }
                for (int j = 0; j < F; ++j) if (open[j]) cost += fcost[j];   // 已开设施的开设成本
                return cost;
            }
            eccalcul_functor* copy() { return new costFunc(*this); }
        };
        struct nearestFunc : eccalcul_functor   // 偏好分配成本最低的设施
        {
            int F; std::vector<double> c;
            nearestFunc(int f, std::vector<double> cc) : F(f), c(std::move(cc)) {}
            double operator()(double** in) const
            {
                int i = static_cast<int>(in[0][0] + 0.5), j = static_cast<int>(in[1][0] + 0.5);
                return -c[i * F + j];
            }
            eccalcul_functor* copy() { return new nearestFunc(*this); }
        };

    public:
        PT_CFLP() {}
        void setName(std::string name) { _name = name; }
        int  getProblemSize() { return _M; }
        void setPenalty(double p) { _penalty = p; }

        void setData(int F, int M, std::vector<double> cap, std::vector<double> fcost,
                     std::vector<double> dem, std::vector<double> c)
        { _F = F; _M = M; _cap = std::move(cap); _fcost = std::move(fcost); _dem = std::move(dem); _c = std::move(c); }

        void save(bool overwrite = false)
        {
            std::string path = "_pdata/cflp/" + _name + ".cflp";
            if (!overwrite) { std::ifstream ex(path); if (ex.good()) { sys_logger.error("CFLP save: file exists (use overwrite): " + path); return; } }
            std::ofstream out(path);
            if (!out) { sys_logger.error("CFLP save: cannot write " + path); return; }
            out << "NAME: " << _name << "\nTYPE: CFLP\nFACILITIES: " << _F << "\nCUSTOMERS: " << _M << "\n";
            out << "CAPACITY_SECTION\n";  for (int j = 0; j < _F; ++j) out << (j + 1) << " " << _cap[j]   << "\n";
            out << "OPENCOST_SECTION\n";  for (int j = 0; j < _F; ++j) out << (j + 1) << " " << _fcost[j] << "\n";
            out << "DEMAND_SECTION\n";    for (int i = 0; i < _M; ++i) out << (i + 1) << " " << _dem[i]   << "\n";
            out << "COST_SECTION\n";
            for (int i = 0; i < _M; ++i) { out << (i + 1); for (int j = 0; j < _F; ++j) out << " " << _c[(size_t)i * _F + j]; out << "\n"; }
        }

        Problem* getProblem()
        {
            if (_F == 0 || _M == 0) return nullptr;
            Problem* back = new Problem(_name);
            back->addVariable("x", 0, _F - 1, 1, _M, 1, VariableType::allocation);   // 每客户→设施(结构性保证恰一个)
            double pen = (_penalty > 0) ? _penalty : 1e6;
            costFunc cf(_F, _M, _fcost, _c);
            back->addObjective("cost", 1, true, "x", &cf);                           // 最小化 分配+开设 成本
            back->addConstrainCapacity("x", _cap.data(), _F, _dem.data(), _M, pen, "cost");  // 设施容量(硬:构造期缩减+violation 兜底)
            nearestFunc nf(_F, _c);
            back->addInspirationFunc("x", "x", &nf);
            return back;
        }

        void load(std::string name)
        {
            setName(instanceName(name));
            std::ifstream f(resolveInstancePath(name, "cflp", "cflp"));
            if (!f) { sys_logger.error("CFLP instance not found: " + name); return; }
            int F = 0, M = 0; std::string tok; bool ok = true;
            std::vector<double> cap, fcost, dem, c;
            auto need = [&](bool needM) -> bool   // 段必须在 FACILITIES/CUSTOMERS 计数之后
            { if (F <= 0 || (needM && M <= 0)) { sys_logger.error("CFLP '" + name + "': section before FACILITIES/CUSTOMERS"); return false; } return true; };
            // 统一行号规范:每逻辑行以 1-based 索引开头(读入即忽略)。向量=每元素一行 `idx val`;矩阵=每行 `idx v0..v_{cols-1}`。
            std::string idx;
            auto rdv = [&](std::vector<double>& v, int n) { v.assign(n, 0); for (int i = 0; i < n && ok; ++i) if (!(f >> idx >> v[i])) ok = false; };
            auto rdm = [&](std::vector<double>& v, int rows, int cols) { v.assign((size_t)rows * cols, 0); for (int r = 0; r < rows && ok; ++r) { if (!(f >> idx)) { ok = false; break; } for (int cc = 0; cc < cols && ok; ++cc) if (!(f >> v[(size_t)r * cols + cc])) ok = false; } };
            while (ok && f >> tok)
            {
                if (tok == "FACILITIES:") f >> F;
                else if (tok == "CUSTOMERS:") f >> M;
                else if (tok == "CAPACITY_SECTION") { if (!(ok = need(false))) break; rdv(cap,   F); }
                else if (tok == "OPENCOST_SECTION") { if (!(ok = need(false))) break; rdv(fcost, F); }
                else if (tok == "DEMAND_SECTION")   { if (!(ok = need(true)))  break; rdv(dem,   M); }
                else if (tok == "COST_SECTION")     { if (!(ok = need(true)))  break; rdm(c,     M, F); }
                else if (!tok.empty() && tok.back() == ':') { std::string rest; std::getline(f, rest); }
            }
            if (!ok) { sys_logger.error("CFLP '" + name + "': section truncated"); return; }
            if (F <= 0 || M <= 0) { sys_logger.error("CFLP '" + name + "': missing FACILITIES/CUSTOMERS"); return; }
            setData(F, M, cap, fcost, dem, c);
        }
    };
}
