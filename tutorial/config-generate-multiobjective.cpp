//------------------------Description------------------------
//   → config/<name>.cfg(ConfigBuilder::saveConfig),并 save→load round-trip 无损 + AssertMatcher 静态自检。
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
static const int SIZE = 100;

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

// 多目标:全局/子群档案 = Multiobject(收集 Pareto 前沿)
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
    fc.optimizer.g_archive_tag = "Multiobject";

    SubpopulationConfig sp;
    sp.tag = "1"; sp.size = size;
    sp.workflow_tag = "wf"; sp.terminate_conditions[0] = MAXFES; sp.archive_tag = "Multiobject";
    fc.optimizer.subpopulations.push_back(sp);

    emit(name, fc);
}

int main()
{
    std::filesystem::create_directories(OUTDIR);
    std::printf("生成多目标 v3.1 配置表 → %s/ (NSGA-II)\n\n", OUTDIR.c_str());

    // ============ NSGA-II:GA(tournament + SBX/PM 两段) + RankCrowding 环境选择 + Multiobject 档案 ============
    gen("nsga2", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.tournament(2, 1);                       // 交配:二元锦标赛(简化:支配比较)
        w.repair.boundary();
        w.strategy.sbxCrossover(20, 1.0); w.generator.generation();                    // 段1:SBX 交叉
        w.topology.continueGraph(); w.strategy.pmMutation(20, 0.05); w.generator.generation(true); // 段2:PM 变异
        w.evaluator.basic();
        w.selector.rankCrowding();                         // 环境选择:非支配 rank + 拥挤度
    });

    // ============ MOEA/D:分解式(权重+邻域) + Neighborhood 交配 + ScalarReplace(Tchebycheff) + Multiobject ============
    gen("moead", SIZE, [](WorkflowSetter& w) {
        w.initializer("Random");
        w.topology.neighborhood(10, 0.9);                  // 邻域交配(T=10, δ=0.9)
        w.repair.boundary();
        w.strategy.sbxCrossover(20, 1.0); w.generator.generation();                    // 段1:SBX 交叉
        w.topology.continueGraph(); w.strategy.pmMutation(20, 0.05); w.generator.generation(true); // 段2:PM 变异
        w.evaluator.basic();
        w.selector.scalarReplace(2, 10);                   // 标量化替换(nr=2, T=10 与邻域一致)
    });

    std::printf("\n===== 多目标配置(NSGA-II + MOEA/D):共 %d → %s/  |  OK=%d  WARN=%d  ERR=%d =====\n",
        g_total, OUTDIR.c_str(), g_ok, g_warn, g_err);
    return g_err ? 1 : 0;
}
