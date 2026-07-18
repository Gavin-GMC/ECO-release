//------------------------Description------------------------
// SETTER-API:fluent 配置构建。分类 setter(Strategy/Topology/Generator/Selector/Evaluator/Repair)
//   = 通用 `set(tag,para)` + 核心带参算子的**具名方法**(具名参数给语义提醒);上层 WorkflowSetter/
//   SubpopulationSetter/ConfigBuilder 装配 WorkflowConfig/SubpopulationConfig/OptimizerConfig。
//-------------------------Reference-------------------------
// 重设计自 ECFC原代码/setter-*.h(旧 enum-id+PARANUM → 新 string-tag;分类嵌套结构保留)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <list>
#include <string>
#include <vector>
#include <utility>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <cctype>
#include "configure-list.h"
#include "optimizer-builder.h"
#include "config-codec.h"        // ConfigCodecV31(文本)
#include "config-codec-json.h"   // ConfigCodecV32(JSON)

namespace ECFlow
{
    // 分类 setter 基:持 WorkflowConfig*,固定 c_type,通用 set()
    template <ModuleType CTYPE>
    struct CategorySetter
    {
        WorkflowConfig* _wf;
        CategorySetter(WorkflowConfig* wf = nullptr) : _wf(wf) {}
        void set(const std::string& tag, std::vector<double> para = {})
        {
            _wf->components.push_back(ComponentConfig(CTYPE, tag, std::move(para)));
        }
    };

    static inline double B(bool b) { return b ? 1.0 : 0.0; }

