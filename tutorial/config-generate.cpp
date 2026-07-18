//------------------------Description------------------------
//   重建稳定版 tutorial/5.config_generatre.cpp 的全 60 个经典优化器配置,各编码为 **v3.1 文本配置表**
//   经 ConfigBuilder::saveConfig 落盘到 config/<name>.cfg(每配置一份,可 ConfigBuilder::loadConfig 按名读回)。生成后即用 AssertMatcher
//   静态自检(encode→decode→round-trip 无损 + 组件兼容校验),末尾汇总。
//   用法:运行可执行文件,配置表落 ./config/<name>.cfg(OUTDIR 为相对路径,依运行 cwd)。
//-------------------------Reference-------------------------
// 移植自 archive/ECFC稳定版本/tutorial/5.config_generatre.cpp(旧 enum-id+PARANUM setter → 新 string-tag)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
//-----------------------------------------------------------

#include <cstdio>
#include <string>
#include <vector>
#include <fstream>
#include <functional>
#include <filesystem>
#include "ecflow.h"
#include "config-setter.h"    // WorkflowSetter
#include "config-codec.h"     // ConfigCodecV31
#include "assert-matcher.h"   // AssertMatcher / ValidateResult
using namespace ECFlow;

static const std::string OUTDIR = "config";
static const int MAXFES = 100000;     // 稳定版统一 1e5
static const double NA = EMPTYVALUE;  // 未设哨兵(NaN);编码为 "nan"

static int g_total = 0, g_ok = 0, g_warn = 0, g_err = 0;

// saveConfig → 写 <name>.cfg → loadConfig(文件级 round-trip 无损) → AssertMatcher 静态校验 → 报告
static void emit(const std::string& name, FullConfig& fc)
{
    g_total++;
    ConfigBuilder::saveConfig(fc, name, "v3.1", OUTDIR);      // 写 <OUTDIR>/<name>.cfg(统一 .cfg,首行版本分派)
    FullConfig d = ConfigBuilder::loadConfig(name, OUTDIR);   // 文件级 round-trip:读回再比对
    bool round_trip = (ConfigCodecV31::encode(d) == ConfigCodecV31::encode(fc));
    ValidateResult vr = AssertMatcher::validate(d.workflows[0]);

    const char* rt = round_trip ? "" : " [ROUND-TRIP!]";
    if (!vr.valid || !round_trip)
    {
        g_err++;
        std::printf("[ERR ] %-22s%s errors=%zu: %s\n", name.c_str(), rt, vr.errors.size(),
            vr.errors.empty() ? "" : vr.errors.front().c_str());
    }
    else if (!vr.warnings.empty())
    {
        g_warn++;
        std::printf("[WARN] %-22s warnings=%zu: %s\n", name.c_str(), vr.warnings.size(), vr.warnings.front().c_str());
    }
    else
    {
        g_ok++;
        std::printf("[OK  ] %s\n", name.c_str());
    }
}

