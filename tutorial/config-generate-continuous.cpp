//------------------------Description------------------------
//   并 encode→decode round-trip 无损 + AssertMatcher 静态自检。

// SPDX-License-Identifier: BSD-3-Clause

#include <cstdio>
#include <string>
#include <vector>
#include <fstream>
#include <functional>
#include <filesystem>
#include "evoflow.h"
#include "config-setter.h"
#include "config-codec.h"
#include "assert-matcher.h"
using namespace EvoFlow;

static const std::string OUTDIR = "config";
static const int MAXFES = 100000;
static const int SIZE = 20;

static int g_total = 0, g_ok = 0, g_warn = 0, g_err = 0;

static void emit(const std::string& name, FullConfig& fc)
{
    g_total++;
    ConfigBuilder::saveConfig(fc, name, "v3.1", OUTDIR);      // 写 <OUTDIR>/<name>.cfg(统一 .cfg,首行版本分派)
    FullConfig d = ConfigBuilder::loadConfig(name, OUTDIR);   // 文件级 round-trip:读回再比对
    bool round_trip = (ConfigCodecV31::encode(d) == ConfigCodecV31::encode(fc));
    ValidateResult vr = AssertMatcher::validate(d.workflows[0]);

    const char* rt = round_trip ? "" : " [ROUND-TRIP!]";
    if (!vr.valid || !round_trip) {
        g_err++;
        std::printf("[ERR ] %-16s%s errors=%zu: %s\n", name.c_str(), rt, vr.errors.size(),
            vr.errors.empty() ? "" : vr.errors.front().c_str());
    } else if (!vr.warnings.empty()) {
        g_warn++;
        std::printf("[WARN] %-16s warnings=%zu: %s\n", name.c_str(), vr.warnings.size(), vr.warnings.front().c_str());
    } else {
        g_ok++;
        std::printf("[OK  ] %s\n", name.c_str());
    }
}

static void gen(const std::string& name, int size,
                std::function<void(WorkflowSetter&)> fill)
{
    FullConfig fc;
    WorkflowConfig wf; wf.tag = "wf";
    WorkflowSetter ws(&wf);
    fill(ws);
    fc.workflows.push_back(wf);

    fc.optimizer.name = name;
    fc.optimizer.terminate_conditions[0] = MAXFES;
    fc.optimizer.cooperation.manager_tag = "Single";
    fc.optimizer.g_archive_tag = "Basic";

    SubpopulationConfig sp;
    sp.tag = "1"; sp.size = size;
    sp.workflow_tag = "wf"; sp.terminate_conditions[0] = MAXFES; sp.archive_tag = "Basic";
    fc.optimizer.subpopulations.push_back(sp);

    emit(name, fc);
}

