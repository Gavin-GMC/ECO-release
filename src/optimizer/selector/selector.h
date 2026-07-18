//------------------------Description------------------------
// 环境选择 EnvirSelect。**两层设计**:
//   第一层(selector 子类 / update_subswarm) = **找谁替换**(Index 逐位 / Close 最相似 / Rank 择优归并);
//   第二层(AcceptCriterion)               = **是否替换**(接受准则:无条件 / 择优爬山 / 退火 Metropolis)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <cmath>
#include <string>
#include <vector>
#include "individual.h"
#include "individual-array.h"
#include "ecflow-rand.h"
#include "ecflow-assert.h"     // acceptPostAssert 追加接受准则契约(如 age 计龄 → ageMaintained)
#include "terminator.h"      // update_subswarm 透传 terminator(第二层接受准则签名)
#include "best-archive.h"    // update_subswarm 透传 archive

namespace ECFlow
{
    // 接受准则枚举(= selector 注册项 para[0])
    enum class AcceptType { Unconditional = 0, Better = 1, Anneal = 2, Aging = 3 };

    // 第二层:是否用子代替换亲代(接受准则)。有状态准则(退火)经 cool/reset 管理温度。
    //   accept 透传 terminator/archive(供 AgingAccept 的 scout 记 FES + 更新档案);多数准则忽略。
    class AcceptCriterion
    {
    public:
        virtual ~AcceptCriterion() {}
        virtual void accept(Individual& offspring, Individual& parent, Terminator* terminator, BestArchive* archive) = 0;
        virtual void update() {}  // 每代推进准则内部状态(退火=降温;无状态准则空实现)
        virtual void reset()  {}  // exe(n) 多轮复位(无状态空实现)
        // INDIV-COMPOSE:age 感知准则声明所需特性(默认空);装配期回填身份键
        virtual std::vector<FeatureDemand> featureDemands() const { return {}; }
        virtual void setFeatureKey(const std::string& /*role*/, const std::string& /*key*/) {}
    };

    // 无条件接受:交换亲/子
    class UnconditionalAccept : public AcceptCriterion
    {
    public:
        void accept(Individual& offspring, Individual& parent, Terminator*, BestArchive*) override
        {
            parent.swap(offspring);
        }
    };

    // 择优接受(爬山):仅当子代更优(operator< 方向感知)才替换
    class BetterAccept : public AcceptCriterion
    {
    public:
        void accept(Individual& offspring, Individual& parent, Terminator*, BestArchive*) override
        {
            if (offspring < parent)
            {
                parent.swap(offspring);
            }
        }
    };

    // 退火接受(Metropolis):更优直接收;更差以 exp(-Δ/T) 概率收。T 每代乘性衰减(decay),reset 归初温 T0。
    class AnnealAccept : public AcceptCriterion
    {
    private:
        double _T0;            // 初温(初始参数)
        double _temperature;   // 当前温度(不用 _T:撞 Windows 宏 _T(x))
        double _decay;         // 每代衰减系数(T *= decay, <1;初始参数)
    public:
        // 参数模板声明 T0/decay 的 allow_empty=true(可留空),故**必须在此兑现该承诺**:
        //   留空(EMPTYVALUE)→ 回落内部默认。否则 NaN 会一路灌进 _temperature,而 Metropolis 里
        //   `exp(-Δ/NaN)` 与其后的比较**全为假** → 更差解永不被接受 → **退火静默退化成纯择优**(不报错,
        //   只是"看起来收敛得快")。放在 ctor 而非 makeAccept:任何构造路径(含直接 new)都受保护。
        static constexpr double T0_DEFAULT    = 100.0;
        static constexpr double DECAY_DEFAULT = 0.95;
        AnnealAccept(double T0 = T0_DEFAULT, double decay = DECAY_DEFAULT)
            : _T0(is_empty(T0) ? T0_DEFAULT : T0),
              _temperature(is_empty(T0) ? T0_DEFAULT : T0),   // 不写 _T0:避免依赖成员声明顺序
              _decay(is_empty(decay) ? DECAY_DEFAULT : decay) {}

        void accept(Individual& offspring, Individual& parent, Terminator*, BestArchive*) override
        {
            if (offspring < parent)   // 更优:直接接受
            {
                parent.swap(offspring);
                return;
            }
            // 更差:Metropolis 概率接受(Δ=目标绝对差;方向已由上面的 operator< 分流)
            double delta = std::fabs(offspring.solution.fitness[0] - parent.solution.fitness[0]);
            if (_temperature > 1e-12 && rand01() < std::exp(-delta / _temperature))
            {
                parent.swap(offspring);
            }
        }

        void update() override { _temperature *= _decay; }   // 每代降温(第一层 update_subswarm 每代末尾驱动)
        void reset()  override { _temperature = _T0; }        // exe(n) 每轮 ini → 复位初温
    };

    // 年龄感知贪心接受(= Better + 停滞计龄)。改进→swap+age 归零;未改进→ageing。
    //   **scout 已剥离**为独立 scout 段(AgeActivation 拓扑 + Reinitialize 策略),本准则只负责逐对的贪心替换与 trial 计数。
    //   age 特性 **Singular**(全个体一份、全程共享:employed/onlooker 计龄、AgeActivation 读、Reinitialize 归零);由本准则经
    //   featureDemands 声明、装配期推断挂载、setFeatureKey 回填身份键。契约 ageMaintained 由 acceptPostAssert 提供(见下)。
    class AgingAccept : public AcceptCriterion
    {
    private:
        std::string _age_key;    // 装配期解析的 age 特性身份键(Singular → 恒 "age")
    public:
        AgingAccept() {}