// 装配公共外壳(单种群 / Single 管理器 / Basic 档案 / maxFES=1e5),workflow 组件由 fill 填,交 emit
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
    std::printf("生成 v3.1 配置表 → %s/ (仿稳定版 5.config_generatre.cpp,全 60 配置)\n\n", OUTDIR.c_str());

    // ============ GA:12 种交叉×变异算子组合(个体编码,Generation) ============
    gen("GA_point_bit", 100, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.tournament(2, 1);
        w.repair.random();
        w.strategy.pointCrossover(1, 0.9); w.generator.generation();   // 交叉 + 生成
        w.topology.continueGraph(); w.strategy.bitMutation(0.01); w.generator.generation(true);   // 变异:续接交叉产物、原地串联
        w.evaluator.basic();
        w.selector.index(false);
    });
    gen("GA_SBX_PM", 100, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.tournament(2, 1);
        w.repair.boundary();
        w.strategy.sbxCrossover(20, 1); w.generator.generation();
        w.topology.continueGraph(); w.strategy.pmMutation(20, 0.01); w.generator.generation(true);   // 变异:续接交叉产物、原地串联
        w.evaluator.basic();
        w.selector.rank();
    });
    gen("GA_no_turnover", 100, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.tournament(2, 1);
        w.repair.random();
        w.strategy.overturnMutation(1, 1); w.generator.generation();   // 无交叉,仅变异
        w.evaluator.basic();
        w.selector.index(false);
    });
    gen("GA_uniform_gauss", 100, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.tournament(2, 1);
        w.repair.random();
        w.strategy.uniformCrossover(0.9); w.generator.generation();
        w.topology.continueGraph(); w.strategy.gaussMutation(3, 0.01); w.generator.generation(true);   // 变异:续接交叉产物、原地串联
        w.evaluator.basic();
        w.selector.index(false);
    });
    gen("GA_uniform_turnover", 100, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.tournament(2, 1);
        w.repair.random();
        w.strategy.uniformCrossover(0.9); w.generator.generation();
        w.topology.continueGraph(); w.strategy.overturnMutation(1, 1); w.generator.generation(true);   // 变异:续接交叉产物、原地串联
        w.evaluator.basic();
        w.selector.index(false);
    });
    gen("GA_no_exchange", 100, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.tournament(2, 1);
        w.repair.random();
        w.strategy.exchangeMutation(5, 1); w.generator.generation();
        w.evaluator.basic();
        w.selector.index(false);
    });
    gen("GA_partialmap_exchange", 100, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.tournament(2, 1);
        w.repair.random();
        w.strategy.partialMappedCrossover(0.9); w.generator.generation();
        w.topology.continueGraph(); w.strategy.exchangeMutation(5, 0.1); w.generator.generation(true);   // 变异:续接交叉产物、原地串联
        w.evaluator.basic();
        w.selector.index(false);
    });
    gen("GA_cycle_insert", 100, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.tournament(2, 1);
        w.repair.random();
        w.strategy.cycleCrossover(0.9); w.generator.generation();
        w.topology.continueGraph(); w.strategy.insertMutation(5, 0.1); w.generator.generation(true);   // 变异:续接交叉产物、原地串联
        w.evaluator.basic();
        w.selector.index(false);
    });
    gen("GA_order_turnover", 100, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.tournament(2, 1);
        w.repair.random();
        w.strategy.orderCrossover(0.9); w.generator.generation();
        w.topology.continueGraph(); w.strategy.overturnMutation(5, 0.1); w.generator.generation(true);   // 变异:续接交叉产物、原地串联
        w.evaluator.basic();
        w.selector.index(false);
    });
    gen("GA_subtourExchange_insert", 100, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.tournament(2, 1);
        w.repair.random();
        w.strategy.subtourExchangeCrossover(0.9); w.generator.generation();
        w.topology.continueGraph(); w.strategy.insertMutation(5, 0.1); w.generator.generation(true);   // 变异:续接交叉产物、原地串联
        w.evaluator.basic();
        w.selector.index(false);
    });
    gen("GA_subtourExchange_no", 100, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.tournament(2, 1);
        w.repair.random();
        w.strategy.subtourExchangeCrossover(0.9); w.generator.generation();   // 无变异,仅交叉
        w.evaluator.basic();
        w.selector.index(false);
    });
    gen("GA_positionBased_exchange", 100, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.tournament(2, 1);
        w.repair.random();
        w.strategy.positionBasedCrossover(0.9, 0.1); w.generator.generation();
        w.topology.continueGraph(); w.strategy.exchangeMutation(5, 0.1); w.generator.generation(true);   // 变异:续接交叉产物、原地串联
        w.evaluator.basic();
        w.selector.index(false);
    });

    // ============ 经典范式:EDA / DE / ACO / PSO 族 ============
    gen("EDA", 100, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.isolate();
        w.repair.random();
        w.strategy.set("DistributionEstimation", { 1, NA, 0.5 });   // model=Gaussian(1), good_number 缺省, good_rate=0.5
        w.generator.generation();
        w.evaluator.basic();
        w.selector.index(false);
    });
    gen("DE", 100, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.randomLearning(2);
        w.repair.random();
        w.strategy.differenceCrossover(0.5, 0.5); w.generator.generation();
        w.evaluator.basic();
        w.selector.index(false);
    });
    gen("AS", 10, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.isolate();
        w.repair.random();
        w.strategy.set("AntSystem", { 1, 2, 0.5, NA });   // alpha,belta,rho, tao_ini 缺省
        w.generator.orderedConstruct();
        w.evaluator.basic();
        w.selector.index(false);
    });
    gen("ACS", 10, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.isolate();
        w.repair.random();
        w.strategy.set("AntColonySystem", { 1, 2, 0.1, 0.1, 0.9, 1, 1, NA });   // alpha,belta,rho_g,rho_l,q0,use_global,use_local, tao_ini 缺省
        w.generator.parallelConstruct();
        w.evaluator.basic();
        w.selector.index(false);
    });
    gen("PSO", 100, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.pgBest();
        w.repair.random();
        w.strategy.velocityDriven(2, 2, 0.9, 0.5 * 100 / MAXFES);
        w.generator.orderedConstruct();
        w.evaluator.basic();
        w.selector.index(false);
    });
    gen("CSO", 100, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.competition();
        w.repair.boundary();
        // 稳定版用双 c 值 c[2]={1,0.1};新注册项 create 仅单 c → 用 c=1 近似(见文件头限制说明)
        w.strategy.velocityDriven(2, 1, 0.9, NA);
        w.generator.orderedConstruct();
        w.evaluator.basic();
        w.selector.index(false);
    });
    gen("LLSO", 100, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.levelBasedLearning(4);
        w.repair.random();
        w.strategy.velocityDriven(2, 2, 0.9, NA);
        w.generator.orderedConstruct();
        w.evaluator.basic();
        w.selector.index(true);
    });
    gen("SDLSO", 100, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.stochasticDominantLearning();
        w.repair.random();
        w.strategy.velocityDriven(2, 2, 0.9, NA);
        w.generator.orderedConstruct();
        w.evaluator.basic();
        w.selector.index(true);
    });
    gen("SPSO", 100, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.pgBest();
        w.repair.random();
        w.strategy.set("SetVelocityDriven", { 2, 2, 0.9, 1, 1, 0.5 * 100 / MAXFES });   // obj,c,w_ini, v_heuristic,f_heuristic, w_att
        w.generator.orderedConstruct();
        w.evaluator.basic();
        w.selector.index(false);
    });
    gen("SLLSO", 100, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.levelBasedLearning(4);
        w.repair.random();
        w.strategy.set("SetVelocityDriven", { 2, 2, 0.9, 1, 1, 0.5 * 100 / MAXFES });
        w.generator.orderedConstruct();
        w.evaluator.basic();
        w.selector.index(false);
    });
    gen("SSDLSO", 100, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.stochasticDominantLearning();
        w.repair.random();
        w.strategy.set("SetVelocityDriven", { 2, 2, 0.9, 1, 1, 0.5 * 100 / MAXFES });
        w.generator.orderedConstruct();
        w.evaluator.basic();
        w.selector.index(false);
    });

    // ============ GA 参数微调变体(_1/_2/_3) ============
    gen("GA_no_turnover_1", 100, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.roulette(1); w.repair.random();
        w.strategy.overturnMutation(1, 1); w.generator.generation();
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("GA_no_turnover_2", 100, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.roulette(1); w.repair.random();
        w.strategy.overturnMutation(2, 1); w.generator.generation();
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("GA_no_turnover_3", 100, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.roulette(1); w.repair.greedy();
        w.strategy.overturnMutation(5, 1); w.generator.generation();
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("GA_uniform_turnover_1", 100, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.tournament(2, 1); w.repair.greedy();
        w.strategy.uniformCrossover(0.01); w.generator.generation();
        w.topology.continueGraph(); w.strategy.overturnMutation(1, 1); w.generator.generation(true);   // 变异:续接交叉产物、原地串联
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("GA_uniform_turnover_2", 100, [](WorkflowSetter& w) {
        w.initializer("Greedy"); w.topology.tournament(2, 1); w.repair.greedy();
        w.strategy.uniformCrossover(0.01); w.generator.generation();
        w.topology.continueGraph(); w.strategy.overturnMutation(1, 1); w.generator.generation(true);   // 变异:续接交叉产物、原地串联
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("GA_uniform_turnover_3", 100, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.tournament(2, 1); w.repair.greedy();
        w.strategy.uniformCrossover(0.01); w.generator.generation();
        w.topology.continueGraph(); w.strategy.overturnMutation(1, 1); w.generator.generation(true);   // 变异:续接交叉产物、原地串联
        w.evaluator.basic(); w.selector.index(true);
    });
    gen("GA_partialmap_exchange_1", 100, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.tournament(2, 1); w.repair.random();
        w.strategy.partialMappedCrossover(0.9, false); w.generator.generation();
        w.topology.continueGraph(); w.strategy.exchangeMutation(5, 0.1); w.generator.generation(true);   // 变异:续接交叉产物、原地串联
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("GA_cycle_insert_1", 100, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.tournament(2, 1); w.repair.random();
        w.strategy.cycleCrossover(0.9); w.generator.generation();
        w.topology.continueGraph(); w.strategy.insertMutation(1, 1); w.generator.generation(true);   // 变异:续接交叉产物、原地串联
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("GA_order_turnover_1", 100, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.tournament(2, 1); w.repair.random();
        w.strategy.orderCrossover(0.9); w.generator.generation();
        w.topology.continueGraph(); w.strategy.overturnMutation(1, 0.01); w.generator.generation(true);   // 变异:续接交叉产物、原地串联
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("GA_positionBased_exchange_1", 100, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.tournament(2, 1); w.repair.random();
        w.strategy.positionBasedCrossover(0.9, 0.01); w.generator.generation();
        w.topology.continueGraph(); w.strategy.exchangeMutation(5, 0.1); w.generator.generation(true);   // 变异:续接交叉产物、原地串联
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("GA_point_bit_1", 100, [](WorkflowSetter& w) {
        w.initializer("Greedy"); w.topology.tournament(2, 1); w.repair.random();
        w.strategy.pointCrossover(1, 0.9); w.generator.generation();
        w.topology.continueGraph(); w.strategy.bitMutation(0.01); w.generator.generation(true);   // 变异:续接交叉产物、原地串联
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("GA_point_bit_2", 100, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.tournament(2, 1); w.repair.greedy();
        w.strategy.pointCrossover(1, 0.9); w.generator.generation();
        w.topology.continueGraph(); w.strategy.bitMutation(0.01); w.generator.generation(true);   // 变异:续接交叉产物、原地串联
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("GA_point_bit_3", 100, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.tournament(2, 1); w.repair.greedy();
        w.strategy.pointCrossover(1, 0.9); w.generator.generation();
        w.topology.continueGraph(); w.strategy.bitMutation(0.01); w.generator.generation(true);   // 变异:续接交叉产物、原地串联
        w.evaluator.basic(); w.selector.rank();
    });
    gen("GA_SBX_PM_1", 100, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.tournament(2, 1); w.repair.greedy();
        w.strategy.sbxCrossover(1, 0.9); w.generator.generation();
        w.topology.continueGraph(); w.strategy.pmMutation(20, 0.01); w.generator.generation(true);   // 变异:续接交叉产物、原地串联
        w.evaluator.basic(); w.selector.index(true);
    });

    // ============ ACO / SetPSO 参数微调变体 ============
    gen("AS_1", 10, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.isolate(); w.repair.random();
        w.strategy.set("AntSystem", { 1, 1, 0.5, NA });
        w.generator.parallelConstruct();
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("ACS_1", 10, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.isolate(); w.repair.random();
        w.strategy.set("AntColonySystem", { 1, 2, 0.5, 0.5, 0.9, 1, 1, NA });
        w.generator.parallelConstruct();
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("ACS_2", 10, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.isolate(); w.repair.random();
        w.strategy.set("AntColonySystem", { 1, 2, 0.1, 0.1, 0.9, 1, 1, NA });
        w.generator.orderedConstruct();
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("ACS_3", 10, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.isolate(); w.repair.random();
        w.strategy.set("AntColonySystem", { 2, 1, 0.1, 0.1, 0.9, 1, 1, NA });
        w.generator.parallelConstruct();
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("SPSO_1", 100, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.pgBest(); w.repair.random();
        w.strategy.set("SetVelocityDriven", { 2, 2, 0.9, 1, 0, 0.5 * 100 / MAXFES });
        w.generator.orderedConstruct();
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("SPSO_2", 100, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.pgBest(); w.repair.random();
        w.strategy.set("SetVelocityDriven", { 2, 2, 0.9, 0, 1, 0.5 * 100 / MAXFES });
        w.generator.orderedConstruct();
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("SPSO_3", 100, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.pgBest(); w.repair.random();
        w.strategy.set("SetVelocityDriven", { 2, 2, 0.9, 0, 0, 0.5 * 100 / MAXFES });
        w.generator.orderedConstruct();
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("SLLSO_1", 100, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.levelBasedLearning(4); w.repair.random();
        w.strategy.set("SetVelocityDriven", { 2, 2, 0.9, 1, 1, 0.5 * 100 / MAXFES });
        w.generator.orderedConstruct();
        w.evaluator.basic(); w.selector.index(true);
    });
    gen("SLLSO_2", 100, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.levelBasedLearning(4); w.repair.random();
        w.strategy.set("SetVelocityDriven", { 2, 2, 0.9, 1, 1, NA });
        w.generator.orderedConstruct();
        w.evaluator.basic(); w.selector.index(true);
    });
    gen("SLLSO_3", 100, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.levelBasedLearning(10); w.repair.random();
        w.strategy.set("SetVelocityDriven", { 2, 2, 0.9, 1, 1, NA });
        w.generator.orderedConstruct();
        w.evaluator.basic(); w.selector.rank();
    });
    gen("SLLSO_4", 100, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.levelBasedLearning(4); w.repair.random();
        w.strategy.set("SetVelocityDriven", { 2, 2, 0.9, 1, 1, NA });
        w.generator.orderedConstruct();
        w.evaluator.basic(); w.selector.index(true);
    });
    gen("SSDLSO_1", 100, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.stochasticDominantLearning(); w.repair.random();
        w.strategy.set("SetVelocityDriven", { 2, 2, 0.9, 1, 1, NA });
        w.generator.orderedConstruct();
        w.evaluator.basic(); w.selector.index(true);
    });

    // ============ 拓扑×算法 组合配置 ============
    gen("CS+GA", 100, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.competition(); w.repair.greedy();
        w.strategy.uniformCrossover(0.01); w.generator.generation();
        w.topology.continueGraph(); w.strategy.overturnMutation(1, 1); w.generator.generation(true);   // 变异:续接交叉产物、原地串联
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("LL+GA", 100, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.levelBasedLearning(4); w.repair.greedy();
        w.strategy.cycleCrossover(0.9); w.generator.generation();
        w.topology.continueGraph(); w.strategy.insertMutation(5, 1); w.generator.generation(true);   // 变异:续接交叉产物、原地串联
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("SDL+GA", 100, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.stochasticDominantLearning(); w.repair.greedy();
        w.strategy.partialMappedCrossover(0.9, false); w.generator.generation();
        w.topology.continueGraph(); w.strategy.exchangeMutation(5, 0.1); w.generator.generation(true);   // 变异:续接交叉产物、原地串联
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("LL+ACS", 10, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.levelBasedLearning(4); w.repair.random();
        w.strategy.set("AntColonySystem", { 1, 2, 0.1, 0.1, 0.9, 1, 1, NA });
        w.generator.parallelConstruct();
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("random+SPSO", 100, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.randomLearning(2); w.repair.random();
        w.strategy.set("SetVelocityDriven", { 2, 2, 0.9, 1, 1, 0.5 * 100 / MAXFES });
        w.generator.orderedConstruct();
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("roulrtte+SPSO", 100, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.roulette(2); w.repair.random();
        w.strategy.set("SetVelocityDriven", { 2, 2, 0.9, 1, 1, 0.5 * 100 / MAXFES });
        w.generator.orderedConstruct();
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("championship+SPSO", 100, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.tournament(2, 2); w.repair.random();
        w.strategy.set("SetVelocityDriven", { 2, 2, 0.9, 1, 1, 0.5 * 100 / MAXFES });
        w.generator.orderedConstruct();
        w.evaluator.basic(); w.selector.index(false);
    });

    // ============ 随机组合示例(randomConfig_1..4) ============
    gen("randomConfig_1", 10, [](WorkflowSetter& w) {
        w.initializer("Random"); w.topology.levelBasedLearning(4); w.repair.greedy();
        w.strategy.set("AntColonySystem", { 1, 2, 0.1, 0.1, 0.9, 1, 1, NA });
        w.generator.generation();
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("randomConfig_2", 100, [](WorkflowSetter& w) {
        w.initializer("Greedy"); w.topology.stochasticDominantLearning(); w.repair.random();
        w.strategy.set("DistributionEstimation", { 1, NA, 0.2 });
        w.generator.orderedConstruct();
        w.evaluator.basic(); w.selector.index(false);
    });
    gen("randomConfig_3", 100, [](WorkflowSetter& w) {
        w.initializer("Greedy"); w.topology.tournament(2, 2); w.repair.random();
        w.strategy.velocityDriven(2, 2, 0.9, 0.5 * 100 / MAXFES);
        w.generator.generation();
        w.evaluator.basic(); w.selector.close(true);
    });
    gen("randomConfig_4", 100, [](WorkflowSetter& w) {
        w.initializer("Greedy"); w.topology.roulette(1); w.repair.random();
        w.strategy.set("DistributionEstimation", { 1, NA, 0.2 });
        w.generator.generationNoCheck();
        w.evaluator.basic(); w.selector.rank();
    });

    std::printf("\n===== 汇总:共 %d 个配置 → %s/  |  OK=%d  WARN=%d  ERR=%d =====\n",
        g_total, OUTDIR.c_str(), g_ok, g_warn, g_err);
    return g_err ? 1 : 0;
}
