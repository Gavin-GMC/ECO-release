//------------------------Description------------------------
// 变异算子:通过以特定方式改变当前个体的片段来开发邻域。基类 Mutation + 8 种具体变异。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <cmath>
#include <cstring>
#include <algorithm>
#include <vector>
#include "ecflow-constant.h"
#include "ecflow-basicfunc.h"
#include "solution.h"
#include "individual.h"
#include "problem-handle.h"
#include "learning-strategy.h"
#include "registry.h"

namespace ECFlow
{
    // 变异族基类:仅持速率。**该速率的语义由派生族界定**(见下两族),不在此假定。
    class Mutation : public LearningStrategy
    {
    protected:
        double _mutation_rate;

    public:
        Mutation(double mutation_rate) { _mutation_rate = mutation_rate; }
        virtual ~Mutation() {}
    };

    // ============================ 逐维变异族============================
    // **门控在基类、不在算子里** —— 子类只答"这一维**变成什么**"(mutatedValue),
    //   "这一维**变不变**"由本类统一裁决。本族的 `_mutation_rate` = **逐维选中概率**。
    //
    // 【为何如此:一次原语化回归的修复】
    //   archive/ECFC稳定版本 的 `Mutation` 基类持有整解驱动 `apply(Solution*, ProblemHandle*)`:
    //       for (i : 全维) if (rand01() < _mutation_rate) solution->result[i] = apply(solution, i, handle);
    //   —— 门控在基类,统一施加于所有逐维算子;子类的 `apply(sol,dim,handle)` 无条件返回"变成什么"。
    //   **ECFC原代码**把这层整解驱动拆掉、改由生成器逐维调 `nextDecision` 后,门控**掉到了各算子头上**,
    //   而只有 `BitMutation` 补上了 → `BitFlip`/`PM`/`Gauss` 的 `mutation_rate` **成了死参数**
    //   (声明了、能配、能存配置表、被 ParamMatcher 校验、被自动生成采样,却对行为零影响;
    //    用户 `gaussMutation(0.5, 0.01)` 以为设了 1% 变异率,实际**每维必扰**)。v2 忠实移植 → 忠实继承该缺陷。
    //   本族即把那层还给基类,与稳定版同构(开发纪律 5:有状态/整群策略以稳定版为准,原语化有回归)。
    //   旁证:`PM` 的逐维门控**在文献上也是对的** —— 标准 NSGA-II 的多项式变异即"逐维以 pm 概率施加"(常取 1/n)。
    class PerDimMutation : public Mutation
    {
    protected:
        // 子类实现:这一维**变成什么**(无条件给值;是否采用由 nextDecision 裁决)
        virtual double mutatedValue(const int decision_d, Individual* individual, ProblemHandle* problem_handle, Individual* child) = 0;

    public:
        PerDimMutation(double mutation_rate) : Mutation(mutation_rate) {}
        virtual ~PerDimMutation() {}

