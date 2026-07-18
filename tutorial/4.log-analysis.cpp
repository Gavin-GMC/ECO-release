// 4.log-analysis.cpp — 日志两轴 + 结果分析（ECAnalyzer）。输出英文(纪律 6)。
//   轴 A: 系统级 sys_logger + 每优化器 Logger(结果.rslt / 过程.prcs)。
//   分析: 跑两个 PSO 变体 × sphere(各 REPEATS 次重复,开结果日志) → ECAnalyzer 读 .rslt
//         → 归约/聚合/比较 → 导出 Txt/Csv/LaTeX 对比表(落 ./_analysis/)。
//   构建: build.ps1 tutorial\4.log-analysis.cpp     日志落 ./_log/、分析表落 ./_analysis/。
//   详见 手册 12-使用-结果分析.md。
#include <cstdio>
#include <fstream>
#include <string>
#include "ecflow.h"
#include "ec-analyzer.h"        // 分析层不在 ecflow.h 内,需单独引入
using namespace ECFlow;

static double f_sphere(double** a) { double s = 0; for (int i = 0; i < 5; i++) s += a[0][i] * a[0][i]; return s; }

// 跑一个 PSO 变体 name(惯性 w),开结果日志,重复 repeats 次。
// tag 显式设为 "sphere" → .rslt 落 _log/<name>/sphere/<name>_sphere(sphere)_<0..repeats-1>.rslt
static void run_variant(const std::string& name, double w, int repeats, int POP, int FES)
{
    Problem p("sphere");
    p.addVariable("x", -5.0, 5.0, 0.01, 5);
    p.addObjective("f", 1, true, "x", f_sphere);

    ConfigBuilder cb;
    auto wf = cb.workflow("pso_wf");
    wf.initializer("Random"); wf.topology.pgBest(); wf.repair.boundary();
    wf.strategy.velocityDriven(2, 2.0, w, 0.5 * POP / FES);
    wf.generator.orderedConstruct(); wf.evaluator.basic(); wf.selector.index(false);

    cb.name(name).tag("sphere").maxFES(FES).manager("Single").gArchive("Basic");
    cb.subpopulation("1").size(POP).workflow("pso_wf").maxFES(FES).archive("Basic");
    cb.logging(/*result*/true, /*process*/true);      // 开 .rslt(+.prcs)

    Optimizer* opt = cb.buildOptimizer();
    opt->setProblem(&p);
    opt->exe(repeats, 42);      // 播种一次,跑 repeats 次,每次自动 logResult → _0..repeats-1.rslt
    delete opt;
}

int main()
{
    const int REPEATS = 10, POP = 20, FES = 20000;

    sys_logger.info("tutorial 4: running two PSO variants on sphere");   // 系统级诊断轴

    // ---- 1. 跑实验(两个变体,开结果日志) ----
    run_variant("pso_high", 0.9, REPEATS, POP, FES);   // 高惯性
    run_variant("pso_low",  0.5, REPEATS, POP, FES);   // 低惯性

    // ---- 2. 分析:读 .rslt → 对比表 ----
    ECAnalyzer az;
    az.addOptimizer("pso_high", "sphere", REPEATS)
      .addOptimizer("pso_low",  "sphere", REPEATS)
      .addProblem("sphere")
      .reduceBy(ECAnalyzer::Stat::Mean, 0)                    // ① 每次运行:取目标均值
      .addStatistic(ECAnalyzer::Stat::Mean)                   // ② 跨重复:均值
      .addStatistic(ECAnalyzer::Stat::Std)                    //          标准差
      .addStatistic(ECAnalyzer::Stat::Smallest)               //          最优
      .addComparison(ECAnalyzer::Compare::Significance)       // ③ Mann-Whitney U p 值
      .addComparison(ECAnalyzer::Compare::Rank,       ECAnalyzer::Stat::Mean, ECAnalyzer::Direction::MinIsBetter)
      .addComparison(ECAnalyzer::Compare::WinTieLose, ECAnalyzer::Stat::Mean, ECAnalyzer::Direction::MinIsBetter)
      .addComparison(ECAnalyzer::Compare::Best,       ECAnalyzer::Stat::Mean, ECAnalyzer::Direction::MinIsBetter)
      .run()
      .report(ECAnalyzer::Format::Txt,   "tutorial4")         // → _analysis/tutorial4.txt
      .report(ECAnalyzer::Format::Csv,   "tutorial4")
      .report(ECAnalyzer::Format::Latex, "tutorial4");        // → _analysis/tutorial4.tex

    std::printf("logs  -> ./_log/    (system*.log ; pso_high|pso_low / sphere / *.rslt)\n");
    std::printf("tables-> ./_analysis/tutorial4.{txt,csv,tex}\n\n");

    // 回显生成的文本表
    { std::ifstream f("_analysis/tutorial4.txt"); if (f) { std::string s((std::istreambuf_iterator<char>(f)), {}); std::printf("%s\n", s.c_str()); } }

    // 速记:
    //   日志两轴 —— 轴 A 范围(sys_logger / 每优化器 / 问题-运行级) · 轴 B 类型(info/error · .rslt · .prcs);详见 手册 22 + docs/日志使用规范.md。
    //   分析三阶段 —— reduceBy(①归约) → addStatistic(②聚合) → addComparison(③比较) → report;详见 手册 12。
    //   多目标质量指标(HV/GD/IGD)见 手册 12 §5;Excel/Word/Png 输出与 peakRatio 指标 v4 收尾前完成。
    std::printf("4.log-analysis tutorial: DONE\n");
    return 0;
}
