//------------------------Description------------------------
// 集合速度 SetVelocity(每维 × 每候选 的速度矩阵 + 稀疏索引缓存):用于集合式 PSO(SetPSO)的离散/组合速度。
//-------------------------Reference-------------------------
// 抽自 individual-setparticle.h(原为 SetParticle 的内嵌类)。INDIV-COMPOSE S4:提为独立类,供 SetVelocityFeature 包裹,
//   与个体子类解耦(S5 删 SetParticle 后仍在)。零逻辑改,仅去嵌套。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
//-----------------------------------------------------------

#pragma once
#include <vector>
#include <cstring>
#include <string>
#include "solution-decoder.h"   // ElementNote / VariableType
#include "ecflow-rand.h"          // ECFlow::get_int(消费方 ini 用)

namespace ECFlow
{
    // 集合速度:每维 × 每候选 的速度矩阵(稀疏索引缓存)
    class SetVelocity
    {
    private:
        static constexpr double threshold = 1e-3;
        double _low_bound;
        double _accuracy;

        SetVelocity()   // 仅供 copy() 使用
        {
            name = "";
            pre_base = false;
            bidiagraph = false;
            demension = 0;
            choice = 0;
            velocity = nullptr;
            velocityIndex = nullptr;
            velocityIndex_Size = nullptr;
            _low_bound = 0;
            _accuracy = 0;
        }

    public:
        std::string name;
        int         demension;
        int         choice;
        double*     velocity;
        int*        velocityIndex;
        int*        velocityIndex_Size;

        bool pre_base;
        bool bidiagraph;

        SetVelocity(const ElementNote& note, int variable_size)
        {
            switch (note._type)
            {
            case VariableType::discrete:
            case VariableType::allocation:
            {
                name = note._name;
                pre_base = false;
                bidiagraph = false;
                demension = variable_size;
                choice = (note._upbound - note._lowbound) / note._accuracy + 1;
                int size = demension * choice;
                velocity = new double[size];
                velocityIndex = new int[size];
                velocityIndex_Size = new int[demension]();   // 零初始化(修复:原未清零,damping 先于 velocityIndexUpdate 时读 garbage → OOB,SetPSO 暴露)
                for (int j = 0; j < size; j++) velocity[j] = 0;
                _low_bound = note._lowbound;
                _accuracy = note._accuracy;
                break;
            }
            case VariableType::sequence_direction:
            case VariableType::sub_module:
            {
                name = note._name;
                pre_base = true;
                bidiagraph = false;
                demension = (note._upbound - note._lowbound) / note._accuracy + 1;
                choice = (note._upbound - note._lowbound) / note._accuracy + 1;
                int size = demension * choice;
                velocity = new double[size];
                velocityIndex = new int[size];
                velocityIndex_Size = new int[demension]();   // 零初始化(见上)
                for (int j = 0; j < size; j++) velocity[j] = 0;
                _low_bound = note._lowbound;
                _accuracy = note._accuracy;
                break;
            }
            case VariableType::sequence_bidiagraph:
            {
                name = note._name;
                pre_base = true;
                bidiagraph = true;
                demension = (note._upbound - note._lowbound) / note._accuracy + 1;
                choice = (note._upbound - note._lowbound) / note._accuracy + 1;
                int size = demension * choice;
                velocity = new double[size];
                velocityIndex = new int[size];
                velocityIndex_Size = new int[demension]();   // 零初始化(见上)
                for (int j = 0; j < size; j++) velocity[j] = 0;
                _low_bound = note._lowbound;
                _accuracy = note._accuracy;
                break;
            }
            default:
                name = "";
                pre_base = false;
                bidiagraph = false;
                demension = 0;
                choice = 0;
                velocity = nullptr;
                velocityIndex = nullptr;
                velocityIndex_Size = nullptr;
                _low_bound = 0;
                _accuracy = 0;
                break;
            }
        }

        ~SetVelocity()
        {
            delete[] velocity;
            delete[] velocityIndex;
            delete[] velocityIndex_Size;
        }

        int    choice2Cid(double choice_) { return int((choice_ - _low_bound) / _accuracy); }
        double cid2Choice(int cid)         { return _low_bound + _accuracy * cid; }

        int trans2Did(int demension_, double prechoice)
        {
            if (prechoice) return choice2Cid(prechoice);
            else           return demension_;
        }

        void damping(double w)
        {
            for (int i = 0; i < demension; i++)
                for (int j = 0; j < velocityIndex_Size[i]; j++)
                    velocity[i * choice + velocityIndex[i * choice + j]] *= w;
        }

        void addToVelocity(int demensionId, int choiceid, double rate)
        {
            if (demensionId >= demension || rate <= velocity[demensionId * choice + choiceid] || choiceid >= choice)
                return;
            if (rate > 1) rate = 1;

            velocity[demensionId * choice + choiceid] = rate;
            if (bidiagraph)
                velocity[choiceid * choice + demensionId] = rate;
        }

        void velocityIndexUpdate()
        {
            int offset;
            for (int i = 0; i < demension; i++)
            {
                velocityIndex_Size[i] = 0;
                offset = i * choice;
                for (int j = 0; j < choice; j++)
                {
                    if (velocity[offset + j] > threshold)
                    {
                        velocityIndex[offset + velocityIndex_Size[i]] = j;
                        velocityIndex_Size[i]++;
                    }
                }
            }
        }

        double getVelocityRate(int demensionId, int choiceId) { return velocity[demensionId * choice + choiceId]; }

        void clear()
        {
            int length = demension * choice;
            for (int i = 0; i < length; i++) velocity[i] = 0;
        }

        SetVelocity* copy()
        {
            SetVelocity* back = new SetVelocity();
            back->_low_bound = _low_bound;
            back->_accuracy = _accuracy;
            back->name = name;
            back->demension = demension;
            back->choice = choice;
            back->velocity = new double[demension * choice];
            back->velocityIndex = new int[demension * choice];
            back->velocityIndex_Size = new int[demension]();   // 零初始化(见上)
            memcpy(back->velocity, velocity, demension * choice * sizeof(double));
            memcpy(back->velocityIndex, velocityIndex, demension * choice * sizeof(int));
            memcpy(back->velocityIndex_Size, velocityIndex_Size, demension * sizeof(int));
            back->pre_base = pre_base;
            back->bidiagraph = bidiagraph;
            return back;
        }
    };
}