    struct StrategySetter : CategorySetter<ModuleType::T_learnstrategy>
    {
        using CategorySetter::CategorySetter;
        // PSO/EDA/ACO
        void velocityDriven(int object_number, double c, double w_ini, double w_attenuation, double vel_init_scale = 0.0) { set("VelocityDriven", { (double)object_number, c, w_ini, w_attenuation, vel_init_scale }); }   // vel_init_scale:0=速度零初始化;>0=值域内 ±k·域宽 随机
        void setVelocityDriven(int object_number, double c, double w_ini, bool v_heuristic, bool f_heuristic, double w_attenuation) { set("SetVelocityDriven", { (double)object_number, c, w_ini, v_heuristic ? 1.0 : 0.0, f_heuristic ? 1.0 : 0.0, w_attenuation }); }   // 修:原漏 v/f_heuristic(4→6 参)
        void binaryVelocityDriven(double c1, double c2, double w_ini, double w_attenuation) { set("BinaryVelocityDriven", { c1, c2, w_ini, w_attenuation }); }
        void stickyBinary(int c, int ratio_p, int ratio_g, double ustkS) { set("StickyBinary", { (double)c, (double)ratio_p, (double)ratio_g, ustkS }); }   // 粘性 BPSO(i_s=c/N,i_p:i_g,ustkS=8·maxfes/pop/100 手算)
        void distributionEstimation(int model, int good_number, double good_rate) { set("DistributionEstimation", { (double)model, (double)good_number, good_rate }); } // model:1 Gaussian/2 Cauchy/3 Uniform/4 Histogram
        void gaussianOneFifth(double sigma, double adapt) { set("GaussianOneFifth", { sigma, adapt }); }   // ES:1/5 成功规则(初始步长 sigma、调整因子 adapt)
        void gaussianSelfAdapt() { set("GaussianSelfAdapt"); }   // ES:self-adaptive σ(sigma 特性由装配期推断,individual="Individual")
        void cmaes(double sigma) { set("CMAES", { sigma }); }    // CMA-ES(初始步长 sigma;其余从 n/λ 自算)
        void greyWolfEncircling(double a_max = 2.0, double a_min = 0.0) { set("GreyWolfEncircling", { a_max, a_min }); }   // GWO(系数 a 衰减边界,配 TopRanked(3) 拓扑)
        void whaleForaging(double a_max = 2.0, double a_min = 0.0, double b = 1.0) { set("WhaleForaging", { a_max, a_min, b }); }   // WOA(a 衰减边界 + 螺旋常数 b,配 LeaderAndRandom 拓扑;whalestate 由装配期推断)
        void affinityHypermutation(double sigma = 0.3, double base_rate = 0.6, double rho = 4.0) { set("AffinityHypermutation", { sigma, base_rate, rho }); }   // 免疫超变异(门控率装饰器包 Gauss,亲和度反比变异率,配 ClonalExpansion)
        void randomGeneration() { set("RandomGeneration"); }   // 域内随机重采样(感受器编辑用,配 RandomSelect + WorstReplace)
        void explosionSpark(double amp_max = 5.0, double dim_rate = 0.5) { set("ExplosionSpark", { amp_max, dim_rate }); }   // FWA 爆炸火花(幅度∝适应度差,z 维均匀位移,配 FireworkExplosion)
        void gaussianSpark(double dim_rate = 0.5) { set("GaussianSpark", { dim_rate }); }   // FWA 高斯火花(每火花 g~N(1,1) 乘性,配 RandomSelect;fireworkstate 由装配期推断)
        // 禁忌搜索(落 strategy):装饰器包裹内部邻域(inner 枚举 0=Overturn/1=Insert/2=Exchange/3=SegmentRelocate/4=Reorder)
        // inner_para **无默认值,必须显式给** —— 5 个内层算子(Overturn/Insert/Exchange/SegmentRelocate/Reorder)
        //   全为 {times, mutation_rate} 两参,**没有零参的** → 缺省空 vector 永远是错的:工厂把 `p + 2` 交给内层
        //   算子,para 只有 2 个时内层读的是**越界内存**。原 `= {}` 使 tabuDecorator(0,7) 看着合法却必错,
        void tabuDecorator(int inner, int tenure, std::vector<double> inner_para) { std::vector<double> v{ (double)inner, (double)tenure }; for (double x : inner_para) v.push_back(x); set("TabuDecorator", std::move(v)); }
        void tabuBit(int tenure) { set("TabuBit", { (double)tenure }); }   // 自带点变异邻域禁忌(主动选非禁忌维、取域内随机值,通用编码)
        void antSystem(std::vector<double> para) { set("AntSystem", std::move(para)); }              // alpha,belta,rho,tao_ini,...
        void maxMinAntSystem(double alpha, double belta, double rho, double p_best, int use_global, int use_local) { set("MaxMinAntSystem", { alpha, belta, rho, p_best, (double)use_global, (double)use_local }); }   // MMAS(use_global/use_local 两开关,可同时开)
        void antColonySystem(std::vector<double> para) { set("AntColonySystem", std::move(para)); }  // alpha,belta,rho_g,rho_l,q0,use_global,use_local,tao_ini
        // 交叉
        void pointCrossover(int point_number, double cross_rate, bool coupled = true) { set("Point", { (double)point_number, cross_rate, B(coupled) }); }
        void uniformCrossover(double cross_rate, bool coupled = true) { set("Uniform", { cross_rate, B(coupled) }); }
        void sbxCrossover(double eta, double cross_rate, bool coupled = true) { set("SBX", { eta, cross_rate, B(coupled) }); }
        void differenceCrossover(double cross_rate, double factor, bool coupled = true) { set("Difference", { cross_rate, factor, B(coupled) }); }
        // ABC 邻域:v=x+φ(x−x_k),随机 1 维、φ∈[−1,1]、k=1 个同伴(拓扑供;employed=randomLearning(1)/onlooker=RouletteSource)
        void differencePerturbation() { set("DifferencePerturbation"); }
        void reinitialize() { set("Reinitialize"); }   // ABC scout:随机重生(配 AgeActivation 拓扑)
        void bestWorstGuided() { set("BestWorstGuided"); }   // 向最优靠拢+远离最差(源自 Jaya,无参数);配 bestAndWorst 拓扑 + index(true) 贪心接受
        void destructRebuild(int destruct_size = 4) { set("DestructRebuild", { (double)destruct_size }); }   // 破坏 d 维 + 逐维贪婪重建(贪婪来自句柄 getPrioriChoice);配 isolate + orderedConstruct + index(true)/indexAnneal 接受
        // —— 水波三算子(wavestate 特性 [h,λ] 由 WavePropagation 声明,Singular)——
        void wavePropagation(double h_max = 12, double alpha = 1.026, double lambda_init = 0.5) { set("WavePropagation", { h_max, alpha, lambda_init }); }   // 传播:x+rand(−1,1)·λ·L;配 isolate + index(true)
        void waveRefraction(double h_max = 12) { set("WaveRefraction", { h_max }); }   // 折射:N((x*+x)/2,|x*−x|/2);配 waveRefractionActivation + index(false)
        void waveBreaking(double beta_max = 0.25, double beta_min = 0.001) { set("WaveBreaking", { beta_max, beta_min }); }   // 碎波:随机 1 维 +N(0,1)·β·L;配 newBestActivation + kinshipGreedy
        void pathRelinking() { set("PathRelinking"); }   // 路径重连:差异维按随机深度 β 取引导值(k 点采样路径扫描);配 repeatWithBest(k) + kinshipGreedy
        // 自适应算子选择(源自 ALNS):池内算子按枚举列表配(0翻转/1插入/2交换/3段平移/4重排/5高斯/6破坏重建),一律默认参数;
        //   每代按权重轮盘选一个;σ 评分(33/9/13)+ 每 segment_len 代更新 w=w(1−ρ)+ρ(π/θ)。接受交 selector(原文 SA 式 → indexAnneal)
        void adaptiveOperatorSelection(double rho, int segment_len, std::vector<int> ops)
        {
            std::vector<double> para = { rho, (double)segment_len, (double)ops.size() };
            for (int o : ops) para.push_back((double)o);
            set("AdaptiveOperatorSelection", std::move(para));
        }
        void partialMappedCrossover(double cross_rate, bool coupled = true) { set("PartialMapped", { cross_rate, B(coupled) }); }
        void cycleCrossover(double cross_rate, bool coupled = true) { set("Cycle", { cross_rate, B(coupled) }); }
        void orderCrossover(double cross_rate, bool coupled = true) { set("Order", { cross_rate, B(coupled) }); }
        void subtourExchangeCrossover(double cross_rate, bool coupled = true) { set("SubtourExchange", { cross_rate, B(coupled) }); }
        void positionBasedCrossover(double cross_rate, double proportion, bool coupled = true) { set("PositionBased", { cross_rate, proportion, B(coupled) }); }
        // 变异
        void bitMutation(double mutation_rate) { set("Bit", { mutation_rate }); }
        void multiPointSameValue(double point_rate) { set("MultiPointSameValue", { point_rate }); }   // 多点同值:**逐维**以 point_rate 选中(同 Bit 语义),选中的维全置**同一个**域内随机值(整体型;与 Bit 的唯一区别=共用一个值)
        void cyclicExchange(int cycle_length, double mutation_rate = 1) { set("CyclicExchange", { (double)cycle_length, mutation_rate }); }   // 循环交换:k 个互异维的值环状轮转(k=2 即退化为 Exchange);值多重集守恒 → 排列编码天然保持可行
        void setExchange(double subset_rate = EMPTYVALUE) { set("SetExchange", { subset_rate }); }   // 集合交换:随机取两值 a/b,把"取值=a 的维"与"取值=b 的维"各随机取子集互换取值(值类层面成组迁移;rate=1 即两类整体对调);留空 → 免参模式,两侧各随机抽概率
        void blockSwap(int times = 1, double mutation_rate = 1) { set("BlockSwap", { (double)times, mutation_rate }); }   // 块互换:前|A|M|B|后 → A、B 互换(长度可不等、可相隔任意距离,M 不动);守恒。退化:M 空即 SegmentRelocate、|A|=|B|=1 即 Exchange
        void classMerge(double mutation_rate = 1) { set("ClassMerge", { mutation_rate }); }   // 值类合并:取值=a 的维整体并入另一现值 b → 类 a 消失,**在用值个数确定减 1**(整合/关停;目标取自现值故必被占用)
        void classRelocate(double subset_rate = EMPTYVALUE) { set("ClassRelocate", { subset_rate }); }   // 值类外迁:取值=a 的维随机取子集,整体迁到域内新抽的值 c(单向;c 未被使用 → 拆出新类/启用闲置值,c 已有 → 单向并入);留空 → 免参模式
        void bitFlipMutation(double mutation_rate) { set("BitFlip", { mutation_rate }); }
        void overturnMutation(int times, double mutation_rate) { set("Overturn", { (double)times, mutation_rate }); }
        void exchangeMutation(int times, double mutation_rate) { set("Exchange", { (double)times, mutation_rate }); }
        void pmMutation(double eta, double mutation_rate) { set("PM", { eta, mutation_rate }); }
        void gaussMutation(double sigma, double mutation_rate) { set("Gauss", { sigma, mutation_rate }); }
        void insertMutation(int times, double mutation_rate) { set("Insert", { (double)times, mutation_rate }); }
        void reorderMutation(int times, double mutation_rate) { set("Reorder", { (double)times, mutation_rate }); }
        void segmentRelocate(int times, double mutation_rate) { set("SegmentRelocate", { (double)times, mutation_rate }); }   // 3-opt 纯段平移(t4);反转另配 Overturn
    };

