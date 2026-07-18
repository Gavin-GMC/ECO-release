// 1.optimizerconfig.cpp — 优化器配置与运行用例（参照稳定版 1.optimzerconfig.cpp,改用 v3.12 的 ConfigBuilder fluent API）
//   演示:用 ConfigBuilder 装配一个 PSO 优化器 → setProblem → exe → 取最优；并说明如何换算子/多轮运行/日志。
//   构建: build.ps1 tutorial\1.optimizerconfig.cpp   输出英文(纪律 6)。
#include <cstdio>
#include "ecflow.h"
using namespace ECFlow;

static double f_sphere(double** a) { double s = 0; for (int i = 0; i < 5; i++) s += a[0][i] * a[0][i]; return s; }

int main()
{
    const int N = 5, POP = 20, FES = 40000;

    // ---------- 1. 定义问题(见 0.problem.cpp) ----------
    Problem p("sphere");
    p.addVariable("x", -5.0, 5.0, 0.01, N);
    p.addObjective("f", 1, true, "x", f_sphere);   // 最小化 Σx²

    // ---------- 2. 用 ConfigBuilder 装配优化器(PSO) ----------
    ConfigBuilder cb;

    // 2a. 一条 workflow(算子流水线):拓扑→修复→策略→生成→评估→选择
    auto wf = cb.workflow("pso_wf");
    wf.initializer("Random");                                  // 初始化:域内随机
    wf.topology.pgBest();                                       // 学习拓扑:pbest+gbest(2 学习对象)
    wf.repair.boundary();                                       // 修复:越界夹回域内
    wf.strategy.velocityDriven(2, 2.0, 0.9, 0.5 * POP / FES);   // 学习策略:PSO(对象数,c,w_ini,w 衰减)
    wf.generator.orderedConstruct();                           // 生成器:逐维构造
    wf.evaluator.basic();                                       // 评估:逐个真实评估
    wf.selector.index(false);                                  // 选择:PSO 式位置替换(无条件)

    // 2b. 优化器级设置(名字/终止/子群协作管理器/全局档案)
    cb.name("pso_run").maxFES(FES).manager("Single").gArchive("Basic");

    // 2c. 子种群:个体类型(PSO 需 Particle 带 velocity)、规模、绑定 workflow、终止、档案
    cb.subpopulation("1").size(POP)
      .workflow("pso_wf").maxFES(FES).archive("Basic");

    // 2d. 可选:日志(结果/过程/详细过程),此处关闭
    cb.logging(false, false, false);

    // ---------- 3. 装配 → 接入问题 → 运行 ----------
    Optimizer* opt = cb.buildOptimizer();
    opt->setProblem(&p);
    opt->exe(42);                                              // 播种 42、跑一次;多轮实验用 opt->exe(30, 42)

    // ---------- 4. 取最优 ----------
    Solution* best = nullptr; int bs = 0;
    opt->getBest(best, bs);
    std::printf("PSO on sphere(5-D): best fitness = %.6f\n", (best && bs > 0) ? best->fitness[0] : 1e18);

    delete opt;

    // ---------- 说明:如何换算子 ----------
    // 换算法 = 换配置行,不改代码结构。例如换成 GWO(灰狼):
    //   wf.topology.topRanked(3); wf.strategy.greyWolfEncircling(2.0, 0.0); wf.selector.rank();
    //   cb.subpopulation("1")...   // GWO 不需 Particle
    // 换成 GA(两段:交叉段+变异段续接):见 tutorial/config-generate.cpp 的 GA_* 配置。
    // 各 tag 的语义/参数/适用编码见 手册 11-使用-优化器配置.md 的算子目录。

    std::printf("\n1.optimizerconfig tutorial: DONE\n");
    return 0;
}
