//------------------------Description------------------------
// 终止器 Terminator:优化器/种群迭代的停止条件。三条件(FES 评估次数、
// Convergence 停滞代数、Time 秒)任一达标即终止并输出结果。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <ctime>
#include <functional>
#include "evoflow-constant.h"

namespace EvoFlow
{
    class Terminator
    {
    private:
        int    _configured[3];  // FES, Convergence, Time —— 用户配置的原始上限
        int    _conditions[3];  // FES, Convergence, Time —— 有效上限(limited 可收紧,reset 恢复为 _configured)
        int    _states[3];      // FES, Convergence, Time —— 当前进度(**只记真实工作量**,见头注)
        time_t _start_time;
        std::function<bool()> _cancel_requested;

        void _refreshElapsedTime()
        {
            if (_conditions[2] != EVOFLOW_MAX)
                _states[2] = static_cast<int>(time(NULL) - _start_time);
        }

    public:
        Terminator()
        {
            for (int i = 0; i < 3; i++)
            {
                _configured[i] = EVOFLOW_MAX;
                _conditions[i] = EVOFLOW_MAX;
                _states[i]     = 0;      // 迁移适配:清零,避免 reset() 前误读
            }
            _start_time = time(NULL);
        }

        void setMaxFES(int fes)         { _configured[0] = _conditions[0] = fes; }
        void setMaxConvergence(int fes) { _configured[1] = _conditions[1] = fes; }
        void setMaxTime(int sec)        { _configured[2] = _conditions[2] = sec; }
        void setCancellationCheck(const std::function<bool()>& check)
        {
            _cancel_requested = check;
        }

        bool termination()
        {
            if (_cancel_requested)
            {
                try
                {
                    if (_cancel_requested()) return true;
                }
                catch (...) {}
            }
            // 时间是墙钟状态，不依赖评估发生；否则零评估 workflow 永远无法按 maxTime 退出。
            _refreshElapsedTime();
            for (int i = 0; i < 3; i++)
            {
                if (_states[i] >= _conditions[i])
                    return true;
            }
            return false;
        }

        void reset()
        {
            for (int i = 0; i < 3; i++)
            {
                _states[i]     = 0;
                _conditions[i] = _configured[i];   // v1.4.8:撤销上一轮 limited 的收紧,回到用户配置的上限
            }
            _start_time = time(NULL);
        }

        void update(bool best_update)
        {
            _states[0]++;

            if (best_update)
                _states[1] = 0;
            else
                _states[1]++;
        }

        int getFESTimes() { return _states[0]; }
        int configuredFES() { return _configured[0]; }   // v2.5.4.3:每轮配置 FES 上限(预分份额按此比例分)

        // 运行总进度(UPDATE-S-CONTEXT):三终止条件中"已启用"(condition != EVOFLOW_MAX)者的 state/condition 比例取最大值——
        // 三条件是"任一达标即终止"(OR),最先逼近终止的那个维度决定实际运行长度;全未启用则返回 0。
        double getProgress()
        {
            double progress = 0.0;
            for (int i = 0; i < 3; i++)
            {
                if (_conditions[i] == EVOFLOW_MAX) continue;   // 未启用,不参与
                double ratio = (double)_states[i] / (double)_conditions[i];
                if (ratio > progress) progress = ratio;
            }
            return progress;
        }

        // 将子终止器的运行进度同步至上级终止器
        void merge(Terminator* subterminator)
        {
            for (int i = 0; i < 3; i++)
            {
                _states[i] += subterminator->_states[i];
            }
        }

        // 确保子终止器的运行资源不超过上级终止器:有效上限 = min(自身配置上限, 上级剩余)。
        //   只动 _conditions、**绝不动 _states**(后者是 merge 要上报的真账,见头注的语义分界)。
        //   上级未启用某条件(EVOFLOW_MAX)时该维不约束——否则 min 会把 _conditions 从 EVOFLOW_MAX 压成
        //   `EVOFLOW_MAX - 上级已用`,使 getProgress 误判该条件"已启用"。
        void limited(Terminator* terminator)
        {
            for (int i = 0; i < 3; i++)
            {
                if (terminator->_conditions[i] == EVOFLOW_MAX) continue;   // 上级不设限 → 本维不约束

                long long left = static_cast<long long>(terminator->_conditions[i]) - terminator->_states[i];
                if (left < 0) left = 0;                                 // 上级已用超 → 不留余量(子群随即终止)

                _conditions[i] = (left < _configured[i]) ? static_cast<int>(left) : _configured[i];
            }
        }

        // 有效上限 = 上级剩余,**不与自身配置取 min**(与 limited 的唯一区别)。
        //   用于初始种群:它是**一次性成本、在 epoch 结构之外** —— 子群自身的 _configured 是"每轮(epoch)预算",
        //   拿它去限初始化是错的(epoch=30 而种群=400 时只会初始化 30 个,余下 370 个**未初始化个体仍会参与后续迭代**)。
        //   初始种群只应受**总预算**约束,故此处忽略自身配置、直接采上级剩余;上级不设限则本维不约束。
        void cappedBy(Terminator* terminator)
        {
            for (int i = 0; i < 3; i++)
            {
                if (terminator->_conditions[i] == EVOFLOW_MAX)             // 上级不设限 → 本维不约束
                {
                    _conditions[i] = EVOFLOW_MAX;
                    continue;
                }

                long long left = static_cast<long long>(terminator->_conditions[i]) - terminator->_states[i];
                _conditions[i] = (left > 0) ? static_cast<int>(left) : 0;
            }
        }

        // v2.5.4.3 A 预分配:把本轮 FES 有效上限置为**预分份额**(由 manager 按各子群配置比例算好),重置进度。
        //   与串行 limited 的区别:FES 维用预分份额(消"读全局实时剩余"的前后依赖 → 可并行);
        //   非 FES 维(Convergence 停滞 / Time)仍沿用 limited 语义 = min(自身配置, 全局剩余)(它们非消耗型共享预算,不预分)。
        //   `fesShare` 已 ≤ 自身配置(manager 保证),此处再 clamp 保险。
        void allocateEpochFES(Terminator* global, int fesShare)
        {
            _states[0] = _states[1] = _states[2] = 0;   // 每轮进度归零(同 reset 后再 limited 的效果)
            _conditions[0] = (fesShare < _configured[0]) ? fesShare : _configured[0];
            for (int i = 1; i < 3; i++)   // Convergence, Time:沿用 limited 语义
            {
                if (global->_conditions[i] == EVOFLOW_MAX) { _conditions[i] = _configured[i]; continue; }
                long long left = static_cast<long long>(global->_conditions[i]) - global->_states[i];
                if (left < 0) left = 0;
                _conditions[i] = (left < _configured[i]) ? static_cast<int>(left) : _configured[i];
            }
        }
    };
}