    struct TopologySetter : CategorySetter<ModuleType::T_learntopology>
    {
        using CategorySetter::CategorySetter;
        void pgBest() { set("PGBest"); }
        void randomLearning(int number) { set("RandomLearning", { (double)number }); }
        void isolate() { set("Isolate"); }
        void elite(int elite_number, int repeat_times) { set("Elite", { (double)elite_number, (double)repeat_times }); }
        void competition() { set("Competition"); }
        void levelBasedLearning(int level_number) { set("LevelBasedLearning", { (double)level_number }); }
        void stochasticDominantLearning() { set("StochasticDominantLearning"); }
        void continueGraph() { set("Continue"); }
        void inherit() { set("Inherit"); }
        void noChange() { set("NoChange"); }
        void roulette(int object_number) { set("Roulette", { (double)object_number }); }
        void rouletteSource() { set("RouletteSource"); }   // ABC onlooker:fit 轮盘有放回选源(起点)+ 1 随机同伴
        void ageActivation(int limit = 100) { set("AgeActivation", { (double)limit }); }   // ABC scout:老化(age>limit)激活自学、未老化空指针透传(配 Reinitialize + index 无条件)
        void topIndividual(int elite_number, int repeat_times) { set("TopIndividual", { (double)elite_number, (double)repeat_times }); }   // 精英局搜:种群 top-e 作起点×r(起点=种群槽,配 kinshipGreedy 归属替换)
        void bestAndWorst() { set("BestAndWorst"); }   // 两端:end[0]=当代最优、end[1]=当代最差(供"靠拢最优+远离最差"类策略)
        void waveRefractionActivation() { set("WaveRefractionActivation"); }   // 水波折射:h≤0 者激活(学习对象=档案最优),其余空指针透传
        void newBestActivation(int k_max = 12) { set("NewBestActivation", { (double)k_max }); }   // 水波碎波:自持上轮 elite 拷贝,仅出新最优才激活(最优个体 × k~U[1,k_max] 孤立波)
        void repeatWithBest(int repeat_times = 5) { set("RepeatWithBest", { (double)repeat_times }); }   // 每个体 × k 起点 + 引导=当代最优(最优自身跳过);配 pathRelinking + kinshipGreedy
        void tournament(int competition_scale, int object_number) { set("Tournament", { (double)competition_scale, (double)object_number }); }
        void uniform(int object_number) { set("Uniform", { (double)object_number }); }
        void topRanked(int leader_count) { set("TopRanked", { (double)leader_count }); }   // 通用前 K 名(GWO 用 3=α/β/δ)
        void leaderAndRandom() { set("LeaderAndRandom"); }   // 领袖(全局最优)+ 随机个体(WOA 用:end[0]=领袖,end[1]=随机鲸)
        void neighborhood(int T, double delta = 0.9) { set("Neighborhood", { (double)T, delta }); }   // MOEA/D 邻域交配(T 邻域大小、δ 邻域交配概率;配 ScalarReplace,T 须一致)
        void clonalExpansion(double clone_factor = 1.0) { set("ClonalExpansion", { clone_factor }); }   // 免疫克隆扩增(好抗体克隆更多,N_c∝亲和度排名)
        void randomSelect(int count) { set("RandomSelect", { (double)count }); }   // 随机选 d 个起点(感受器编辑:多产 d 个新解)
        void fireworkExplosion(double M = 40.0, double a = 0.04, double b = 0.8) { set("FireworkExplosion", { M, a, b }); }   // FWA 爆炸(火花数∝适应度好坏,截断 [aM,bM])
    };

