//------------------------Description------------------------
// NeighborSet:一个子种群的"邻居子种群集合"(容量-大小分离,裸 Subpopulation** 数组,仅持指针不拥有)。
//   由子群拓扑(SubpopulationTopology)填充,再经 Subpopulation::setNeibors 注入到各子群的邻居 buffer。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "subpopulation.h"

namespace ECFlow
{
    struct NeighborSet
    {
    private:
        int             neighbor_number;
        Subpopulation** neighborhood;   // 仅持指针,不拥有邻居子种群

    public:
        NeighborSet()
        {
            neighbor_number = -1;
            neighborhood = nullptr;
        }

        ~NeighborSet()
        {
            delete[] neighborhood;
        }

        void setSize(int size)
        {
            delete[] neighborhood;          // 对 nullptr 安全(迁移修复:去原双重释放隐患)
            neighborhood = nullptr;
            neighbor_number = size;
            if (size > 0)
                neighborhood = new Subpopulation * [size];
        }

        int getSize()
        {
            return neighbor_number;
        }

        Subpopulation** getNSet()
        {
            return neighborhood;
        }

        Subpopulation*& operator[](int i)
        {
            return neighborhood[i];
        }
    };
}