int main()
{
    std::filesystem::create_directories(OUTDIR);
    std::printf("生成连续优化 v3.1 配置表 → %s/ (ES + CMA-ES + Sticky BPSO + MMAS + EDA 新分布 + GWO + WOA + 免疫 + 烟花)\n\n", OUTDIR.c_str());

    // ============ ES(进化策略):高斯采样 + 步长自适应,selector=Rank((μ+λ)) ============
    gen("es_onefifth", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.isolate();
        w.repair.boundary();
        w.strategy.gaussianOneFifth(1.0, 0.85);   // 全局 σ + 1/5 成功规则
        w.generator.generation();
        w.evaluator.basic();
        w.selector.rank();
    });
    gen("es_selfadapt", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.isolate();
        w.repair.boundary();
        w.strategy.gaussianSelfAdapt();           // per-individual σ(sigma 特性由装配期推断提供)
        w.generator.generation();
        w.evaluator.basic();
        w.selector.rank();
    });

    // ============ CMA-ES:多元正态 N(m,σ²C) + 协方差自适应 ============
    gen("cmaes", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.isolate();
        w.repair.boundary();
        w.strategy.cmaes(0.5);                    // 初始步长 σ=0.5(其余从 n/λ 自算)
        w.generator.generation();
        w.evaluator.basic();
        w.selector.rank();
    });

    // ============ Sticky BPSO(二进制):粘性 stickiness + 直接翻转概率 ============
    // 二进制编码;individual=Individual(velocity 特性由装配期推断,复用存 stickiness);PGBest(pbest+gbest)。ustkS=8·maxfes/pop/100(此处按 100000/20 手算=400)。
    gen("sticky_bpso", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.pgBest();                      // pbest + gbest(2 学习目标)
        w.repair.no();
        w.strategy.stickyBinary(4, 1, 1, 400.0);  // i_s=4/N, i_p:i_g=1:1, ustkS=400
        w.generator.generation();
        w.evaluator.basic();
        w.selector.index(false);                  // PSO 式位置替换
    });

    // ============ MMAS(组合优化,ACO 族):信息素上下界 clamp + 释放模式(iter/global/both) ============
    // 排列/序列编码(TSP 等);Isolate(objects=0)、OrderedConstruct 逐维构造;use_global=1&use_local=1→当代+全局(archive)同时释放。
    gen("mmas", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.isolate();
        w.repair.random();
        w.strategy.maxMinAntSystem(1, 2, 0.1, 0.05, 1, 1);   // alpha,belta,rho,p_best,use_global,use_local(both)
        w.generator.orderedConstruct();
        w.evaluator.basic();
        w.selector.index(false);
    });

    // ============ EDA 新分布模型(v3.11.5):DistributionEstimation 的 model 从 Gaussian 扩到 Cauchy/Uniform/Histogram ============
    // 逐维单变量分布(objects=0,Isolate);good_number 缺省(NA)→按 good_rate 取好解;仅 model 不同。
    const char* eda_names[3] = { "eda_cauchy", "eda_uniform", "eda_histogram" };
    const int   eda_models[3] = { 2, 3, 4 };   // 2 Cauchy / 3 Uniform / 4 Histogram
    for (int e = 0; e < 3; ++e) {
        int model = eda_models[e];
        gen(eda_names[e], 100, [model](WorkflowSetter& w) {
            w.initializer("Random");
            w.topology.isolate();
            w.repair.random();
            w.strategy.set("DistributionEstimation", { (double)model, EMPTYVALUE, 0.5 });   // model,good_number 缺省,good_rate=0.5
            w.generator.generation();
            w.evaluator.basic();
            w.selector.index(false);
        });
    }

    // ============ GWO(灰狼优化,连续):TopRanked(3,α/β/δ) + 系数 a 随进度衰减 ============
    gen("gwo", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.topRanked(3);
        w.repair.boundary();
        w.strategy.greyWolfEncircling(2.0, 0.0);   // a_max=2, a_min=0(标准 GWO)
        w.generator.generation();
        w.evaluator.basic();
        w.selector.rank();
    });

    // ============ SCA(正弦余弦,连续):TopRanked(1,当代最优) + 振幅 r1 随进度衰减 ============
    gen("sca", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.topRanked(1);
        w.repair.boundary();
        w.strategy.sineCosineOscillation(2.0);   // a=2(标准 SCA)
        w.generator.generation();
        w.evaluator.basic();
        w.selector.rank();
    });

    // ============ AOA(四则运算,连续):TopRanked(1,当代最优) + MOA递增/MOP递减进度门控 ============
    gen("aoa", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.topRanked(1);
        w.repair.boundary();
        w.strategy.arithmeticOperatorUpdate(5.0, 0.2, 1.0, 0.5);   // 标准 AOA:alpha=5, MOA∈[0.2,1], mu=0.5
        w.generator.generation();
        w.evaluator.basic();
        w.selector.rank();
    });

    // ============ RSA(爬行动物搜索,连续):LeaderRandomCenter(最优+随机+中心) + 4 相位包围/狩猎 ============
    gen("rsa", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.leaderRandomCenter();
        w.repair.boundary();
        w.strategy.reptileEncircling(0.1, 0.1);   // 标准 RSA:alpha=beta=0.1
        w.generator.generation();
        w.evaluator.basic();
        w.selector.rank();
    });

    // ============ GJO(金豺优化,连续):TopRanked(2,雌雄双豺) + 能量 E 衰减 + Levy 飞行 ============
    gen("gjo", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.topRanked(2);
        w.repair.boundary();
        w.strategy.goldenJackalPincer(1.5, 1.5);   // 标准 GJO:c1=1.5, Levy beta=1.5
        w.generator.generation();
        w.evaluator.basic();
        w.selector.rank();
    });

    // ============ TSA(海鞘群,连续):TopRanked(1,食物源) + 喷射推进 + 群聚阻尼(无进度衰减) ============
    gen("tsa", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.topRanked(1);
        w.repair.boundary();
        w.strategy.tunicateJetSwarm(1.0, 4.0);   // 标准 TSA:Pmin=1, Pmax=4
        w.generator.generation();
        w.evaluator.basic();
        w.selector.rank();
    });

    // ============ MPA(海洋捕食者,连续):LeaderTwoRandom(最优+两随机) + 3 相位 Brownian/Levy + FADs ============
    gen("mpa", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.leaderTwoRandom();
        w.repair.boundary();
        w.strategy.marinePredatorMotion(0.5, 0.2);   // 标准 MPA:P=0.5, FADs=0.2
        w.generator.generation();
        w.evaluator.basic();
        w.selector.rank();
    });

    // ============ AO(天鹰优化,连续):LeaderRandomCenter(最优+随机+中心) + 4 method 相位切换 + Levy + 螺旋 ============
    gen("aquila", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.leaderRandomCenter();
        w.repair.boundary();
        w.strategy.aquilaHunt();   // 标准 AO(alpha/delta=0.1,螺旋常数默认)
        w.generator.generation();
        w.evaluator.basic();
        w.selector.rank();
    });

    // ============ SMA(黏菌,连续):LeaderTwoRandom(最优+两随机) + 适应度权重 W + 振荡 vb/vc ============
    gen("sma", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.leaderTwoRandom();
        w.repair.boundary();
        w.strategy.slimeMouldOscillation(0.03);   // 标准 SMA:z=0.03
        w.generator.generation();
        w.evaluator.basic();
        w.selector.rank();
    });

    // ============ EO(平衡优化器,连续):TopRanked(4,平衡池前 4) + 指数质量平衡 ============
    gen("eo", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.topRanked(4);
        w.repair.boundary();
        w.strategy.equilibriumUpdate();   // 标准 EO(a1=2, a2=1, GP=0.5, V=1)
        w.generator.generation();
        w.evaluator.basic();
        w.selector.rank();
    });

    // ============ AVOA(非洲秃鹫,连续):TopRanked(2,两领头) + 饱食率 F 分相位多分支 + 螺旋 + Levy ============
    gen("avoa", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.topRanked(2);
        w.repair.boundary();
        w.strategy.vultureForaging();   // 标准 AVOA(L1=0.8,P1/P2/P3=0.6/0.4/0.6,ω=2.5)
        w.generator.generation();
        w.evaluator.basic();
        w.selector.rank();
    });

    // ============ BWO(白鲸优化,连续):LeaderAndRandom(最优+随机) + 游动(跨维)/捕食(Levy)/鲸落 三相 ============
    gen("bwo", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.leaderAndRandom();
        w.repair.boundary();
        w.strategy.belugaWhalePhases();   // 标准 BWO(wf 0.1→0.05, B0∈[0,1])
        w.generator.generation();
        w.evaluator.basic();
        w.selector.rank();
    });

    // ============ INFO(加权均值向量,连续):两段——RankExtremesTriRandom+更新 → Continue…+局搜 ============
    gen("info", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.rankExtremesTriRandom();
        w.repair.boundary();
        w.strategy.weightedMeanUpdate(); w.generator.generation();                                              // 段A:更新规则+向量组合
        w.topology.continueRankExtremesTriRandom(); w.strategy.weightedMeanLocalSearch(); w.generator.generation(true);   // 段B:加权均值局搜(续接)
        w.evaluator.basic();
        w.selector.rank();
    });

    // ============ SSA(海鞘群,连续):SalpChain(食物+前一只) + 领导跟食物/跟随链式平均 ============
    gen("ssa", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.salpChain();
        w.repair.boundary();
        w.strategy.salpChainUpdate(2.0, 4.0);   // 标准 SSA:c1=2·e^(−(4p)²)
        w.generator.generation();
        w.evaluator.basic();
        w.selector.rank();
    });

    // ============ GOA(蝗虫优化,连续):TopRanked(1,目标) + 全群两两社会力 O(n²) ============
    gen("goa", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.topRanked(1);
        w.repair.boundary();
        w.strategy.grasshopperSwarm();   // 标准 GOA(f=0.5, l=1.5, c_max=1, c_min=4e-5)
        w.generator.generation();
        w.evaluator.basic();
        w.selector.rank();
    });

    // ============ DBO(蜣螂优化,连续):DungBeetleRefs(全局/当代最优+最差) + 四角色 ============
    gen("dbo", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.dungBeetleRefs();
        w.repair.boundary();
        w.strategy.dungBeetleRoles();   // 标准 DBO(k=.1,b=.3,S=.5,比例 .2/.2/.233/余)
        w.generator.generation();
        w.evaluator.basic();
        w.selector.rank();
    });

    // ============ WOA(鲸鱼优化,连续):LeaderAndRandom(领袖=全局最优 + 随机鲸) + 系数 a 随进度衰减 ============
    // individual=Individual(whalestate=p/A/C/l 4 标量由装配期推断);OrderedConstruct 逐维构造;selector=Index(无条件替换,遵原文,领袖由档案保留)。
    gen("woa", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.leaderAndRandom();
        w.repair.boundary();
        w.strategy.whaleForaging(2.0, 0.0, 1.0);   // a_max=2, a_min=0, b=1(标准 WOA)
        w.generator.orderedConstruct();            // 逐维构造
        w.evaluator.basic();
        w.selector.index(false);                   // 无条件替换(WOA 整代覆盖,全局最优由档案保留)
    });

    // ============ 免疫算法(克隆选择 CLONALG,连续):克隆扩增 + 亲和度反比超变异 + 感受器编辑,2 段 workflow ============
    // 段1:ClonalExpansion(好抗体克隆更多) + AffinityHypermutation(门控率装饰器,亲和度反比变异率) → Rank(克隆选择,精英)。
    // 段2:RandomSelect(4)+RandomGeneration(域内随机新生) → WorstReplace(无条件换掉最差 4,感受器编辑/换血保多样性)。
    gen("immune", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.clonalExpansion(1.0);      w.repair.boundary();
        w.strategy.affinityHypermutation(0.3, 0.6, 4.0); w.generator.generation();   // 段1:克隆 + 超变异
        w.evaluator.basic();  w.selector.rank();                                      //      克隆选择(精英)
        w.topology.randomSelect(4); w.strategy.randomGeneration(); w.generator.generation();  // 段2:注入 4 随机新生
        w.evaluator.basic();  w.selector.worstReplace();                             //      感受器编辑(强制换最差 4)
    });

    // ============ 烟花算法(FWA,连续):爆炸火花 + 高斯火花 + 距离选择,2 段 workflow + 模映射修复 ============
    // 段1:FireworkExplosion(好烟花产更多火花) + ExplosionSpark(幅度∝适应度差,均匀位移)。
    // 段2:RandomSelect(5)+GaussianSpark(每火花 g~N(1,1) 乘性,fireworkstate 由装配期推断)。
    // 越界=Modular(绕回域内);选择=DistanceSelect(留最优+距离密度选,保多样性)。烟花数少(n=5),火花池大。
    gen("firework", 5, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.fireworkExplosion(40.0, 0.04, 0.8);  w.repair.modular();
        w.strategy.explosionSpark(5.0, 0.5); w.generator.generation();          // 段1:爆炸火花
        w.topology.randomSelect(5); w.strategy.gaussianSpark(0.5); w.generator.generation();  // 段2:高斯火花
        w.evaluator.basic();  w.selector.distanceSelect();                      // 留最优 + 距离选择
    });
    // ABC 人工蜂群:三段/代(employed 均匀 1:1 / onlooker 轮盘有放回多对一血缘定向 / scout 老化重生独立段)
    gen("abc", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random"); w.repair.boundary();
        w.topology.randomLearning(1); w.strategy.differencePerturbation();
        w.generator.generation(); w.evaluator.basic(); w.selector.indexAging();       // 段1 employed:均匀 1:1 + 计龄
        w.topology.rouletteSource(); w.strategy.differencePerturbation();
        w.generator.generation(); w.evaluator.basic(); w.selector.kinshipAging();      // 段2 onlooker:轮盘多对一 + 血缘定向
        w.topology.ageActivation(30); w.strategy.reinitialize();
        w.generator.generation(); w.evaluator.basic(); w.selector.index(false);        // 段3 scout:老化重生 + 无条件替换
    });

    // Jaya:向最优靠拢 + 远离最差(无参数);两端拓扑 + 逐维构造 + 贪心接受(原文"仅更优才接受")
    gen("jaya", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.bestAndWorst(); w.repair.boundary();
        w.strategy.bestWorstGuided();          // 无算法专属参数
        w.generator.orderedConstruct(); w.evaluator.basic();
        w.selector.index(true);                // 贪心接受
    });

    // WWO 水波:三段/代 —— 传播(自扰动+波长) / 折射(h 降到 0 者,向档案最优折射) / 碎波(仅出新最优时,孤立波)
    gen("wwo", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random"); w.repair.boundary();
        w.topology.isolate(); w.strategy.wavePropagation(12, 1.026, 0.5);
        w.generator.orderedConstruct(); w.evaluator.basic(); w.selector.index(true);          // ① 传播
        w.topology.waveRefractionActivation(); w.strategy.waveRefraction(12);
        w.generator.orderedConstruct(); w.evaluator.basic(); w.selector.index(false);         // ② 折射(无条件替换)
        w.topology.newBestActivation(12); w.strategy.waveBreaking(0.25, 0.001);
        w.generator.generation(); w.evaluator.basic(); w.selector.kinshipGreedy();            // ③ 碎波(血缘归属)
    });

    std::printf("\n===== 新算子配置(ES + CMA-ES + Sticky BPSO + MMAS + EDA 新分布 + GWO + WOA + 免疫 + 烟花 + ABC + Jaya + WWO):共 %d → %s/  |  OK=%d  WARN=%d  ERR=%d =====\n",
        g_total, OUTDIR.c_str(), g_ok, g_warn, g_err);
    return g_err ? 1 : 0;
}
