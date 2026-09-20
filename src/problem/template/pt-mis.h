//------------------------Description------------------------
// 最大独立集 (Maximum Independent Set) 问题模板。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include"problem-template-registry.h"
#include "pt-instance-path.h"   // v1.4.9:load 路径自适应(短名/完整路径)
#include "problem.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include "logger.hpp"

namespace EvoFlow
{
    class PT_MIS : public ProblemTemplate
    {
    private:
        std::string      _name = "mis";
        int              _n = 0;          // 顶点数
        std::vector<int> _edges;          // 原始扁平 0 基边列表
        double           _penalty = 0;    // 0 = 默认 1e6
        bool             _degree_sort = true;
        std::vector<int> _perm;           // 新序 i → 原顶点 _perm[i]

        struct sizeFunc : eccalcul_functor
        {
            int n;
            explicit sizeFunc(int n_) : n(n_) {}
            double operator()(double** a) const { double s = 0; for (int i = 0; i < n; ++i) s += a[0][i]; return s; }
            eccalcul_functor* copy() { return new sizeFunc(n); }
        };
        static double prefer_select(double** in) { return in[1][0]; }   // 可行则偏好取 1

    public:
        PT_MIS() {}
        // functor 由 add* 在 add 时即拷贝(见 CalcSpec),故模板无需持有 functor 成员、传局部即可。

        void setName(std::string name) override { _name = name; }
        void clear() override { *this = PT_MIS(); }   // 无自持裸资源:复位到默认构造态
        int  getProblemSize() { return _n; }
        void setPenalty(double p) { _penalty = p; }          // ≤0 → 默认 1e6
        void setDegreeSort(bool on) { _degree_sort = on; }   // 度感知预排序开关(默认开)
        const std::vector<int>& getPermutation() const { return _perm; }   // 新序→原顶点

        void setGraph(int n, const int* edges, int num_edges) { _n = n; _edges.assign(edges, edges + 2 * num_edges); }
        void setGraph(int n, const std::vector<int>& edges)   { _n = n; _edges = edges; }

        // v1.4.9(T-SAVE):回写 EvoFlow 格式 _pdata/mis/<name>.mis(overwrite=false 时不覆盖)。
        void save(bool overwrite = false) override
        {
            std::string path = "_pdata/mis/" + _name + ".mis";
            if (!overwrite) { std::ifstream ex(path); if (ex.good()) { sys_logger.error("MIS save: file exists (use overwrite): " + path); return; } }
            std::ofstream out(path);
            if (!out) { sys_logger.error("MIS save: cannot write " + path); return; }
            int m = (int)_edges.size() / 2;
            out << "NAME: " << _name << "\nTYPE: MIS\nVERTICES: " << _n << "\nEDGES: " << m << "\nEDGE_SECTION\n";
            for (int k = 0; k < m; ++k) out << (_edges[2 * k] + 1) << " " << (_edges[2 * k + 1] + 1) << "\n";  // 0 基 → 1 基
        }

        Problem* getProblem() override
        {
            if (_n == 0) return nullptr;

            // 预排序：按度升序得 新序→原 的 _perm；恒等(关排序时)。
            _perm.resize(_n);
            for (int i = 0; i < _n; ++i) _perm[i] = i;
            if (_degree_sort)
            {
                std::vector<int> deg(_n, 0);
                for (size_t k = 0; k < _edges.size(); k += 2) { deg[_edges[k]]++; deg[_edges[k + 1]]++; }
                std::stable_sort(_perm.begin(), _perm.end(), [&](int a, int b) { return deg[a] < deg[b]; });
            }
            std::vector<int> inv(_n);                               // 原顶点 → 新序
            for (int i = 0; i < _n; ++i) inv[_perm[i]] = i;
            std::vector<int> redges(_edges.size());                 // 重标号边
            for (size_t k = 0; k < _edges.size(); ++k) redges[k] = inv[_edges[k]];

            Problem* back = new Problem(_name);
            back->addVariable("x", 0, 1, 1, _n);
            sizeFunc sf(_n);                                        // 局部:add 时即被拷贝
            back->addObjective("size", 1, false, "x", &sf);        // 最大化 |S|
            double pen = (_penalty > 0) ? _penalty : 1e6;
            back->addConstrainGraphIndependent("x", redges, pen, "size");
            back->addInspirationFunc("x", "", prefer_select);
            return back;
        }

        // 读 EvoFlow 标准格式(TSPLIB 风格)：KEY: value 头 + EDGE_SECTION 段(边 1 基)。
        void load(std::string name) override
        {
            setName(instanceName(name));
            std::ifstream f(resolveInstancePath(name, "mis", "mis"));
            if (!f) { sys_logger.error("MIS instance not found: " + name); return; }
            int n = 0;
            std::vector<int> e;
            std::string tok;
            while (f >> tok)
            {
                if (tok == "VERTICES:") f >> n;
                else if (tok == "EDGE_SECTION")
                { int u, v; while (f >> u >> v) { e.push_back(u - 1); e.push_back(v - 1); } }   // 1 基 → 0 基
                else if (!tok.empty() && tok.back() == ':') { std::string rest; std::getline(f, rest); }  // 跳过其它 KEY: value
            }
            setGraph(n, e);
        }
    };
}

// —— 自注册到 ProblemTemplateRegistry(按 TYPE 分派;loadProblem 查表构造)——
EVOFLOW_REGISTER_PROBLEM(mis, "MIS", EvoFlow::PT_MIS);
