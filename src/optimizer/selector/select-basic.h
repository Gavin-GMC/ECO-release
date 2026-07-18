//------------------------Description------------------------
// 常用环境选择算子:IndexUpdater(按索引替换)/ RankUpdater(择优合并)/ CloseUpdater(替换最相似)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "selector.h"
#include "ecflow-math.h"
#include "registry.h"

namespace ECFlow
{
    // 按索引替换:对齐下标,逐位替换(范围取两群体较小者)
    class IndexUpdater final : public EnvirSelect
    {
    public:
        IndexUpdater(AcceptCriterion* accept) : EnvirSelect(accept) {}
        ~IndexUpdater() {}

        void update_subswarm(IndividualArray& parent, IndividualArray& offspring, Terminator* terminator, BestArchive* archive) override
        {
            int population_size = parent.getSize();
            if (offspring.getSize() < population_size)
                population_size = offspring.getSize();

            for (int i = 0; i < population_size; i++)
                _accept->accept(offspring[i], parent[i], terminator, archive);
            _accept->update();   // 每代推进接受准则(退火降温;无状态准则空操作)
        }
    };

    // 择优合并:亲/子各自排序后归并出较优的 parent.size 个(需完备偏序比较器)
    class RankUpdater final : public EnvirSelect
    {
    public:
        RankUpdater() : EnvirSelect(new UnconditionalAccept()) {}   // 择优归并内建全局择优,不走接受层(占位闲置)
        ~RankUpdater() {}

        void update_subswarm(IndividualArray& parent, IndividualArray& offspring, Terminator* terminator, BestArchive* archive) override
        {
            parent.sort();
            offspring.sort();

            // 先统计新群体构成比例(比逐个插入省大量插入操作)
            int parent_number = 0;
            int offspring_number = 0;
            int counter = 0;
            while (counter < parent.getSize())
            {
                if (offspring[offspring_number] < parent[parent_number])
                    offspring_number++;
                else
                    parent_number++;
                counter++;
            }

            // 合并新种群
            offspring_number--;
            parent_number--;
            counter--;
            while (parent_number != -1 && offspring_number != -1)
            {
                if (offspring[offspring_number] < parent[parent_number])
                {
                    parent[counter].swap(offspring[offspring_number]);
                    offspring_number--;
                    counter--;
                }
                else
                {
                    parent[counter].swap(parent[parent_number]);
                    parent_number--;
                    counter--;
                }
            }
            while (offspring_number != -1)
            {
                parent[counter].swap(offspring[offspring_number]);
                offspring_number--;
                counter--;
            }
        }
    };

    // 替换最相似:每个子代替换与之欧氏距离最近的父代
    class CloseUpdater final : public EnvirSelect
    {
    public:
        CloseUpdater(AcceptCriterion* accept) : EnvirSelect(accept) {}
        ~CloseUpdater() {}

        void update_subswarm(IndividualArray& parent, IndividualArray& offspring, Terminator* terminator, BestArchive* archive) override
        {
            int population_size = offspring.getSize();

            double distance_min;
            double distance_current;
            int distance_min_id;
            for (int offspring_id = 0; offspring_id < population_size; offspring_id++)
            {
                distance_min = eu_distance(offspring[offspring_id].solution.result, parent[0].solution.result);
                distance_min_id = 0;

                for (int parent_id = 1; parent_id < parent.getSize(); parent_id++)
                {
                    distance_current = eu_distance(offspring[offspring_id].solution.result, parent[parent_id].solution.result);
                    if (distance_current < distance_min)
                    {
                        distance_min = distance_current;
                        distance_min_id = parent_id;
                    }
                }

                _accept->accept(offspring[offspring_id], parent[distance_min_id], terminator, archive);
            }
            _accept->update();   // 每代推进接受准则(退火降温;无状态准则空操作)
        }
    };

    // 自注册进 Registry<EnvirSelect>(T_selector)。better_replace: para?para[0]!=0:false(缺省无条件替换)。
    inline Registry<EnvirSelect>::Entry indexSelectorEntry()
    {
        // 参数 = 自身 `{accept_type}` ++ **接受准则的参数**(经 tail → acceptParaTemplate,与 postAssert 的追加式对仗)。
        return { "Index", ModuleType::T_selector,
            ParameterTemplate{ { {"accept_type", ParamKind::Enum, 0, 3, false, 0, 1} },   // 0 无条件/1 择优/2 退火/3 年龄计龄
                               acceptParaTemplate },                                       // next:追加接受准则自己的参数
            sizeof(IndexUpdater),
            [](const double* p) -> EnvirSelect* { return new IndexUpdater(makeAccept(p)); },
            [](AssertList&, const double*) {}, [](AssertList& L, const double* p) { acceptPostAssert(p, L); } };   // 追加接受准则契约
    }
    inline Registry<EnvirSelect>::Entry rankSelectorEntry()
    {
        return { "Rank", ModuleType::T_selector, ParameterTemplate{}, sizeof(RankUpdater),
            [](const double*) -> EnvirSelect* { return new RankUpdater(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    inline Registry<EnvirSelect>::Entry closeSelectorEntry()
    {
        return { "Close", ModuleType::T_selector,
            ParameterTemplate{ { {"accept_type", ParamKind::Enum, 0, 3, false, 0, 1} },
                               acceptParaTemplate },                                       // next:同 Index
            sizeof(CloseUpdater),
            [](const double* p) -> EnvirSelect* { return new CloseUpdater(makeAccept(p)); },
            [](AssertList&, const double*) {}, [](AssertList& L, const double* p) { acceptPostAssert(p, L); } };   // 追加接受准则契约
    }
    ECFLOW_REGISTER(sel_index, EnvirSelect, indexSelectorEntry());
    ECFLOW_REGISTER(sel_rank,  EnvirSelect, rankSelectorEntry());
    ECFLOW_REGISTER(sel_close, EnvirSelect, closeSelectorEntry());
}
