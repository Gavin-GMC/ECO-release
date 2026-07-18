//------------------------Description------------------------
// CMA-ES(协方差矩阵自适应进化策略)学习策略:维护多元正态 N(m,σ²C),从中采样、用选出父代更新 m/C/σ/进化路径。
//   分布型策略(类 EDA/ES),连续优化。topology=Isolate、selector=Rank((μ+λ))、individual=Individual(状态全在策略)。
//-------------------------Reference-------------------------
// Hansen & Ostermeier CMA-ES(标准公式,参照 purecmaes)。特征分解用自实现 jacobiEigen(ecflow-eigen.h)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
//-----------------------------------------------------------

#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include "solution.h"
#include "individual.h"
#include "individual-array.h"
#include "learning-strategy.h"
#include "ecflow-rand.h"
#include "ecflow-eigen.h"
#include "registry.h"

namespace ECFlow
{
    class CMAES : public LearningStrategy
    {
    private:
        int _n, _lambda, _mu, _gen;
        double _sigma, _sigma0;
        double _cc, _cs, _c1, _cmu, _damps, _chiN, _mueff;
        std::vector<double> _weights;         // μ 个权重(归一)
        std::vector<double> _m, _pc, _ps;     // 均值 / 进化路径(n)
        std::vector<double> _C, _B;           // 协方差 / 特征向量(n×n)
        std::vector<double> _D;               // 特征值平方根(n)
        bool _inited;

        void _setupParams()
        {
            _mu = _lambda / 2; if (_mu < 1) _mu = 1;
            _weights.assign(_mu, 0.0);
            double sumw = 0, sumw2 = 0;
            for (int i = 0; i < _mu; i++) _weights[i] = std::log(_mu + 0.5) - std::log((double)(i + 1));
            for (double w : _weights) sumw += w;
            for (double& w : _weights) w /= sumw;
            for (double w : _weights) sumw2 += w * w;
            _mueff = 1.0 / sumw2;
            double n = _n;
            _cc    = (4.0 + _mueff / n) / (n + 4.0 + 2.0 * _mueff / n);
            _cs    = (_mueff + 2.0) / (n + _mueff + 5.0);
            _c1    = 2.0 / ((n + 1.3) * (n + 1.3) + _mueff);
            _cmu   = std::min(1.0 - _c1, 2.0 * (_mueff - 2.0 + 1.0 / _mueff) / ((n + 2.0) * (n + 2.0) + _mueff));
            _damps = 1.0 + 2.0 * std::max(0.0, std::sqrt((_mueff - 1.0) / (n + 1.0)) - 1.0) + _cs;
            _chiN  = std::sqrt(n) * (1.0 - 1.0 / (4.0 * n) + 1.0 / (21.0 * n * n));
        }

    public:
        CMAES(double sigma = 0.5) : _n(0), _lambda(0), _mu(0), _gen(0), _sigma(sigma), _sigma0(sigma), _inited(false) {}
        ~CMAES() {}

        void ini(ProblemHandle*) override { _inited = false; _sigma = _sigma0; _gen = 0; }
        void setProblem(ProblemHandle* problem_handle) override { _n = problem_handle->getProblemSize(); }