        std::vector<FeatureDemand> featureDemands() const override { return { { "age", "age", FeatureScope::Singular, {} } }; }
        void setFeatureKey(const std::string& role, const std::string& key) override { if (role == "age") _age_key = key; }

        void accept(Individual& offspring, Individual& parent, Terminator*, BestArchive*) override
        {
            if (offspring < parent)                                   // 更优 → 接受(=BetterAccept)
            {
                parent.swap(offspring);
                parent.feature<AgeFeature>(_age_key)->reset();        //   swap 后取新槽 age → 归零(改进)
            }
            else parent.feature<AgeFeature>(_age_key)->ageing();      // 未改进 → 计龄自增(= trial+1)
        }
    };

    // 按 para[0] 造接受准则:0 无条件 / 1 择优 / 2 退火(para[1]=T0, para[2]=decay)/ 3 年龄感知(仅计龄,scout 独立段)。
    //   para 为空 → 无条件(缺省)。退火参数 T0/decay **允许留空**(参数模板 allow_empty=true):留空即 EMPTYVALUE,
    //   由 AnnealAccept 的 ctor 回落内部默认(见其注释)。**勿在此处判空** —— 兑现承诺的责任在 ctor,
    //   以覆盖所有构造路径(原注释称"由 config-setter 保证给满",与 allow_empty=true 的声明自相矛盾,已订正)。
    inline AcceptCriterion* makeAccept(const double* p)
    {
        int type = p ? (int)p[0] : 0;
        if (type == (int)AcceptType::Better) return new BetterAccept();
        if (type == (int)AcceptType::Anneal) return new AnnealAccept(p[1], p[2]);
        if (type == (int)AcceptType::Aging)  return new AgingAccept();
        return new UnconditionalAccept();
    }

    // 接受准则的**装配期契约贡献**(与 makeAccept 对称,按 accept_type 派发)。选择器 Entry 的 postAssert **追加**本函数,
    //   使"选择器断言 = 自身 ++ 接受准则断言";新接受准则的契约只在此加一行,所有带 accept_type 的选择器自动同步。
    //   age 计龄(type 3)→ 提供 ageMaintained,供 age 消费方(AgeActivation scout 拓扑)的 preAssert 匹配。
    inline void acceptPostAssert(const double* p, AssertList& out)
    {
        int type = p ? (int)p[0] : 0;
        if (type == (int)AcceptType::Aging)
            out.add(new Assert(ModuleType::T_selector, "ageMaintained", 1, MatchType::postAssert));
    }

    // 接受准则的**参数模板贡献**(与 makeAccept / acceptPostAssert 对称,按 accept_type 派发)。
    //   选择器 Entry 的 `tail` 指向本函数,使"选择器参数 = 自身 `{accept_type}` ++ 接受准则的参数" —— 与
    //   acceptPostAssert 的追加式**完全对仗**:新接受准则的参数只在此加一行,所有带 accept_type 的选择器自动同步。
    // 为何必须如此:T0/decay 是 **AnnealAccept 的**参数,不是选择器的。原先把它们拍平硬编进 `Index`/`Close`/
    //   `Kinship` 的模板 = 在语义上把选择器**绑定到了退火接受**,与"选择器与接受准则正交"的设计相悖
    //   (旁证:手册曾据此误记 accept_type=3 带 `limit` 参数,而 AgingAccept 零参、limit 实为 AgeActivation 拓扑的)。
    //   四个接受准则中**仅 AnnealAccept 带参**,其余三个尾部为空。
    inline ParameterTemplate acceptParaTemplate(const double* head, size_t cnt)
    {
        int type = (head && cnt >= 1) ? (int)head[0] : 0;
        if (type == (int)AcceptType::Anneal)
            return ParameterTemplate{ { {"T0",    ParamKind::Real, 0.0, 1e9, true, 1, 100},     // 退火初温(留空→AnnealAccept ctor 回落 100.0)
                                        {"decay", ParamKind::Real, 0.0, 1.0, true, 0.9, 0.99} } };   // 每代衰减(留空→回落 0.95)
        return ParameterTemplate{};   // 无条件 / 择优 / 年龄计龄:零参数
    }

    // 第一层:找谁替换。持一个 AcceptCriterion(第二层);reset 转发退火复位(RunSelect.ini 每轮调)。
    //   透传 terminator/archive 给接受层(AgingAccept scout 用);转发 featureDemands/setFeatureKey(age 声明,装配期)。
    class EnvirSelect
    {
    protected:
        AcceptCriterion* _accept;

    public:
        EnvirSelect(AcceptCriterion* accept) : _accept(accept) {}
        virtual ~EnvirSelect() { delete _accept; }

        virtual void update_subswarm(IndividualArray& parent, IndividualArray& offspring, Terminator* terminator, BestArchive* archive) = 0;
        virtual void reset() { if (_accept) _accept->reset(); }
        // 默认转发接受层需求(age 等);带自身逐个体状态的选择器(如 KinshipSelector)覆写以追加自身声明。
        virtual std::vector<FeatureDemand> featureDemands() const { return _accept ? _accept->featureDemands() : std::vector<FeatureDemand>{}; }
        virtual void setFeatureKey(const std::string& role, const std::string& key) { if (_accept) _accept->setFeatureKey(role, key); }
    };
}
