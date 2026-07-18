//------------------------Description------------------------
//   均以现有/新增算子**纯配置**构筑(不单独造局搜组件)。用 v3.10 同款流程(ConfigBuilder/WorkflowSetter
//   fluent)→ ConfigBuilder::saveConfig 落 v3.1 文本配置表 → config/<name>.cfg,并 save→load→
//   round-trip 无损 + AssertMatcher 静态自检。随局搜推进逐步扩充(当前:爬山、模拟退火)。
//-------------------------Reference-------------------------
// 局搜落位见 v3.11 讨论:爬山=Index{better_replace=1}(贪婪接受,现成),邻域=现有变异算子(rate=1 每代必移动)。
//-----------------------------------------------------------

#include <cstdio>
#include <string>
#include <vector>
#include <fstream>
#include <functional>
#include <filesystem>
#include "ecflow.h"
#include "config-setter.h"
#include "config-codec.h"
#include "assert-matcher.h"
using namespace ECFlow;

static const std::string OUTDIR = "config";
static const int MAXFES = 100000;
static const int SIZE = 30;

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
        std::printf("[ERR ] %-24s%s errors=%zu: %s\n", name.c_str(), rt, vr.errors.size(),
            vr.errors.empty() ? "" : vr.errors.front().c_str());
    } else if (!vr.warnings.empty()) {
        g_warn++;
        std::printf("[WARN] %-24s warnings=%zu: %s\n", name.c_str(), vr.warnings.size(), vr.warnings.front().c_str());
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
    std::printf("生成局部搜索 v3.1 配置表 → %s/ (当前:爬山、模拟退火)\n\n", OUTDIR.c_str());

    // ============ 爬山(贪婪接受 Index{better_replace=1}) × 邻域算子 ============
    // 排列/序列邻域(TSP 等)
    gen("hillclimb_2opt", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.isolate();
        w.repair.no();
        w.strategy.overturnMutation(1, 1);     // 2opt:随机段反转,每代必移动
        w.generator.generation();
        w.evaluator.basic();
        w.selector.index(true);                // 贪婪接受(只接受更优)
    });
    gen("hillclimb_1opt", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.isolate();
        w.repair.no();
        w.strategy.insertMutation(1, 1);       // 1opt:节点重定位
        w.generator.generation();
        w.evaluator.basic();
        w.selector.index(true);
    });
    gen("hillclimb_exchange", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.isolate();
        w.repair.no();
        w.strategy.exchangeMutation(1, 1);     // 对换两点
        w.generator.generation();
        w.evaluator.basic();
        w.selector.index(true);
    });
    gen("hillclimb_3opt", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.isolate(); w.repair.no();
        w.strategy.segmentRelocate(1, 1);   // 3opt 纯段平移(t4,Overturn 做不到的段搬迁);反转另配 Overturn
        w.generator.generation();
        w.evaluator.basic(); w.selector.index(true);
    });
    // 连续邻域(高斯扰动 + Boundary 夹回域界)
    gen("hillclimb_gauss", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.isolate();
        w.repair.boundary();
        w.strategy.gaussMutation(1.0, 1);      // 高斯小扰动,每代必移动
        w.generator.generation();
        w.evaluator.basic();
        w.selector.index(true);
    });

    // ============ 模拟退火(Metropolis 接受 indexAnneal:para=Index{2,T0,decay}) × 邻域算子 ============
    // 与爬山唯一差别:接受准则 selector.index(true)(贪婪) → selector.indexAnneal(T0,decay)(概率接受较差解,跳出局部最优)
    gen("anneal_2opt", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.isolate(); w.repair.no();
        w.strategy.overturnMutation(1, 1); w.generator.generation();
        w.evaluator.basic(); w.selector.indexAnneal(100.0, 0.95);   // 初温 100、每代衰减 0.95
    });
    gen("anneal_1opt", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.isolate(); w.repair.no();
        w.strategy.insertMutation(1, 1); w.generator.generation();
        w.evaluator.basic(); w.selector.indexAnneal(100.0, 0.95);
    });
    gen("anneal_exchange", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.isolate(); w.repair.no();
        w.strategy.exchangeMutation(1, 1); w.generator.generation();
        w.evaluator.basic(); w.selector.indexAnneal(100.0, 0.95);
    });
    gen("anneal_3opt", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.isolate(); w.repair.no();
        w.strategy.segmentRelocate(1, 1); w.generator.generation();
        w.evaluator.basic(); w.selector.indexAnneal(100.0, 0.95);
    });
    gen("anneal_gauss", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.isolate(); w.repair.boundary();
        w.strategy.gaussMutation(1.0, 1); w.generator.generation();
        w.evaluator.basic(); w.selector.indexAnneal(100.0, 0.95);
    });

    // ============ 禁忌搜索(落 strategy:生成侧记忆引导;selector.index(false)=接受非禁忌移动含较差) ============
    gen("tabu_2opt", SIZE, [](WorkflowSetter& w) {      // 装饰器,内核 Overturn(=2opt)
        w.initializer("Random"); w.topology.isolate(); w.repair.no();
        w.strategy.tabuDecorator(/*Overturn*/0, /*tenure*/10, { 1, 1 }); w.generator.generation();
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("tabu_relocate", SIZE, [](WorkflowSetter& w) {  // 装饰器,内核 SegmentRelocate
        w.initializer("Random"); w.topology.isolate(); w.repair.no();
        w.strategy.tabuDecorator(/*SegmentRelocate*/3, 10, { 1, 1 }); w.generator.generation();
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("tabu_bit", SIZE, [](WorkflowSetter& w) {   // 自带点变异邻域(主动选非禁忌维、取域内随机值,通用编码;二元下=翻转)
        w.initializer("Random"); w.topology.isolate(); w.repair.no();
        w.strategy.tabuBit(7); w.generator.generation();
        w.evaluator.basic(); w.selector.index(false);
    });

    // ============ 迭代贪婪(破坏-重建):destruct d 维 + 逐维贪婪重建(贪婪来自句柄) ============
    // 接受由 selector 区分:index(true)=仅更优才接受;indexAnneal=原文 RS 的常温 Metropolis 式接受变体。
    gen("ig_greedy", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.isolate(); w.repair.greedy();
        w.strategy.destructRebuild(4);      // 破坏 4 维 + 贪婪重建
        w.generator.orderedConstruct();     // 构造式:逐维推进约束
        w.evaluator.basic(); w.selector.index(true);
    });
    gen("ig_anneal", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.isolate(); w.repair.greedy();
        w.strategy.destructRebuild(4); w.generator.orderedConstruct();
        w.evaluator.basic(); w.selector.indexAnneal(100.0, 0.95);   // 接受层换成 Metropolis
    });

    // ============ 自适应算子选择(源自 ALNS):算子池 + 权重轮盘 + σ 评分 + 周期权重更新 ============
    // 接受交 selector:原文用 SA 式 → indexAnneal;也可配 index(true) 退化为"仅更优才接受"。
    gen("alns_anneal", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.isolate(); w.repair.no();
        w.strategy.adaptiveOperatorSelection(0.1, 20, { 0, 1, 2, 3 });   // 池:翻转/插入/交换/段平移
        w.generator.generation(); w.evaluator.basic();
        w.selector.indexAnneal(100.0, 0.95);                             // 原文 SA 式接受
    });
    gen("alns_ig", SIZE, [](WorkflowSetter& w) {           // 池含"破坏重建"(IG 大邻域) → 更贴原文 ALNS
        w.initializer("Random"); w.topology.isolate(); w.repair.greedy();
        w.strategy.adaptiveOperatorSelection(0.1, 20, { 0, 1, 6 });      // 池:翻转/插入/破坏重建
        w.generator.generation(); w.evaluator.basic();
        w.selector.indexAnneal(100.0, 0.95);
    });

    // ============ 路径重连(段追加到主算法末尾):每个体 × k 起点向当代最优采样路径点,血缘归属取每源最优 ============
    // 忠实要义:发起解→引导解的路径上取中间解。框架化为 **k 点采样式路径扫描**(逐子代随机深度 β),
    // 避开"算子内评估整条路径"(与评估分工冲突);扫描的择优由 kinshipGreedy 承担。
    gen("pathrelink_pso", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random"); w.repair.boundary();
        w.topology.pgBest(); w.strategy.velocityDriven(2, 2.0, 0.9, 0.0001);
        w.generator.orderedConstruct(); w.evaluator.basic(); w.selector.index(false);          // —— PSO 主段 ——
        w.topology.repeatWithBest(4); w.strategy.pathRelinking();
        w.generator.orderedConstruct(); w.evaluator.basic(); w.selector.kinshipGreedy();        // —— 路径重连段 ——
    });

    // ============ 精英局部搜索(段**追加到主算法末尾**;KINSHIP-ID 第二消费方) ============
    // 两条精英来源路径,差异在 select 层:
    //   ① 种群 top-e:`topIndividual`(起点=**种群槽**)+ 血缘定向 `kinshipGreedy`(读 parent_id 归属回精英槽、择优替换)
    //   ② 档案精英:`elite`(档案精英,内部 buffer,起点非种群槽)+ `rank`(全局归并,不走血缘)
    // 以 PSO 为主段示例(连续),精英段追加其后每代强化 top-e。repair 一次前置覆盖两段。
    gen("elite_pop_pso", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random"); w.repair.boundary();
        w.topology.pgBest(); w.strategy.velocityDriven(2, 2.0, 0.9, 0.0001);
        w.generator.orderedConstruct(); w.evaluator.basic(); w.selector.index(false);   // —— PSO 主段 ——
        w.topology.topIndividual(3, 3); w.strategy.differencePerturbation();
        w.generator.generation(); w.evaluator.basic(); w.selector.kinshipGreedy();       // —— 精英段:种群 top-e + 血缘定向替换 ——
    });
    gen("elite_archive_pso", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random"); w.repair.boundary();
        w.topology.pgBest(); w.strategy.velocityDriven(2, 2.0, 0.9, 0.0001);
        w.generator.orderedConstruct(); w.evaluator.basic(); w.selector.index(false);   // —— PSO 主段 ——
        w.topology.elite(3, 3); w.strategy.gaussMutation(1.0, 1);
        w.generator.generation(); w.evaluator.basic(); w.selector.rank();                // —— 精英段:档案精英 + rank 全局归并 ——
    });

    std::printf("\n===== 局搜配置(爬山+退火+禁忌+精英局搜):共 %d → %s/  |  OK=%d  WARN=%d  ERR=%d =====\n",
        g_total, OUTDIR.c_str(), g_ok, g_warn, g_err);
    return g_err ? 1 : 0;
}