    struct GeneratorSetter : CategorySetter<ModuleType::T_offspringgenerator>
    {
        using CategorySetter::CategorySetter;
        void generation(bool inplace = false) { set("Generation", { B(inplace) }); }
        void generationNoCheck(bool inplace = false) { set("GenerationNoCheck", { B(inplace) }); }
        void orderedConstruct(bool inplace = false) { set("OrderedConstruct", { B(inplace) }); }
        void parallelConstruct(bool inplace = false) { set("ParallelConstruct", { B(inplace) }); }
    };

    struct SelectorSetter : CategorySetter<ModuleType::T_selector>
    {
        using CategorySetter::CategorySetter;
        // accept_type: index/close 布尔=0 无条件/1 择优(爬山);indexAnneal/closeAnneal=2 退火(T0/decay 带默认初始参数)
        void index(bool better_replace = false) { set("Index", { B(better_replace) }); }
        void indexAnneal(double T0 = 100.0, double decay = 0.95) { set("Index", { 2, T0, decay }); }
        void indexAging() { set("Index", { 3 }); }   // Index + 年龄计龄接受(Better+trial;scout 独立为 AgeActivation 段)。age 特性自动挂载(Singular)
        void kinshipAging() { set("Kinship", { 3 }); }   // ABC onlooker:血缘定向 + 年龄计龄(读 parent_id 定位源槽)。kinship/age 自动挂载(Singular)
        void kinshipGreedy() { set("Kinship", { 1 }); }   // 精英局搜:血缘定向 + 择优接受(邻居优于精英→替换精英槽)。kinship 自动挂载(Singular)
        void rank() { set("Rank"); }
        void close(bool better_replace = false) { set("Close", { B(better_replace) }); }
        void closeAnneal(double T0 = 100.0, double decay = 0.95) { set("Close", { 2, T0, decay }); }
        void worstReplace() { set("WorstReplace"); }   // 无条件替换最差 λ 个(免疫感受器编辑/换血)
        void distanceSelect() { set("DistanceSelect"); }   // FWA 选择:留最优 + 距离密度 roulette 选(保多样性)
        void rankCrowding() { set("RankCrowding"); }   // 多目标:非支配 rank + 拥挤度截断(NSGA-II 环境选择,配 Multiobject 档案)
        void scalarReplace(int nr = 2, int T = 10) { set("ScalarReplace", { (double)nr, (double)T }); }   // MOEA/D 标量化替换(nr 每子代最多替换、T 邻域大小须与 Neighborhood 一致)
    };