        static void preAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 0, MatchType::notLessButNotice)); // 0 学习目标
        }

        void preparation_s(IndividualArray& population, Terminator*) override
        {
            int n = _n;
            if (!_inited)
            {
                _lambda = population.getSize();
                _setupParams();
                _m.assign(n, 0.0);
                for (int k = 0; k < population.getSize(); k++)
                    for (int d = 0; d < n; d++) _m[d] += population[k].solution.result[d];
                for (int d = 0; d < n; d++) _m[d] /= population.getSize();   // 初始均值=种群均值
                _pc.assign(n, 0.0); _ps.assign(n, 0.0);
                _C.assign((size_t)n * n, 0.0); _B.assign((size_t)n * n, 0.0);
                for (int i = 0; i < n; i++) { _C[i * n + i] = 1.0; _B[i * n + i] = 1.0; }   // C=B=I
                _D.assign(n, 1.0);
                _sigma = _sigma0; _gen = 0; _inited = true;
                return;   // 首代:仅初始化,采样用 m/σ/C=I
            }

            // 收敛/退化冻结:σ 缩到极小(离散网格下过度收敛)时,y=(x-m)/σ 会除以极小值发散 → 冻结分布更新
            //   (等价真 CMA-ES 的 TolX/TolFun 停止准则;ECFlow 跑满 FES 无此准则,故内建守卫)。继续以当前紧分布采样。
            if (!std::isfinite(_sigma) || _sigma < 1e-12) return;

            population.sort();   // 最优在前
            std::vector<double> m_old = _m;

            // 各选中父代的标准化偏移 y_i、新均值 m
            std::vector<std::vector<double>> yk(_mu, std::vector<double>(n));
            std::vector<double> m_new(n, 0.0);
            for (int i = 0; i < _mu; i++)
                for (int d = 0; d < n; d++)
                {
                    yk[i][d] = (population[i].solution.result[d] - m_old[d]) / _sigma;
                    m_new[d] += _weights[i] * population[i].solution.result[d];
                }
            std::vector<double> yw(n);                         // <y>_w = Σ w_i y_i = (m_new-m_old)/σ
            for (int d = 0; d < n; d++) yw[d] = (m_new[d] - m_old[d]) / _sigma;

            // pσ = (1-cs)pσ + √(cs(2-cs)μeff)·C^{-1/2}·yw;  C^{-1/2}·yw = B·(1/D .* (Bᵀ·yw))
            std::vector<double> t(n, 0.0);
            for (int i = 0; i < n; i++) { double s = 0; for (int j = 0; j < n; j++) s += _B[j * n + i] * yw[j]; t[i] = s / _D[i]; }
            std::vector<double> Cinv_yw(n, 0.0);
            for (int i = 0; i < n; i++) { double s = 0; for (int j = 0; j < n; j++) s += _B[i * n + j] * t[j]; Cinv_yw[i] = s; }
            double coef_ps = std::sqrt(_cs * (2.0 - _cs) * _mueff);
            for (int d = 0; d < n; d++) _ps[d] = (1.0 - _cs) * _ps[d] + coef_ps * Cinv_yw[d];

            double ps_norm = 0; for (double x : _ps) ps_norm += x * x; ps_norm = std::sqrt(ps_norm);
            double hsig = (ps_norm / std::sqrt(1.0 - std::pow(1.0 - _cs, 2.0 * (_gen + 1))) / _chiN < 1.4 + 2.0 / (n + 1.0)) ? 1.0 : 0.0;

            // pc = (1-cc)pc + hsig·√(cc(2-cc)μeff)·yw
            double coef_pc = std::sqrt(_cc * (2.0 - _cc) * _mueff);
            for (int d = 0; d < n; d++) _pc[d] = (1.0 - _cc) * _pc[d] + hsig * coef_pc * yw[d];

            // C = (1-c1-cmu)C + c1(pc·pcᵀ + (1-hsig)cc(2-cc)C) + cmu·Σ w_i y_i y_iᵀ
            double delta_h = (1.0 - hsig) * _cc * (2.0 - _cc);
            for (int i = 0; i < n; i++)
                for (int j = 0; j < n; j++)
                {
                    double rank1 = _pc[i] * _pc[j] + delta_h * _C[i * n + j];
                    double rankmu = 0; for (int k = 0; k < _mu; k++) rankmu += _weights[k] * yk[k][i] * yk[k][j];
                    _C[i * n + j] = (1.0 - _c1 - _cmu) * _C[i * n + j] + _c1 * rank1 + _cmu * rankmu;
                }

            _sigma *= std::exp((_cs / _damps) * (ps_norm / _chiN - 1.0));   // CSA 步长
            _m = m_new;

            // 对称化(数值) + 特征分解 C → B, D
            for (int i = 0; i < n; i++)
                for (int j = i + 1; j < n; j++) { double avg = 0.5 * (_C[i * n + j] + _C[j * n + i]); _C[i * n + j] = _C[j * n + i] = avg; }
            std::vector<double> eig;
            jacobiEigen(_C, n, eig, _B);
            for (int i = 0; i < n; i++) _D[i] = std::sqrt(std::max(eig[i], 1e-20));   // 防数值负特征值
            _gen++;
        }

        void getNewIndividual(Individual* child, Individual*, Solution**, ProblemHandle*) override
        {
            int n = _n;
            std::vector<double> Dz(n);
            for (int d = 0; d < n; d++) Dz[d] = _D[d] * get_normal(0.0, 1.0);   // D .* z
            for (int i = 0; i < n; i++)   // x = m + σ·B·(D.*z)
            {
                double s = 0; for (int j = 0; j < n; j++) s += _B[i * n + j] * Dz[j];
                child->solution.result[i] = _m[i] + _sigma * s;
            }
        }
    };

    inline Registry<LearningStrategy>::Entry cmaesEntry()
    {
        return { "CMAES", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"sigma", ParamKind::Real, 0.0, 1e9, false, 0.3, 1.0} } }, sizeof(CMAES),
            [](const double* p) -> LearningStrategy* { return p ? new CMAES(p[0]) : new CMAES(); },
            [](AssertList& L, const double* p) { CMAES::preAssert(L, const_cast<double*>(p)); },
            [](AssertList&, const double*) {} };
    }
    ECFLOW_REGISTER(lstrat_cmaes, LearningStrategy, cmaesEntry());
}
