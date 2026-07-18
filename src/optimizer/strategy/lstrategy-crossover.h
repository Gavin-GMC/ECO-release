//------------------------Description------------------------
// 交叉算子基类 Crossover:以特定方式交换不同个体的片段生成新候选解。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "ecflow-basicfunc.h"
#include "learning-strategy.h"

namespace ECFlow
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