    struct EvaluatorSetter : CategorySetter<ModuleType::T_evaluator>
    {
        using CategorySetter::CategorySetter;
        void basic() { set("Basic"); }
        void hash() { set("Hash"); }
    };

    struct RepairSetter : CategorySetter<ModuleType::T_Repair>
    {
        using CategorySetter::CategorySetter;
        void boundary() { set("Boundary"); }
        void modular() { set("Modular"); }   // 模映射:越界绕回域内(FWA 原文,x=L+|x| mod (U-L))
        void greedy() { set("Greedy"); }
        void random() { set("Random"); }
        void no() { set("No"); }
    };

    // workflow 设置:分类 setter 成员 + tag/initializer/通用 component
    class WorkflowSetter
    {
        WorkflowConfig* _wf;
    public:
        StrategySetter  strategy;
        TopologySetter  topology;
        GeneratorSetter generator;
        SelectorSetter  selector;
        EvaluatorSetter evaluator;
        RepairSetter    repair;

        WorkflowSetter(WorkflowConfig* wf)
            : _wf(wf), strategy(wf), topology(wf), generator(wf), selector(wf), evaluator(wf), repair(wf) {}

        void tag(const std::string& t) { _wf->tag = t; }
        void initializer(const std::string& tag, std::vector<double> para = {}) { _wf->ini_tag = tag; _wf->ini_para = std::move(para); }

