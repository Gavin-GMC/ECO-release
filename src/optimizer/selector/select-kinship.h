//------------------------Description------------------------
// 血缘定向环境选择 KinshipSelector(第一层):onlooker 多对一场景——offspring[j] 源自哪个源由血缘 id 决定,
//   读 offspring[j].kinship.parent_id 定位到源槽,交第二层接受准则(通常 AgingAccept)逐源比较/替换。
//-------------------------Reference-------------------------
// 新增(ABC 重做,1.4.4.5)。KINSHIP-ID 消费方:与 Index(位置 1:1)对偶,用于 Roulette 有放回选源的多对一归属。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "selector.h"
#include "registry.h"

namespace ECFlow
{
    // 血缘定向:offspring[j] 与其源亲本(parent[parent_id])比较。第二层接受准则决定是否替换 + 计龄。
    class KinshipSelector final : public EnvirSelect
    {
        std::string _kin_key;   // kinship 特性身份键(Singular → 恒 "kinship")
    public:
        KinshipSelector(AcceptCriterion* accept) : EnvirSelect(accept) {}
        ~KinshipSelector() {}

        std::vector<FeatureDemand> featureDemands() const override
        {
            std::vector<FeatureDemand> d = _accept ? _accept->featureDemands() : std::vector<FeatureDemand>{};   // 转发接受层(age)
            d.push_back({ "kinship", "kinship", FeatureScope::Singular, {} });                                   // 自身声明血缘
            return d;
        }
        void setFeatureKey(const std::string& role, const std::string& key) override
        {
            if (role == "kinship") _kin_key = key;
            else if (_accept)      _accept->setFeatureKey(role, key);   // age → 接受层
        }

        void update_subswarm(IndividualArray& parent, IndividualArray& offspring, Terminator* terminator, BestArchive* archive) override
        {
            for (int j = 0; j < offspring.getSize(); j++)
            {
                long k = offspring[j].feature<KinshipFeature>(_kin_key)->getParentId();   // 源槽 = 出生派生的 parent_id
                if (k < 0 || k >= parent.getSize()) continue;                             // 越界护栏(无源者跳过)
                _accept->accept(offspring[j], parent[(int)k], terminator, archive);       // 逐源比较/替换 + 计龄
            }
            _accept->update();
        }
    };

    inline Registry<EnvirSelect>::Entry kinshipSelectorEntry()
    {
        return { "Kinship", ModuleType::T_selector,
            ParameterTemplate{ { {"accept_type", ParamKind::Enum, 0, 3, false, 3, 3} },   // 0 无条件/1 择优/2 退火/3 年龄计龄(ABC 默认)
                               acceptParaTemplate },                                       // next:同 Index
            sizeof(KinshipSelector),
            [](const double* p) -> EnvirSelect* { return new KinshipSelector(makeAccept(p)); },
            [](AssertList&, const double*) {}, [](AssertList& L, const double* p) { acceptPostAssert(p, L); } };   // 追加接受准则契约
    }
    ECFLOW_REGISTER(sel_kinship, EnvirSelect, kinshipSelectorEntry());
}