        // 与稳定版 Mutation::apply 的循环体同构(生成器已逐维驱动,故此处只做单维的门控)
        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual* child) override
        {
            if (rand01() < _mutation_rate)
                return mutatedValue(decision_d, individual, problem_handle, child);
            return individual->solution[decision_d];   // 未选中 → 保持原值
        }
    };

    // 位变异:选中的维取域内随机值
    class BitMutation : public PerDimMutation
    {
    protected:
        double mutatedValue(const int decision_d, Individual*, ProblemHandle* problem_handle, Individual*) override
        {
            return problem_handle->getRandomChoiceInspace(decision_d);
        }
    public:
        BitMutation(double mutation_rate = 0.01) : PerDimMutation(mutation_rate) {}
        ~BitMutation() {}
        static void postAssert(AssertList& list, double* paras) { list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert)); }
    };

    // 位翻转变异:选中的维 0↔1
    class BitFlipMutation : public PerDimMutation
    {
    protected:
        double mutatedValue(const int decision_d, Individual* individual, ProblemHandle*, Individual*) override
        {
            return individual->solution.result[decision_d] ? 0 : 1;
        }
    public:
        BitFlipMutation(double mutation_rate = 0.01) : PerDimMutation(mutation_rate) {}
        ~BitFlipMutation() {}
        static void postAssert(AssertList& list, double* paras) { list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert)); }
    };

    // 反转变异:反转随机片段 _times 次
    class OverturnMutation : public Mutation
    {
    private:
        int _times;

    public:
        OverturnMutation(int times = 1, double mutation_rate = 0.01) : Mutation(mutation_rate) { _times = times; }
        ~OverturnMutation() {}

        void getNewIndividual(Individual* child_individual, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle) override
        {
            Solution* solution = &child_individual->solution;
            solution->copy(individual->solution);

            if (_mutation_rate < rand01()) // 不变异
                return;

            int oid1, oid2;
            int s_size = solution->getSolutionSize();
            for (int i = 0; i < _times; i++)
            {
                oid1 = ECFlow::get_int(0, s_size - 1);
                oid2 = ECFlow::get_int(0, s_size - 1);
                if (oid1 > oid2)
                    std::swap(oid1, oid2);
                while (oid1 < oid2)
                    std::swap(solution->result[oid1++], solution->result[oid2--]);
            }
        }
    };

    // 交换变异:交换随机两位 _times 次
    class ExchangeMutation : public Mutation
    {
    private:
        int _times;

    public:
        ExchangeMutation(int times = 1, double mutation_rate = 1) : Mutation(mutation_rate) { _times = times; }
        ~ExchangeMutation() {}

        void getNewIndividual(Individual* child_individual, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle) override
        {
            Solution* solution = &child_individual->solution;
            solution->copy(individual->solution);

            if (_mutation_rate < rand01()) // 不变异
                return;

            int eid1, eid2;
            int s_size = solution->getSolutionSize();
            for (int i = 0; i < _times; i++)
            {
                eid1 = ECFlow::get_int(0, s_size - 1);
                eid2 = ECFlow::get_int(0, s_size - 1);
                std::swap(solution->result[eid1], solution->result[eid2]);
            }
        }
    };

    // 多项式变异(Polynomial Mutation)
    class PM_Mutation : public PerDimMutation
    {
    private:
        double _eta;
        double _r;
        double _sigma;

    protected:
        //   原实现无条件施加(mutation_rate 是死参数);现"选中才调本函数",与稳定版及 NSGA-II 原文的
        //   "逐维以 pm 概率施加多项式变异"一致。
        double mutatedValue(const int decision_d, Individual* individual, ProblemHandle* problem_handle, Individual* child) override
        {
            double upb = problem_handle->getVariableUpbound(decision_d);
            double lowb = problem_handle->getVariableLowbound(decision_d);

            // 消副作用:局部钳制,不写回亲代(输出对 child 逐位一致)
            double x = individual->solution.result[decision_d];
            if (x > upb)       x = upb;
            else if (x < lowb) x = lowb;

            _r = rand01_();
            if (_r > 0.5)
                _sigma = 1 - pow(2 * (1 - _r) + 2 * (_r - 0.5) * pow(1 - (upb - x) / (upb - lowb), _eta + 1), 1 / (_eta + 1));
            else
                _sigma = pow(2 * _r + (1 - 2 * _r) * pow(1 - (x - lowb) / (upb - lowb), _eta + 1), 1 / (_eta + 1)) - 1;

            return x + _sigma * (upb - lowb);
        }

    public:
        PM_Mutation(double eta = 20, double mutation_rate = 0.01) : PerDimMutation(mutation_rate)
        {
            _eta = eta;
            _r = 0;
            _sigma = 0;
        }
        ~PM_Mutation() {}
        static void postAssert(AssertList& list, double* paras) { list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert)); }
    };

    // 高斯变异:加高斯噪声
    class GaussMutation : public PerDimMutation
    {
    private:
        double _sigma;

        double guassion()
        {
            return _sigma * (
                sqrt((-2) * log(ECFlow::rand01()))
                * sin(2 * 3.1415926 * ECFlow::rand01()));
        }

    protected:
        //   原实现无条件加噪(mutation_rate 是死参数,手册曾注"当前实现忽略,逐维必扰");现"选中才加噪"。
        double mutatedValue(const int decision_d, Individual* individual, ProblemHandle*, Individual*) override
        {
            return individual->solution.result[decision_d] + guassion();
        }

    public:
        GaussMutation(double sigma = 1, double mutation_rate = 0.01) : PerDimMutation(mutation_rate) { _sigma = sigma; }
        ~GaussMutation() {}
        static void postAssert(AssertList& list, double* paras) { list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert)); }
    };

    // 插入变异:摘取一位重插到另一位 _times 次
    class InsertMutation : public Mutation
    {
    private:
        int _times;

    public:
        InsertMutation(int times = 1, double mutation_rate = 0.01) : Mutation(mutation_rate) { _times = times; }
        ~InsertMutation() {}

        void getNewIndividual(Individual* child_individual, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle) override
        {
            Solution* solution = &child_individual->solution;
            solution->copy(individual->solution);

            if (_mutation_rate < rand01()) // 不变异
                return;

            int oid1; // 选择位
            int oid2; // 插入位
            int s_size = solution->getSolutionSize();
            for (int i = 0; i < _times; i++)
            {
                oid1 = ECFlow::get_int(0, s_size - 1);
                oid2 = ECFlow::get_int(0, s_size - 1);

                double buffer = (*solution)[oid1];

                if (oid1 < oid2)
                {
                    memmove(solution->result + oid1, solution->result + oid1 + 1, sizeof(double) * (oid2 - oid1));
                    (*solution)[oid2] = buffer;
                }
                else
                {
                    memmove(solution->result + oid2 + 1, solution->result + oid2, sizeof(double) * (oid1 - oid2));
                    (*solution)[oid2] = buffer;
                }
            }
        }
    };

    // 扰乱变异:打乱随机片段 _times 次
    class ReorderMutation : public Mutation
    {
    private:
        int _times;

    public:
        ReorderMutation(int times = 1, double mutation_rate = 0.01) : Mutation(mutation_rate) { _times = times; }
        ~ReorderMutation() {}

        void getNewIndividual(Individual* child_individual, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle) override
        {
            Solution* solution = &child_individual->solution;
            solution->copy(individual->solution);

            if (_mutation_rate < rand01()) // 不变异
                return;

            int oid1, oid2;
            int s_size = solution->getSolutionSize();
            for (int i = 0; i < _times; i++)
            {
                oid1 = ECFlow::get_int(0, s_size - 1);
                oid2 = ECFlow::get_int(0, s_size - 1);

                if (oid2 < oid1)
                    std::swap(oid1, oid2);

                oid2++; // 打乱时索引小于该值

                ECFlow::shuffle(solution->result + oid1, oid2 - oid1);   // 修复:原 std::random_device → ECFlow 引擎(可复现)
            }
        }
    };

    // 段重定位(3-opt 纯平移 t4 / or-opt 段级):删 3 边把序列分 a|b|c|d(a、d 固定),**段 b、c 换位**(c 提前、b 挪后,
    //   **不反转**)= 把一整段搬到别处。这是 3-opt 相对 2-opt 的独有能力(Overturn 只能就地反转、搬不了段)。
    //   反转维度**正交剥离**给 Overturn(2opt):需"平移+反转"时在 workflow 里 SegmentRelocate 后跟 Overturn 组合,语义可控。
    class SegmentRelocate : public Mutation
    {
    private:
        int _times;

        // 区间 [i+1..k] = b段[i+1..j] + c段[j+1..k];纯平移换位为 c 接 b(不反转),写临时 buffer 再拷回。a、d 不动。
        static void relocate(double* r, int i, int j, int k)
        {
            int lb = j - i;          // b 段长度([i+1..j])
            int lc = k - j;          // c 段长度([j+1..k])
            std::vector<double> buf(lb + lc);
            for (int x = 0; x < lc; x++) buf[x]      = r[j + 1 + x];   // c 在前
            for (int x = 0; x < lb; x++) buf[lc + x] = r[i + 1 + x];   // b 在后
            for (int x = 0; x < lb + lc; x++) r[i + 1 + x] = buf[x];   // 拷回 [i+1..k]
        }

    public:
        SegmentRelocate(int times = 1, double mutation_rate = 0.01) : Mutation(mutation_rate) { _times = times; }
        ~SegmentRelocate() {}

        void getNewIndividual(Individual* child_individual, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle) override
        {
            Solution* solution = &child_individual->solution;
            solution->copy(individual->solution);

            if (_mutation_rate < rand01()) // 不变异
                return;

            int s_size = solution->getSolutionSize();
            if (s_size < 3)                // 不足 3 段,无法段重定位
                return;

            for (int t = 0; t < _times; t++)
            {
                // 三断点:段 a=[0..i] b=[i+1..j] c=[j+1..k] d=[k+1..s_size-1](b、c 非空)。
                // **i 下界取 -1(非 0)**:允许段 a 空 → 段 c 可提到最前;i=-1 且 k=s_size-1 即"整段分两段交换"。
                //   否则 result[0] 恒留最前,邻域不对称(只能把段搬到后/中,搬不到最前);d 空(k=s_size-1)已覆盖"搬到最后"。
                int i = ECFlow::get_int(-1, s_size - 3);
                int j = ECFlow::get_int(i + 1, s_size - 2);
                int k = ECFlow::get_int(j + 1, s_size - 1);
                relocate(solution->result, i, j, k);   // 纯段平移(c、b 换位,不反转;i=-1 时拷回区间为 [0..k])
            }
        }
    };

    // 自注册进 Registry<LearningStrategy>(T_learnstrategy)。变异均无前/后置断言。
    inline Registry<LearningStrategy>::Entry bitMutationEntry()
    {
        return { "Bit", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"mutation_rate", ParamKind::Real, 0.0, 1.0, false, 0.001, 0.1} } }, sizeof(BitMutation),
            [](const double* p) -> LearningStrategy* { return p ? new BitMutation(p[0]) : new BitMutation(); },
            [](AssertList&, const double*) {}, [](AssertList& L, const double* p) { BitMutation::postAssert(L, const_cast<double*>(p)); } };
    }
    inline Registry<LearningStrategy>::Entry bitFlipMutationEntry()
    {
        return { "BitFlip", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"mutation_rate", ParamKind::Real, 0.0, 1.0, false, 0.001, 0.1} } }, sizeof(BitFlipMutation),
            [](const double* p) -> LearningStrategy* { return p ? new BitFlipMutation(p[0]) : new BitFlipMutation(); },
            [](AssertList&, const double*) {}, [](AssertList& L, const double* p) { BitFlipMutation::postAssert(L, const_cast<double*>(p)); } };
    }
    inline Registry<LearningStrategy>::Entry overturnMutationEntry()
    {
        return { "Overturn", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"times", ParamKind::Int, 1, 0x3f3f3f3f, false, 1, 3}, {"mutation_rate", ParamKind::Real, 0.0, 1.0, false, 0.01, 0.2} } }, sizeof(OverturnMutation),
            [](const double* p) -> LearningStrategy* { return p ? new OverturnMutation(int(p[0]), p[1]) : new OverturnMutation(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    // 多点同值变异:**逐维**以 point_rate 概率独立选中,被选中的维**全部置为同一个**域内随机值
    //   (经句柄 getRandomChoiceInspace,含域边界+离散化)。
    // 与 `Bit`(逐点变异)**参数语义一致**——都是"每一维被选中的概率";**唯一区别**:
    //   `Bit` = 每个选中维**各自独立**取随机值;本算子 = 选中维**共用同一个**随机值(一次"归并式"移动)。
    // 因需跨维协同(共用同一个值),故为整体型、override getNewIndividual(不能走逐维 nextDecision)。
    // 通用编码、与问题场景无关。选中 0 维 → 不变异;选中 1 维 → 自然退化为一次点变异。
    class MultiPointSameValue : public Mutation
    {
    public:
        // point_rate:**每一维被选中的概率**(与 Bit 的 mutation_rate 同义);基类 _mutation_rate 即承载它
        explicit MultiPointSameValue(double point_rate = 0.2) : Mutation(point_rate) {}
        ~MultiPointSameValue() {}

        void getNewIndividual(Individual* child_individual, Individual* individual, Solution** /*learning_object*/, ProblemHandle* problem_handle) override
        {
            Solution* solution = &child_individual->solution;
            solution->copy(individual->solution);

            int n = solution->getSolutionSize();
            if (n <= 0) return;

            // 逐维独立以 point_rate 选中(同 Bit 的逐维掷率)
            std::vector<int> picked;
            picked.reserve(n);
            for (int d = 0; d < n; d++) if (rand01() < _mutation_rate) picked.push_back(d);
            if (picked.empty()) return;   // 一维未中 → 不变异

            // 取**一个**域内随机值(以首个选中维的值域为准;各维域不同时,越界交约束修复层处理)
            double v = problem_handle->getRandomChoiceInspace(picked[0]);
            for (int q : picked) solution->result[q] = v;   // 选中的维全置同值
        }
    };

    // 集合交换:随机选两维、记其取值为 a、b(a==b 则无可交换、原样返回);收集**全解中取值为 a 的所有维**(集合 A)与
    //   **取值为 b 的所有维**(集合 B);对 A、B **各随机取一个子集**(逐元素以 subset_rate 独立入选),
    //   把选中的 A 子集置为 b、选中的 B 子集置为 a(**互换为对方的取值**)。
    // 与逐维/环状类算子的区别:它是**值类(等值维集合)层面**的成组迁移——一次动的是"同值的一批维",
    //   而非若干个孤立维。指派编码下 = 两个"值类"(如两台机器上的工序集)各挑一批互换归属。
    // 子集独立取 → 两侧规模可不等 → **值多重集一般不守恒**(有意为之:成组再平衡);subset_rate=1 时退化为
    //   **两个值类整体对调**;subset_rate=0 → 两侧子集皆空 → 不变异。
    // subset_rate = EMPTYVALUE(未设置)→ **免参模式**:每次调用为 A、B **两侧各随机抽一个概率** pA、pB ~ U(0,1),
    //   再各按自己的概率选取。两侧概率独立 → 天然产生**非对称迁移**(一侧大批给出、另一侧少量回让),
    //   且子集规模分布展宽(p~U(0,1) 时 |子集| 近似均匀分布于 0..|类|),覆盖从微调到整体对调的全谱。
    // 整体型(需先扫全解成类、再成组改写),故 override getNewIndividual。通用编码、与问题场景无关。
    class SetExchange : public Mutation
    {
    private:
        bool _random_rate;   // subset_rate 未设置 → 每次调用两侧各随机抽概率
    public:
        // subset_rate:值类中**每个成员被选入子集的概率**(A、B 两侧独立掷);EMPTYVALUE → 免参模式(两侧各随机抽概率)
        explicit SetExchange(double subset_rate = EMPTYVALUE) : Mutation(subset_rate)
        {
            _random_rate = is_empty(subset_rate);   // 必须 is_empty:裸 ==EMPTYVALUE 对 NaN 恒假(参见 Difference 的同类 bug)
        }
        ~SetExchange() {}

        void getNewIndividual(Individual* child_individual, Individual* individual, Solution** /*learning_object*/, ProblemHandle* /*problem_handle*/) override
        {
            Solution* solution = &child_individual->solution;
            solution->copy(individual->solution);

            int n = solution->getSolutionSize();
            if (n < 2) return;

            // 随机选两维,取其值 a、b
            double a = solution->result[ECFlow::get_int(0, n - 1)];
            double b = solution->result[ECFlow::get_int(0, n - 1)];
            if (a == b) return;   // 同值 → 无可交换(按规格:不重挑)

            // 先收集两个值类的下标(必须先收集完再改写,否则改写会污染后续判定)
            std::vector<int> setA, setB;
            for (int d = 0; d < n; d++)
            {
                if (solution->result[d] == a)      setA.push_back(d);
                else if (solution->result[d] == b) setB.push_back(d);
            }

            // 免参模式:两侧**各自**随机抽一个概率(独立)→ 非对称迁移;否则两侧共用给定的 subset_rate
            double rate_a = _random_rate ? rand01() : _mutation_rate;
            double rate_b = _random_rate ? rand01() : _mutation_rate;

            // 各取随机子集(逐元素独立入选)→ 置为对方的取值
            for (int d : setA) if (rand01() < rate_a) solution->result[d] = b;
            for (int d : setB) if (rand01() < rate_b) solution->result[d] = a;
        }
    };

    inline Registry<LearningStrategy>::Entry setExchangeEntry()
    {
        return { "SetExchange", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"subset_rate", ParamKind::Real, 0.0, 1.0, true, 0.3, 0.7} } }, sizeof(SetExchange),   // 值类成员入选子集的概率;可留空(EMPTYVALUE)→ 两侧各随机抽概率
            [](const double* p) -> LearningStrategy* { return p ? new SetExchange(p[0]) : new SetExchange(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    ECFLOW_REGISTER(lstrat_setexchange, LearningStrategy, setExchangeEntry());

    // 值类外迁:随机选一维、记其取值为 a(迁出类);收集**全解中取值=a 的所有维**(集合 A),对 A 随机取一个子集
    //   (逐元素以 subset_rate 独立入选),**整体置为一个新抽取的值 c**(经句柄 getRandomChoiceInspace);c==a 则无迁移。
    // 与 SetExchange 的关键区别:**单向**。SetExchange 只在**两个已有类之间**对调,故 ①永远启用不了"当前无维取到的值"
    //   ②拆不开一个类(两侧同时动)。本算子两者皆可:c 落在未使用值 → **把类劈出一块到新值上**(如启用一台闲置机器);
    //   c 落在已有类 → **单向并入**(SetExchange 做不到的不对等动作)。
    // 与 MultiPointSameValue 的区别:那个**逐维独立**选(选中维可横跨多个类);本算子**按类**选(动的必是同一类的成员)。
    // 值多重集不守恒(类规模变化,有意为之)。subset_rate = EMPTYVALUE → 免参模式:每次调用随机抽一个概率,
    //   子集规模分布展宽 → 覆盖"迁一维"到"整类迁走"的全谱。
    // 整体型(需先扫全解成类、再成组改写),故 override getNewIndividual。通用编码、与问题场景无关。
    class ClassRelocate : public Mutation
    {
    private:
        bool _random_rate;   // subset_rate 未设置 → 每次调用随机抽概率
    public:
        // subset_rate:迁出类中**每个成员被选入子集的概率**;EMPTYVALUE → 免参模式(每次随机抽)
        explicit ClassRelocate(double subset_rate = EMPTYVALUE) : Mutation(subset_rate)
        {
            _random_rate = is_empty(subset_rate);   // 必须 is_empty:裸 ==EMPTYVALUE 对 NaN 恒假
        }
        ~ClassRelocate() {}

        void getNewIndividual(Individual* child_individual, Individual* individual, Solution** /*learning_object*/, ProblemHandle* problem_handle) override
        {
            Solution* solution = &child_individual->solution;
            solution->copy(individual->solution);

            int n = solution->getSolutionSize();
            if (n <= 0) return;

            // 随机选一维,取其值 a = 迁出类
            double a = solution->result[ECFlow::get_int(0, n - 1)];

            // 先收集迁出类的下标,再取子集
            std::vector<int> setA;
            for (int d = 0; d < n; d++) if (solution->result[d] == a) setA.push_back(d);

            double rate = _random_rate ? rand01() : _mutation_rate;
            std::vector<int> picked;
            for (int d : setA) if (rand01() < rate) picked.push_back(d);
            if (picked.empty()) return;

            // 新值 c:由句柄在**域内**抽(可能落在未使用值 → 拆出新类;也可能落在已有类 → 单向并入)
            double c = problem_handle->getRandomChoiceInspace(picked[0]);
            if (c == a) return;   // 抽到同值 → 无迁移(按规格:不重挑)

            for (int d : picked) solution->result[d] = c;
        }
    };

    inline Registry<LearningStrategy>::Entry classRelocateEntry()
    {
        return { "ClassRelocate", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"subset_rate", ParamKind::Real, 0.0, 1.0, true, 0.3, 0.7} } }, sizeof(ClassRelocate),   // 迁出类成员入选子集的概率;可留空(EMPTYVALUE)→ 每次随机抽
            [](const double* p) -> LearningStrategy* { return p ? new ClassRelocate(p[0]) : new ClassRelocate(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    ECFLOW_REGISTER(lstrat_classrelocate, LearningStrategy, classRelocateEntry());

    // 块互换:四断点把序列分 前|A|M|B|后,**A 与 B 互换**(中段 M 内容与次序不动,仅随 |B|-|A| 平移)。
    //   A、B **长度可不等、且可相隔任意距离**(M 可空)。值多重集守恒 → 排列编码天然保持可行。
    // 与既有序列算子的关系:它是**统一的推广**,两个既有算子都是其退化特例——
    //   * M 空(A、B 相邻)     → 即 SegmentRelocate(相邻两段换位);
    //   * |A|=|B|=1(单维对单维) → 即 Exchange(两点对换)。
    //   真正新增的邻域 = **长度不等 且 相隔有距** 的两块互换(既有算子一步够不着:SegmentRelocate 限相邻、
    //   Exchange 限单维)。该动作需 3 次 SegmentRelocate 才能复合出来,故作为单步邻域是实质扩张。
    // 反转维度按既定设计**不并入**(需要时在 workflow 里组合 Overturn),与 SegmentRelocate 的正交剥离保持一致。
    class BlockSwap : public Mutation
    {
    private:
        int _times;

        // 区间 [i1..j2] = A[i1..j1] + M[j1+1..i2-1] + B[i2..j2] → 重排为 B + M + A。写临时 buffer 再拷回。
        static void blockswap(double* r, int i1, int j1, int i2, int j2)
        {
            int la = j1 - i1 + 1;    // A 段长
            int lm = i2 - j1 - 1;    // M 段长(可为 0 → 退化为相邻两段互换)
            int lb = j2 - i2 + 1;    // B 段长
            std::vector<double> buf(la + lm + lb);
            int x = 0;
            for (int t = 0; t < lb; t++) buf[x++] = r[i2 + t];       // B 在前
            for (int t = 0; t < lm; t++) buf[x++] = r[j1 + 1 + t];   // M 居中(次序不变)
            for (int t = 0; t < la; t++) buf[x++] = r[i1 + t];       // A 在后
            for (int t = 0; t < (int)buf.size(); t++) r[i1 + t] = buf[t];
        }

    public:
        BlockSwap(int times = 1, double mutation_rate = 1) : Mutation(mutation_rate) { _times = times; }
        ~BlockSwap() {}

        void getNewIndividual(Individual* child_individual, Individual* individual, Solution** /*learning_object*/, ProblemHandle* /*problem_handle*/) override
        {
            Solution* solution = &child_individual->solution;
            solution->copy(individual->solution);

            if (_mutation_rate < rand01()) return;   // 整体型:mutation_rate = 本次是否施加

            int s_size = solution->getSolutionSize();
            if (s_size < 2) return;                  // 不足两块

            for (int t = 0; t < _times; t++)
            {
                // 四断点:A=[i1..j1]、B=[i2..j2],保证 i1<=j1 < i2<=j2(两块非空、不重叠;M=[j1+1..i2-1] 可空)
                int i1 = ECFlow::get_int(0, s_size - 2);
                int j1 = ECFlow::get_int(i1, s_size - 2);        // 至少给 B 留 1 维
                int i2 = ECFlow::get_int(j1 + 1, s_size - 1);
                int j2 = ECFlow::get_int(i2, s_size - 1);
                blockswap(solution->result, i1, j1, i2, j2);
            }
        }
    };

    inline Registry<LearningStrategy>::Entry blockSwapEntry()
    {
        return { "BlockSwap", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"times",         ParamKind::Int,  1, 0x3f3f3f3f, false, 1, 3},
                                 {"mutation_rate", ParamKind::Real, 0.0, 1.0,      false, 0.1, 1.0} } }, sizeof(BlockSwap),
            [](const double* p) -> LearningStrategy* { return p ? new BlockSwap((int)p[0], p[1]) : new BlockSwap(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    ECFLOW_REGISTER(lstrat_blockswap, LearningStrategy, blockSwapEntry());

    // 值类合并:随机选两维、记其取值为 a、b(a==b 则无可合并);把**全解中取值=a 的所有维**整体置为 b
    //   → 类 a 彻底消失、并入类 b。**在用的不同值个数恰好减 1**(整合/关停语义:合并两台机器的负载、装箱合并)。
    // 与另两个值类算子的区别(合并性由此保证):
    //   * SetExchange 是**双向对调**,在用值个数不变 → 永远合并不掉一个类;
    //   * ClassRelocate 的目标值 c 由域内**随机抽**,抽不中"必被占用"→ c 落在未使用值时只是**改名**,
    //     在用值个数不减。本算子的 b **取自另一维的现值**,故必被占用 → **确定性地**少一个类。
    //   形式上本算子 = SetExchange 单向满率(rate_a=1、rate_b=0)的特例,但那是 SetExchange 参数表达不了的
    //   (两侧共用一个 rate),故独立成算子;行为名清晰、语义单一。
    // 值多重集不守恒。整体型,故 override getNewIndividual。通用编码、与问题场景无关。
    // 单趟改写即可:目标 b != a,置 b 后不会再被当作 a 命中,无需先收集下标。
    class ClassMerge : public Mutation
    {
    public:
        // mutation_rate:本次是否施加(整体型算子的族内惯例;本算子无子集,故无规模参数)
        explicit ClassMerge(double mutation_rate = 1) : Mutation(mutation_rate) {}
        ~ClassMerge() {}

        void getNewIndividual(Individual* child_individual, Individual* individual, Solution** /*learning_object*/, ProblemHandle* /*problem_handle*/) override
        {
            Solution* solution = &child_individual->solution;
            solution->copy(individual->solution);

            if (_mutation_rate < rand01()) return;   // 不施加

            int n = solution->getSolutionSize();
            if (n < 2) return;

            // 两维现值:a = 被合并掉的类、b = 承接类(取自现值 → **必被占用**,这是"确定减一"的来源)
            double a = solution->result[ECFlow::get_int(0, n - 1)];
            double b = solution->result[ECFlow::get_int(0, n - 1)];
            if (a == b) return;   // 同值 → 无可合并(按规格:不重挑)

            for (int d = 0; d < n; d++) if (solution->result[d] == a) solution->result[d] = b;
        }
    };

    inline Registry<LearningStrategy>::Entry classMergeEntry()
    {
        return { "ClassMerge", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"mutation_rate", ParamKind::Real, 0.0, 1.0, false, 0.1, 1.0} } }, sizeof(ClassMerge),
            [](const double* p) -> LearningStrategy* { return p ? new ClassMerge(p[0]) : new ClassMerge(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    ECFLOW_REGISTER(lstrat_classmerge, LearningStrategy, classMergeEntry());

    // 循环交换(多交换的随机版):随机选 k 个**互异**维,把它们的值做**环状轮转**——
    //   `x[d0]←x[d1], x[d1]←x[d2], …, x[d(k−1)]←x[d0]`。
    // **k=2 时严格退化为 `Exchange`(对换)** → 本算子是"对换/单点迁移"的**真泛化**(k 阶复合移动)。
    // 关键性质:**值的多重集守恒**(只换归属、不增删值) → **排列编码下天然保持可行**(不造重复值);
    //   指派编码下 = k 个元素沿环同时换集合(各集合出一进一、规模守恒)。
    // 出处(思想):Thompson & Orlin《The theory of cyclic transfers》1989;Ahuja/Orlin/Sharma, *Math. Prog.* 2001;
    //   Ahuja/Ergun/Orlin/Punnen《VLSN 综述》*Discrete Applied Mathematics* 2002。
    //   注:原文精髓的"**改进图搜负环找最优环**"需**增量代价**(解码后结构知识)→ 属**问题侧**;
    //   本算子实现的是**随机 k 环**版(纯编码级、问题无关),择优交选择器——与框架"生成/接受"两层分工一致。
    // 整体型(跨维协同:同一批维 + 一次轮转),故 override getNewIndividual。
    class CyclicExchange : public Mutation
    {
    private:
        int _cycle_length;   // 环长 k(k=2 即对换)

    public:
        CyclicExchange(int cycle_length = 3, double mutation_rate = 1) : Mutation(mutation_rate) { _cycle_length = cycle_length; }
        ~CyclicExchange() {}

        void getNewIndividual(Individual* child_individual, Individual* individual, Solution** /*learning_object*/, ProblemHandle* /*problem_handle*/) override
        {
            Solution* solution = &child_individual->solution;
            solution->copy(individual->solution);

            if (_mutation_rate < rand01()) return;   // 不变异

            int n = solution->getSolutionSize();
            int k = (_cycle_length < n) ? _cycle_length : n;   // k 超解维时截断
            if (k < 2) return;                                 // 环长 <2 无意义(自环=不动)

            // 无放回选 k 个互异维
            std::vector<int> picked;
            picked.reserve(k);
            while ((int)picked.size() < k)
            {
                int d = ECFlow::get_int(0, n - 1);
                bool dup = false;
                for (int q : picked) if (q == d) { dup = true; break; }
                if (!dup) picked.push_back(d);
            }

            // 环状轮转:每维取"环上下一维"的原值,末维取首维原值(单个 k 环)
            double first = solution->result[picked[0]];
            for (int i = 0; i < k - 1; i++) solution->result[picked[i]] = solution->result[picked[i + 1]];
            solution->result[picked[k - 1]] = first;
        }
    };

    inline Registry<LearningStrategy>::Entry cyclicExchangeEntry()
    {
        return { "CyclicExchange", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"cycle_length",  ParamKind::Int,  2, 0x3f3f3f3f, false, 3, 5},
                                 {"mutation_rate", ParamKind::Real, 0.0, 1.0, false, 0.01, 1.0} } }, sizeof(CyclicExchange),
            [](const double* p) -> LearningStrategy* { return p ? new CyclicExchange(int(p[0]), p[1]) : new CyclicExchange(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    ECFLOW_REGISTER(lstrat_cyclicexchange, LearningStrategy, cyclicExchangeEntry());

    inline Registry<LearningStrategy>::Entry multiPointSameValueEntry()
    {
        return { "MultiPointSameValue", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"point_rate", ParamKind::Real, 0.0, 1.0, false, 0.05, 0.3} } }, sizeof(MultiPointSameValue),   // 每一维被选中的概率
            [](const double* p) -> LearningStrategy* { return p ? new MultiPointSameValue(p[0]) : new MultiPointSameValue(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    ECFLOW_REGISTER(lstrat_multipointsamevalue, LearningStrategy, multiPointSameValueEntry());

    inline Registry<LearningStrategy>::Entry exchangeMutationEntry()
    {
        return { "Exchange", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"times", ParamKind::Int, 1, 0x3f3f3f3f, false, 1, 3}, {"mutation_rate", ParamKind::Real, 0.0, 1.0, false, 0.01, 1.0} } }, sizeof(ExchangeMutation),
            [](const double* p) -> LearningStrategy* { return p ? new ExchangeMutation(int(p[0]), p[1]) : new ExchangeMutation(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    inline Registry<LearningStrategy>::Entry pmMutationEntry()
    {
        return { "PM", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"eta", ParamKind::Real, 0.0, 100.0, false, 20, 20}, {"mutation_rate", ParamKind::Real, 0.0, 1.0, false, 0.01, 0.1} } }, sizeof(PM_Mutation),
            [](const double* p) -> LearningStrategy* { return p ? new PM_Mutation(p[0], p[1]) : new PM_Mutation(); },
            [](AssertList&, const double*) {}, [](AssertList& L, const double* p) { PM_Mutation::postAssert(L, const_cast<double*>(p)); } };
    }
    inline Registry<LearningStrategy>::Entry gaussMutationEntry()
    {
        return { "Gauss", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"sigma", ParamKind::Real, 0.0, 10.0, false, 0.1, 1.0}, {"mutation_rate", ParamKind::Real, 0.0, 1.0, false, 0.01, 0.1} } }, sizeof(GaussMutation),
            [](const double* p) -> LearningStrategy* { return p ? new GaussMutation(p[0], p[1]) : new GaussMutation(); },
            [](AssertList&, const double*) {}, [](AssertList& L, const double* p) { GaussMutation::postAssert(L, const_cast<double*>(p)); } };
    }
    inline Registry<LearningStrategy>::Entry insertMutationEntry()
    {
        return { "Insert", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"times", ParamKind::Int, 1, 0x3f3f3f3f, false, 1, 3}, {"mutation_rate", ParamKind::Real, 0.0, 1.0, false, 0.01, 0.2} } }, sizeof(InsertMutation),
            [](const double* p) -> LearningStrategy* { return p ? new InsertMutation(int(p[0]), p[1]) : new InsertMutation(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    inline Registry<LearningStrategy>::Entry reorderMutationEntry()
    {
        return { "Reorder", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"times", ParamKind::Int, 1, 0x3f3f3f3f, false, 1, 3}, {"mutation_rate", ParamKind::Real, 0.0, 1.0, false, 0.01, 0.2} } }, sizeof(ReorderMutation),
            [](const double* p) -> LearningStrategy* { return p ? new ReorderMutation(int(p[0]), p[1]) : new ReorderMutation(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    inline Registry<LearningStrategy>::Entry segmentRelocateEntry()
    {
        return { "SegmentRelocate", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"times", ParamKind::Int, 1, 0x3f3f3f3f, false, 1, 3}, {"mutation_rate", ParamKind::Real, 0.0, 1.0, false, 0.01, 0.2} } }, sizeof(SegmentRelocate),
            [](const double* p) -> LearningStrategy* { return p ? new SegmentRelocate(int(p[0]), p[1]) : new SegmentRelocate(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }

    ECFLOW_REGISTER(lstrat_mut_bit,      LearningStrategy, bitMutationEntry());
    ECFLOW_REGISTER(lstrat_mut_bitflip,  LearningStrategy, bitFlipMutationEntry());
    ECFLOW_REGISTER(lstrat_mut_overturn, LearningStrategy, overturnMutationEntry());
    ECFLOW_REGISTER(lstrat_mut_exchange, LearningStrategy, exchangeMutationEntry());
    ECFLOW_REGISTER(lstrat_mut_pm,       LearningStrategy, pmMutationEntry());
    ECFLOW_REGISTER(lstrat_mut_gauss,    LearningStrategy, gaussMutationEntry());
    ECFLOW_REGISTER(lstrat_mut_insert,   LearningStrategy, insertMutationEntry());
    ECFLOW_REGISTER(lstrat_mut_reorder,  LearningStrategy, reorderMutationEntry());
    ECFLOW_REGISTER(lstrat_mut_segrelocate, LearningStrategy, segmentRelocateEntry());
}