        // 通用兜底(任意类型/tag,含未提供具名方法的算子)
        void component(ModuleType type, const std::string& tag, std::vector<double> para = {})
        {
            _wf->components.push_back(ComponentConfig(type, tag, std::move(para)));
        }
    };

    // 子群设置(链式)
    class SubpopulationSetter
    {
        SubpopulationConfig* _pc;
    public:
        SubpopulationSetter(SubpopulationConfig* pc) : _pc(pc) {}
        SubpopulationSetter& tag(const std::string& t) { _pc->tag = t; return *this; }
        SubpopulationSetter& size(int n) { _pc->size = n; return *this; }
        SubpopulationSetter& workflow(const std::string& t) { _pc->workflow_tag = t; return *this; }
        SubpopulationSetter& archive(const std::string& t, std::vector<double> para = {}) { _pc->archive_tag = t; _pc->archive_para = std::move(para); return *this; }
        SubpopulationSetter& maxFES(int fes) { _pc->terminate_conditions[0] = fes; return *this; }
        SubpopulationSetter& maxConvergence(int c) { _pc->terminate_conditions[1] = c; return *this; }
        SubpopulationSetter& maxTime(int s) { _pc->terminate_conditions[2] = s; return *this; }
    };

    // 顶层配置构建:workflow()/subpopulation() 返回子 setter;优化器/协作级链式;buildOptimizer 交付
    class ConfigBuilder
    {
        std::list<WorkflowConfig> _workflows;   // std::list:地址稳定,子 setter 持指针安全
        OptimizerConfig           _cfg;
        OptimizerBuilder          _builder;
    public:
        WorkflowSetter workflow(const std::string& tag)
        {
            _workflows.emplace_back();
            _workflows.back().tag = tag;
            return WorkflowSetter(&_workflows.back());
        }

        SubpopulationSetter subpopulation(const std::string& tag)
        {
            _cfg.subpopulations.emplace_back();
            _cfg.subpopulations.back().tag = tag;
            return SubpopulationSetter(&_cfg.subpopulations.back());
        }

        ConfigBuilder& name(const std::string& n) { _cfg.name = n; return *this; }
        ConfigBuilder& tag(const std::string& t) { _cfg.tag = t; return *this; }
        ConfigBuilder& maxFES(int fes) { _cfg.terminate_conditions[0] = fes; return *this; }
        ConfigBuilder& maxConvergence(int c) { _cfg.terminate_conditions[1] = c; return *this; }
        ConfigBuilder& maxTime(int s) { _cfg.terminate_conditions[2] = s; return *this; }
        ConfigBuilder& manager(const std::string& t, std::vector<double> para = {}) { _cfg.cooperation.manager_tag = t; _cfg.cooperation.manager_para = std::move(para); return *this; }
        ConfigBuilder& cooperationConstructer(const std::string& t, std::vector<double> para = {}) { _cfg.cooperation.constructer_tag = t; _cfg.cooperation.constructer_para = std::move(para); return *this; }
        ConfigBuilder& cooperationTopology(const std::string& t, std::vector<double> para = {}) { _cfg.cooperation.topology_tag = t; _cfg.cooperation.topology_para = std::move(para); return *this; }
        ConfigBuilder& gArchive(const std::string& t, std::vector<double> para = {}) { _cfg.g_archive_tag = t; _cfg.g_archive_para = std::move(para); return *this; }
        ConfigBuilder& logging(bool result, bool process = false, bool full_process = false) { _cfg.logger_full_result = result; _cfg.logger_process = process; _cfg.logger_full_process = full_process; return *this; }

