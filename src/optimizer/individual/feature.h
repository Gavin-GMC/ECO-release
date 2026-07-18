//------------------------Description------------------------
// 个体特性组件 Feature(INDIV-COMPOSE,收尾 #2):个体的逐个体状态由"逐子类继承"改为
// "特性组件挂载"。Feature = 一件可挂到 Individual 特性袋里的状态载体(值语义,自带深拷贝/定尺/初始化)。
//-------------------------Reference-------------------------
// 新增。取代 individual-{pbest,particle,step,whale,firework}.h 的子类特性成员。
//-----------------------------------------------------------

#pragma once
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include "solution.h"
#include "module-type.h"
#include "parameter-template.h"
#include "ecflow-assert.h"
#include "registry.h"
#include "comparer.hpp"        // PbestFeature::afterEvaluate 用 Comparer::isBetter(单向依赖,无环)
#include "problem-handle.h"    // RandomInDomain 初始化用 handle 取域边界(getVariableLow/Upbound)
#include "ecflow-rand.h"         // RandomInDomain 均匀采样

namespace ECFlow
{

    // 特性作用域(决定装配期身份键的解析方式,见 docs/个体重构方案.md §4)。
    enum class FeatureScope {
        Private,    // 默认:每组件实例私有(可经通道 opt-in 共享)
        Exclusive,  // 每实例私有,禁共享(配通道→装配期报错)
        Singular    // 每个体唯一,所有读者共用
    };

    // 特性初始化策略(由声明该特性的组件经参数调控;共享时装配期校验一致)。
    //   Zero          → 全 0(velocity 默认)
    //   Constant      → 全 init_value(sigma=σ0)
    //   RandomInDomain→ 逐维 U(-k·(hi-lo), k·(hi-lo)) 对称随机,k=init_value(velocity 随机初始化;速度无需可行,故直接算域边界)
    enum class FeatureInit { Zero, Constant, RandomInDomain };

    // 组件对个体特性的需求声明(驱动装配期推断个体特性集)。
    struct FeatureDemand {
        std::string         role;                        // 语义名(velocity/pbest/sigma...)
        std::string         kind;                        // Registry<Feature> 种类 tag(vector/scalar/solution...)
        FeatureScope        scope       = FeatureScope::Private;
        std::vector<double> params;                      // 造特性参数(如 scalar 的 k)
        FeatureInit         init        = FeatureInit::Zero;   // 初始化策略(组件调控)
        double              init_value  = 0.0;                 // Constant→常量;RandomInDomain→幅度系数 k
    };

    // 装配期推断出的一个特性(登记进个体原型描述符)。
    struct FeatureSpec {
        std::string         key;                          // 袋内身份键
        std::string         kind;
        std::vector<double> params;
        FeatureInit         init        = FeatureInit::Zero;
        double              init_value  = 0.0;
    };

    // 定尺上下文:装配期给特性提供"依问题定尺"所需信息。
    struct FeatureContext
    {
        int             solution_size = 0;       // 解维数(向量特性按此定尺)
        const Solution* prototype     = nullptr; // 参考解(SolutionFeature 取规模+解码器)
        Comparer*       comparer      = nullptr; // 需比较的特性用(如 pbest)
        ProblemHandle*  handle        = nullptr; // 透传个体初始化所用句柄(RandomInDomain 取域边界;不在个体自持)
    };

    // 特性基类:深拷贝 / 依问题定尺 / 特性初始化。
    struct Feature
    {
        virtual ~Feature() = default;
        virtual Feature* clone() const = 0;                        // 深拷贝新建(供首次建/兜底)
        virtual void copyFrom(const Feature& other) = 0;           // **就地**复制内容(保持对象地址不变)
        virtual void setProblem(const FeatureContext&) {}          // 依问题定尺
        virtual void ini(const FeatureContext&) {}                 // 特性初始化(取代旧 ini_speciality 分支)
        // 由声明该特性的组件设定初始化策略(装配期戳入;默认空操作,数值类特性覆写)。
        virtual void setInitPolicy(FeatureInit /*mode*/, double /*value*/) {}
        // 生成子代时是否随粒子"记忆前传"(默认否;pbest 类特性覆写为真)。
        virtual bool inheritAtBirth() const { return false; }
        // 出生时从源亲本(起点)取本特性的"派生"语义。默认 = 就地拷贝(pbest/setvelocity:子代继承亲代记忆);
        //   血缘类特性覆写为"派生"(child.parent_id = parent.id,而非照抄)。由 inheritFeaturesFrom 在 inheritAtBirth 特性上调用。
        virtual void birthFrom(const Feature& parent) { copyFrom(parent); }
        // 个体评估完成后的自更新钩子(默认空操作;pbest 类特性覆写为择优保留)。
        virtual void afterEvaluate(const Solution& /*evaluated*/, Comparer* /*cmp*/) {}
    };

