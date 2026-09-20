//------------------------Description------------------------
// 集合速度 SetVelocity(每维 × 每候选 的速度矩阵 + 稀疏索引缓存):用于集合式 PSO(SetPSO)的离散/组合速度。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <cmath>
#include <vector>
#include <cstring>
#include <string>
#include "solution-decoder.h"   // ElementNote / VariableType
#include "evoflow-rand.h"          // EvoFlow::get_int(消费方 ini 用)

namespace EvoFlow
{
    // 集合速度:每维 × 每候选 的速度矩阵(稀疏索引缓存)
    class SetVelocity
    {
    private:
        static constexpr double threshold = 1e-3;
        double _low_bound;
        double _accuracy;

        bool validIndex(int demensionId, int choiceId) const
        {
            return velocity && demensionId >= 0 && demensionId < demension &&
                   choiceId >= 0 && choiceId < choice;
        }

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

        bool tryChoice2Cid(double choice_, int& cid) const
        {
            cid = -1;
            if (!std::isfinite(choice_) || !std::isfinite(_low_bound) ||
                !std::isfinite(_accuracy) || _accuracy <= 0.0 || choice <= 0)
                return false;

            const double high = _low_bound + _accuracy * (choice - 1);
            if (!std::isfinite(high) || choice_ < _low_bound || choice_ > high)
                return false;

            const double scaled = (choice_ - _low_bound) / _accuracy;
            if (!std::isfinite(scaled) || scaled < 0.0 || scaled > static_cast<double>(choice - 1))
                return false;

            cid = static_cast<int>(scaled);
            return cid >= 0 && cid < choice;
        }

        int choice2Cid(double choice_)
        {
            int cid;
            return tryChoice2Cid(choice_, cid) ? cid : -1;
        }
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
            if (!validIndex(demensionId, choiceid) || !std::isfinite(rate))
                return;
            if (rate <= velocity[demensionId * choice + choiceid]) return;
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

        double getVelocityRate(int demensionId, int choiceId)
        {
            return validIndex(demensionId, choiceId) ? velocity[demensionId * choice + choiceId] : 0.0;
        }

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
