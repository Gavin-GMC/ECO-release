//------------------------Description------------------------
// 交叉算子基类 Crossover:以特定方式交换不同个体的片段生成新候选解。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include "evoflow-basicfunc.h"
#include "learning-strategy.h"

namespace EvoFlow
{
    class Crossover : public LearningStrategy
    {
    protected:
        double _cross_rate;
        bool new_pair;
        bool is_crossover;
        bool _coupled;

    public:
        Crossover(double cross_rate, bool coupled = true)
        {
            _cross_rate = cross_rate;
            _coupled = coupled;
            new_pair = true;
            is_crossover = false;
        }

        virtual ~Crossover() {}

        void preparation_s(IndividualArray& population, Terminator*) override   // ini
        {
            new_pair = true;
        }

        // 交叉前的预备操作
        virtual void preparation(Solution* s)
        {
            if (new_pair)
                is_crossover = _cross_rate > rand01_();
        }

        // 交叉后的收尾操作
        virtual void ending()
        {
            if (_coupled)
                new_pair = !new_pair;
        }
    };
}