    // 按解维定尺的实数向量:velocity / sigma。初始化策略由声明组件经 setInitPolicy 戳入(个体初始化时执行)。
    struct VectorFeature : Feature
    {
        std::vector<double> data;
        FeatureInit _mode  = FeatureInit::Zero;   // 初始化策略(组件调控)
        double      _value = 0.0;                 // Constant→常量;RandomInDomain→幅度系数 k

        Feature* clone() const override { return new VectorFeature(*this); }   // 拷贝 _mode/_value/data
        void copyFrom(const Feature& o) override { data = static_cast<const VectorFeature&>(o).data; }
        void setInitPolicy(FeatureInit mode, double value) override { _mode = mode; _value = value; }
        void setProblem(const FeatureContext& c) override { data.assign(c.solution_size, 0.0); }   // 仅定尺(值由 ini 按策略填)
        void ini(const FeatureContext& c) override { _fill(c); }

    private:
        void _fill(const FeatureContext& c)
        {
            switch (_mode)
            {
            case FeatureInit::Constant:
                std::fill(data.begin(), data.end(), _value);
                break;
            case FeatureInit::RandomInDomain:                      // 逐维 U(-k(hi-lo), k(hi-lo)) 对称;速度无需可行→直接算域边界
                for (int d = 0; d < (int)data.size(); ++d)
                {
                    double span = c.handle ? (c.handle->getVariableUpbound(d) - c.handle->getVariableLowbound(d)) : 0.0;
                    data[d] = (2.0 * rand01_() - 1.0) * _value * span;
                }
                break;
            case FeatureInit::Zero:
            default:
                std::fill(data.begin(), data.end(), 0.0);
                break;
            }
        }
    };

    // k 个标量:whalestate(4) / fireworkstate(1)。
    struct ScalarFeature : Feature
    {
        std::vector<double> vals;
        explicit ScalarFeature(int k = 1) : vals(k, 0.0) {}
        Feature* clone() const override { return new ScalarFeature(*this); }
        void copyFrom(const Feature& o) override { vals = static_cast<const ScalarFeature&>(o).vals; }
        void ini(const FeatureContext&) override { std::fill(vals.begin(), vals.end(), 0.0); }
    };

    // 一个 Solution:pbest(初始化 = 复制当前解,忠实 PbestIndividual::ini)。
    struct SolutionFeature : Feature
    {
        Solution sol;
        Feature* clone() const override { auto* f = new SolutionFeature(); f->sol.copy(sol); return f; }
        void copyFrom(const Feature& o) override { sol.copy(static_cast<const SolutionFeature&>(o).sol); }
        void setProblem(const FeatureContext& c) override
        {
            if (c.prototype) { sol.setSize(*c.prototype); sol.setDecoder(*c.prototype); }
        }
        void ini(const FeatureContext& c) override { if (c.prototype) sol.copy(*c.prototype); }
    };

    // 个体历史最优 pbest:复用 SolutionFeature 的 Solution 存储/定尺/初始化,附加 pbest 专有语义——
    //   生成时随粒子记忆前传(inheritAtBirth)、评估后择优保留(afterEvaluate)。忠实 PbestIndividual 语义。
    struct PbestFeature : SolutionFeature
    {
        Feature* clone() const override { auto* f = new PbestFeature(); f->sol.copy(sol); return f; }
        bool inheritAtBirth() const override { return true; }
        void afterEvaluate(const Solution& s, Comparer* cmp) override
        {
            if (cmp && cmp->isBetter(s.fitness, sol.fitness)) sol.copy(s);   // 更优则更新 pbest
        }
    };

    // 个体年龄/停滞计数(整数):记该个体(槽)连续未改进的代数(= ABC 的 trial)。由消费者(AgingAccept)驱动:
    //   改进/被接受→reset(0);未改进→ageing(++);超限→消费者触发 scout(individual.ini() 重生,ini 里本特性归零)。
    struct AgeFeature : Feature
    {
        int age = 0;

        // 对外接口(消费者用)
        int  get() const { return age; }   // 读当前年龄/停滞代数
        void reset()     { age = 0; }      // 改进/被接受 → 归零
        void ageing()    { age++; }        // 未改进 → 自增

        // Feature 基类契约(inheritAtBirth 默认 false=子代不继承年龄;afterEvaluate/setProblem 不覆写)
        Feature* clone() const override { auto* f = new AgeFeature(); f->age = age; return f; }
        void copyFrom(const Feature& o) override { age = static_cast<const AgeFeature&>(o).age; }
        void ini(const FeatureContext&) override { age = 0; }   // 新个体/scout 重生 → 归零
    };

