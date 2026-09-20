//------------------------Description------------------------
// 天鹰狩猎(Aquila Optimizer)学习策略 AquilaHunt:天鹰 4 种狩猎法按相位切换(前 2/3 探索、后 1/3 开发,相位内
//   rand 二选一)。向导来自 LeaderRandomCenter(end[0]=最优 X_best、end[1]=随机 X_R、end[2]=逐维种群均值 X_M)。

//-------------------------Reference-------------------------
// L. Abualigah, D. Yousri, M. Abd Elaziz, A. A. Ewees, M. A. Al-qaness, A. H. Gandomi, "Aquila
// Optimizer: A novel meta-heuristic optimization algorithm," Computers & Industrial Engineering,
// vol. 157, 107250, 2021, doi: 10.1016/j.cie.2021.107250.

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <cmath>
#include "evoflow-rand.h"
#include "evoflow-basicfunc.h"
#include "solution.h"
#include "individual.h"
#include "individual-array.h"
#include "problem-handle.h"
#include "learning-strategy.h"
#include "terminator.h"
#include "strategy-levy.h"
#include "registry.h"

namespace EvoFlow
{
    class AquilaHunt : public LearningStrategy
    {
    private:
        double _alpha, _delta;                 // 开发微调(标准各 0.1)
        double _r1, _U, _omega, _theta1;       // 螺旋常数(**罕调标准值**:r1≈10 搜索周期[1,20]、U=0.00565、ω=0.005、θ1=3π/2)
        double _progress, _t, _maxfes;              // 当代进度 + QF 严格所需 t/T(FES 轴)

    public:
        AquilaHunt(double alpha = 0.1, double delta = 0.1, double r1 = 10.0,
                   double U = 0.00565, double omega = 0.005, double theta1 = 4.71238898038469 /*3π/2*/)
            : _alpha(alpha), _delta(delta), _r1(r1), _U(U), _omega(omega), _theta1(theta1),
              _progress(0.0), _t(0.0), _maxfes(1e9) {}
        ~AquilaHunt() {}

        static void preAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 3, MatchType::notLessButNotice)); // 需 3:最优/随机/中心
        }
        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert));  // 逐维构造
        }

        void ini(ProblemHandle*) override { _progress = 0.0; _t = 0.0; _maxfes = 1e9; }

        void preparation_s(IndividualArray&, Terminator* terminator) override
        {
            _progress = terminator ? terminator->getProgress() : 0.0;
            _t = terminator ? (double)terminator->getFESTimes() : 0.0;
            _maxfes = (_progress > 1e-9) ? (_t / _progress) : 1e9;   // maxFES 重建(FES 轴)
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual*) override
        {
            double x = (*individual)[decision_d];
            if (learning_object == nullptr || learning_object[0] == nullptr) return x;  // 无最优 → 不动
            double Xb = (*learning_object[0])[decision_d];                   // 最优
            if (is_empty(Xb)) return x;
            if (is_empty(x)) x = Xb;
            double Xr = learning_object[1] ? (*learning_object[1])[decision_d] : Xb;  // 随机
            double Xm = learning_object[2] ? (*learning_object[2])[decision_d] : Xb;  // 逐维种群均值
            if (is_empty(Xr)) Xr = Xb;
            if (is_empty(Xm)) Xm = Xb;

            if (_progress <= 2.0 / 3.0)                                      // 探索
            {
                if (rand01() < 0.5)                                         // M1 扩展探索(高翔垂扑)
                    return Xb * (1.0 - _progress) + (Xm - Xb * rand01());
                else                                                       // M2 收窄探索(等高线滑翔 + 螺旋)
                {
                    double D1 = decision_d + 1;
                    double r = _r1 + _U * D1;
                    double theta = -_omega * D1 + _theta1;
                    double x_sp = r * std::sin(theta);
                    double y_sp = r * std::cos(theta);
                    return Xb * levyFlight() + Xr + (y_sp - x_sp) * rand01();
                }
            }
            else                                                            // 开发
            {
                if (rand01() < 0.5)                                         // M3 扩展开发(低飞缓降)
                {
                    double UB = problem_handle->getVariableUpbound(decision_d);
                    double LB = problem_handle->getVariableLowbound(decision_d);
                    return (Xb - Xm) * _alpha - rand01() + ((UB - LB) * rand01() + LB) * _delta;
                }
                else                                                       // M4 收窄开发(游走抓捕)
                {
                    double G1 = 2.0 * rand01() - 1.0;
                    double G2 = 2.0 * (1.0 - _progress);
                    // 严格原式 QF=t^((2r−1)/(1−T)²)(t/T 取 FES 轴)。退化守卫:maxFES 未设/极小致 _maxfes≈1(分母→0)
                    //   或 _t 无效(<1)时,QF 取其本意近 1 值,避免除零 / pow(0,负)→inf。真实运行 _maxfes 恒大 → 走严格式。
                    double denom = std::pow(1.0 - _maxfes, 2.0);
                    double QF = (_t >= 1.0 && denom > 1e-9) ? std::pow(_t, (2.0 * rand01() - 1.0) / denom) : 1.0;
                    return QF * Xb - (G1 * x * rand01()) - G2 * levyFlight() + rand01() * G1;
                }
            }
        }
    };

    inline Registry<LearningStrategy>::Entry aquilaHuntEntry()
    {
        return { "AquilaHunt", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"alpha",  ParamKind::Real, 0.0,  2.0, false, 0.05, 0.3},
                                 {"delta",  ParamKind::Real, 0.0,  2.0, false, 0.05, 0.3},
                                 {"r1",     ParamKind::Real, 1.0, 20.0, false, 5.0, 15.0},   // 罕调:搜索周期数
                                 {"U",      ParamKind::Real, 0.0,  1.0, false, 0.001, 0.01}, // 罕调:螺旋半径系数
                                 {"omega",  ParamKind::Real, 0.0,  1.0, false, 0.001, 0.01}, // 罕调:螺旋角速度
                                 {"theta1", ParamKind::Real, 0.0,  10.0, false, 4.0, 5.0} } }, sizeof(AquilaHunt), // 罕调:螺旋初相 3π/2
            [](const double* p) -> LearningStrategy* { return p ? new AquilaHunt(p[0], p[1], p[2], p[3], p[4], p[5]) : new AquilaHunt(); },
            [](AssertList& L, const double* p) { AquilaHunt::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { AquilaHunt::postAssert(L, const_cast<double*>(p)); } };
    }
    EVOFLOW_REGISTER(lstrat_aquila, LearningStrategy, aquilaHuntEntry());
}