        OptimizerConfig& config() { return _cfg; }

        // 注册所有 workflow 后装配 Optimizer
        Optimizer* buildOptimizer()
        {
            for (auto& wf : _workflows)
                _builder.registerWorkflow(wf);
            return _builder.buildOptimizer(_cfg);
        }

        // ======== 配置文件 save/load(CONFIG-IO,v1.4.5)========
        //   统一后缀 **.cfg**(纯文本);不同版本均 .cfg,**仅由首行版本分派**(不用后缀区分)。
        //   参考原 ecflow-builder 的 saveConfigure/loadConfigure 流程(格式/codec 已弃,仅流程)。
        //   静态、以 FullConfig 为界面 → ConfigBuilder(fluent) 与 config-generate(直填 FullConfig) 共用。

        // 首行版本探测:JSON('{' 开头)→ v3.2;文本 `<ver= vX.Y>` → 提取 vX.Y;不可识别 → 空串。
        static std::string detectVersion(const std::string& text)
        {
            size_t i = 0;
            while (i < text.size() && (std::isspace((unsigned char)text[i]) || (unsigned char)text[i] == 0xEF
                    || (unsigned char)text[i] == 0xBB || (unsigned char)text[i] == 0xBF)) i++;   // 跳空白/UTF-8 BOM
            if (i < text.size() && text[i] == '{') return "v3.2";        // JSON(ConfigCodecV32)
            size_t p = text.find("<ver=");                               // 文本头 <ver= vX.Y>
            if (p != std::string::npos)
            {
                size_t q = text.find('>', p);
                if (q != std::string::npos)
                {
                    std::string inner = text.substr(p + 5, q - (p + 5));
                    size_t a = inner.find_first_not_of(" \t");
                    size_t b = inner.find_last_not_of(" \t");
                    if (a != std::string::npos) return inner.substr(a, b - a + 1);
                }
            }
            return "";
        }

        // 按 ver 选 codec 编码 → 写 <dir>/<name>.cfg。ver 缺省 v3.1(文本);未知版本抛错。
        static void saveConfig(const FullConfig& fc, const std::string& name,
                               const std::string& ver = "v3.1", const std::string& dir = "config")
        {
            std::string text;
            if (ver == "v3.1")      text = ConfigCodecV31::encode(fc);
            else if (ver == "v3.2") text = ConfigCodecV32::encode(fc);
            else throw std::runtime_error("[config] unsupported version for saving: " + ver);

            std::filesystem::create_directories(dir);
            std::ofstream out(dir + "/" + name + ".cfg");
            if (!out) throw std::runtime_error("[config] cannot write: " + dir + "/" + name + ".cfg");
            out << text;
        }

        // 读 <dir>/<name>.cfg → 首行探测版本 → 对应 codec 解码 → FullConfig。未知/缺失抛错。
        static FullConfig loadConfig(const std::string& name, const std::string& dir = "config")
        {
            std::ifstream in(dir + "/" + name + ".cfg");
            if (!in) throw std::runtime_error("[config] file not found: " + dir + "/" + name + ".cfg");
            std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

            std::string ver = detectVersion(text);
            if (ver == "v3.1") return ConfigCodecV31::decode(text);
            if (ver == "v3.2") return ConfigCodecV32::decode(text);
            throw std::runtime_error("[config] unsupported/undetected version in '" + name + ".cfg': '" + ver + "'");
        }
    };
}
