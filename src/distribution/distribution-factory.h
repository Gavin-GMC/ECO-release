//------------------------Description------------------------
// 分布模型工厂:按 DistributionType 构造 DistributionModel 对象(数组)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "distribution-model.h"
#include "distribution-type.h"

namespace ECFlow
{
    class DistributionFactory
    {
    public:
        static DistributionModel** newModelArray(DistributionType type, int length = 1)
        {
            DistributionModel** back = new DistributionModel * [length];
            switch (type)
            {
            case DistributionType::F_Gaussian:
                for (int i = 0; i < length; i++)
                    back[i] = new DM_Gaussian();
                return back;
            case DistributionType::F_Cauchy:
                for (int i = 0; i < length; i++)
                    back[i] = new DM_Cauchy();
                return back;
            case DistributionType::F_Uniform:
                for (int i = 0; i < length; i++)
                    back[i] = new DM_Uniform();
                return back;
            case DistributionType::F_Histogram:
                for (int i = 0; i < length; i++)
                    back[i] = new DM_Histogram();
                return back;
            default:
                delete[] back;
                return nullptr;
            }
        }

        static size_t typeSize(DistributionType type)
        {
            switch (type)
            {
            case DistributionType::F_Gaussian:  return sizeof(DM_Gaussian);
            case DistributionType::F_Cauchy:    return sizeof(DM_Cauchy);
            case DistributionType::F_Uniform:   return sizeof(DM_Uniform);
            case DistributionType::F_Histogram: return sizeof(DM_Histogram);
            default:                            return 0;
            }
        }
    };
}
