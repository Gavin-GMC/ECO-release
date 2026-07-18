//------------------------Description------------------------
// SetVelocityFeature:包裹每变量一个 SetVelocity(集合速度矩阵),供集合式 PSO(SetPSO)的 setvelocity 特性。
//   INDIV-COMPOSE S4:把 SetParticle 的集合速度从"子类成员"改为"可挂载特性"(kind "setvelocity")。
//-----------------------------------------------------------

#pragma once
#include <vector>
#include "feature.h"
#include "set-velocity.h"
#include "solution-decoder.h"
#include "ecflow-rand.h"
#include "registry.h"

namespace ECFlow
{
    struct SetVelocityFeature : Feature
    {
        std::vector<SetVelocity*> velocity;    // 每变量一个
        std::vector<int>          var_length;  // 前缀和(getVlength 用)

        ~SetVelocityFeature() override { for (auto* v : velocity) delete v; }

        int getVlength(int vid) const { return vid ? var_length[vid] - var_length[vid - 1] : var_length[0]; }

        bool inheritAtBirth() const override { return true; }   // 子代继承亲代速度矩阵(供 damp),走通用 inheritFeaturesFrom

        Feature* clone() const override { auto* f = new SetVelocityFeature(); f->copyFrom(*this); return f; }

        void copyFrom(const Feature& o) override            // 就地深拷(保持特性对象地址;继承/copy 都用它)
        {
            const auto& s = static_cast<const SetVelocityFeature&>(o);
            for (auto* v : velocity) delete v; velocity.clear();
            for (auto* v : s.velocity) velocity.push_back(v->copy());
            var_length = s.var_length;
        }

        void setProblem(const FeatureContext& c) override   // 从 prototype 的 decoder 逐变量建
        {
            SolutionDecoder* d = c.prototype ? c.prototype->decoder_pointer.get() : nullptr;
            if (!d) return;
            for (auto* v : velocity) delete v; velocity.clear(); var_length.clear();
            int vn = d->getVariableNumber();
            for (int i = 0; i < vn; i++)
            {
                velocity.push_back(new SetVelocity(d->getNote(i), d->getVariableSize(i)));
                var_length.push_back(d->getVariableSize(i));
            }
            for (int i = 1; i < vn; i++) var_length[i] += var_length[i - 1];
        }

        void ini(const FeatureContext&) override            // 随机播种(自 SetParticle::ini 的 ini_speciality)
        {
            for (auto* v : velocity)
            {
                v->clear();
                int cn = v->choice;
                for (int i = 0; i < v->demension; i++)
                    v->addToVelocity(i, ECFlow::get_int(0, cn - 1), 1);
                v->velocityIndexUpdate();
            }
        }
    };

    inline Registry<Feature>::Entry setVelocityFeatureEntry()
    {
        return { "setvelocity", ModuleType::T_feature, ParameterTemplate{}, sizeof(SetVelocityFeature),
            [](const double*) -> Feature* { return new SetVelocityFeature(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    ECFLOW_REGISTER(feat_setvelocity, Feature, setVelocityFeatureEntry());
}