    // 解血缘 id(KINSHIP-ID):使"某子代候选源自哪个亲本"可一步判定(直系亲本标记,等值判定)。
    //   `id` = 本解的血缘 id(朴素值,由声明该特性的组件按语义盖:槽序号 / 精英编号 / 自增…,特性本身不定策略);
    //   `parent_id` = 出生时从源亲本(生成流程的起点 startPoint)派生 = parent.id;判定 isChildOf(p) = (parent_id == p.id)。
    //   scope 既定为 Singular(个体级血缘,生产方〔盖源 id〕与消费方〔读 parent_id 归属〕须共用同一份,同 pbest);
    //   inheritAtBirth=true 但走"派生"而非"拷贝"(birthFrom 覆写),故 id 随基因走(swap 交换特性袋),对重排/子集稳健。
    //   用例(消费方另立增量,本轮不建):ABC onlooker 逐源贪心、精英解局搜邻居归属。
    struct KinshipFeature : Feature
    {
        long id        = 0;    // 本解血缘 id(朴素值,消费方盖)
        long parent_id = -1;   // 源自哪个亲本的 id(-1 = 初代/无源)

        // 对外接口(消费方用)
        long getId()       const { return id; }
        long getParentId() const { return parent_id; }
        void setId(long v)       { id = v; }
        bool isChildOf(const KinshipFeature& p) const { return parent_id == p.id; }   // 一步归属判定

        // Feature 基类契约
        Feature* clone() const override { auto* f = new KinshipFeature(); f->id = id; f->parent_id = parent_id; return f; }
        void copyFrom(const Feature& o) override { const auto& k = static_cast<const KinshipFeature&>(o); id = k.id; parent_id = k.parent_id; }
        void ini(const FeatureContext&) override { id = 0; parent_id = -1; }   // 新个体 → 血缘复位
        bool inheritAtBirth() const override { return true; }                  // 出生时触发 birthFrom(经 inheritFeaturesFrom)
        // 出生"派生":记源亲本 id(非照抄);本体 id 保留(消费方按需另盖,单代归属只需 parent_id)。
        void birthFrom(const Feature& parent) override { parent_id = static_cast<const KinshipFeature&>(parent).id; }
    };

    // ---- 自注册进 Registry<Feature>(T_feature),tag = 通用种类名 ----
    inline Registry<Feature>::Entry vectorFeatureEntry()
    {
        return { "vector", ModuleType::T_feature, ParameterTemplate{}, sizeof(VectorFeature),
            [](const double*) -> Feature* { return new VectorFeature(); },   // 初始化策略经 setInitPolicy 戳入,非构造参数
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    ECFLOW_REGISTER(feat_vector, Feature, vectorFeatureEntry());

    inline Registry<Feature>::Entry scalarFeatureEntry()
    {
        return { "scalar", ModuleType::T_feature, ParameterTemplate{}, sizeof(ScalarFeature),
            [](const double* p) -> Feature* { return new ScalarFeature(p ? (int)p[0] : 1); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    ECFLOW_REGISTER(feat_scalar, Feature, scalarFeatureEntry());

    inline Registry<Feature>::Entry solutionFeatureEntry()
    {
        return { "solution", ModuleType::T_feature, ParameterTemplate{}, sizeof(SolutionFeature),
            [](const double*) -> Feature* { return new SolutionFeature(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    ECFLOW_REGISTER(feat_solution, Feature, solutionFeatureEntry());

    inline Registry<Feature>::Entry pbestFeatureEntry()
    {
        return { "pbest", ModuleType::T_feature, ParameterTemplate{}, sizeof(PbestFeature),
            [](const double*) -> Feature* { return new PbestFeature(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    ECFLOW_REGISTER(feat_pbest, Feature, pbestFeatureEntry());

    inline Registry<Feature>::Entry ageFeatureEntry()
    {
        return { "age", ModuleType::T_feature, ParameterTemplate{}, sizeof(AgeFeature),
            [](const double*) -> Feature* { return new AgeFeature(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    ECFLOW_REGISTER(feat_age, Feature, ageFeatureEntry());

    inline Registry<Feature>::Entry kinshipFeatureEntry()
    {
        return { "kinship", ModuleType::T_feature, ParameterTemplate{}, sizeof(KinshipFeature),
            [](const double*) -> Feature* { return new KinshipFeature(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    ECFLOW_REGISTER(feat_kinship, Feature, kinshipFeatureEntry());
}
