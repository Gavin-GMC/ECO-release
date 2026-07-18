// 3.population-manager.cpp — 多子群协作用例（稳定版对应文件为空壳,此处按 v3.12 架构演示"种群管理"）
//   演示:Population 顶层聚合下,多个**异构**子种群(一群 PSO + 一群 GA)由 SubpopulationManager 协调、
//     全局档案汇集各子群最优。管理器=NoInteraction(各子群独立进化,全局仅汇集);另有 Immigrant(迁徙)等。
//   构建: build.ps1 tutorial\3.population-manager.cpp   输出英文(纪律 6)。
#include <cstdio>
#include "ecflow.h"
using namespace ECFlow;

static double f_sphere(double** a) { double s = 0; for (int i = 0; i < 5; i++) s += a[0][i] * a[0][i]; return s; }

int main()
{
    const int N = 5, POP = 20, FES = 20000;

    Problem p("sphere");
    p.addVariable("x", -5.0, 5.0, 0.01, N);
    p.addObjective("f", 1, true, "x", f_sphere);

    ConfigBuilder cb;

    // 两条不同的 workflow:一条 PSO、一条 GA(展示子群可异构)
    auto pso = cb.workflow("pso_wf");
    pso.initializer("Random"); pso.topology.pgBest(); pso.repair.boundary();
    pso.strategy.velocityDriven(2, 2.0, 0.9, 0.5 * POP / FES);
    pso.generator.orderedConstruct(); pso.evaluator.basic(); pso.selector.index(false);

    auto ga = cb.workflow("ga_wf");
    ga.initializer("Random"); ga.topology.tournament(2, 1); ga.repair.boundary();
    ga.strategy.sbxCrossover(20, 1.0); ga.generator.generation();
    ga.topology.continueGraph(); ga.strategy.pmMutation(20, 0.01); ga.generator.generation(true);
    ga.evaluator.basic(); ga.selector.rank();

    // 优化器级 + 多子群协作:NoInteraction(独立进化+全局汇集) + Fixed 构建器 + Connected 拓扑
    cb.name("multi_run").maxFES(FES).gArchive("Basic")
      .manager("NoInteraction").cooperationConstructer("Fixed").cooperationTopology("Connected");

    // 两个异构子群:pso 群用 Particle,ga 群用 Individual
    cb.subpopulation("pso").size(POP)
      .workflow("pso_wf").maxFES(FES).archive("Basic");
    cb.subpopulation("ga").size(POP)
      .workflow("ga_wf").maxFES(FES).archive("Basic");

    Optimizer* opt = cb.buildOptimizer();
    opt->setProblem(&p);

    std::printf("subpops found: pso=%s, ga=%s\n",
                opt->getSubswarm("pso") ? "yes" : "no",
                opt->getSubswarm("ga")  ? "yes" : "no");

    opt->exe(42);

    Solution* best = nullptr; int bs = 0;
    opt->getBest(best, bs);                       // 全局档案:两子群最优的汇集
    std::printf("multi-subpop (PSO + GA, NoInteraction) global best = %.6f\n",
                (best && bs > 0) ? best->fitness[0] : 1e18);

    delete opt;
    std::printf("\n3.population-manager tutorial: DONE\n");
    return 0;
}
