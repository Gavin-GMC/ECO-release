// 2.subpopulation.cpp — 子种群 / workflow 内部结构用例（参照稳定版 2.subpopulation.cpp,适配 v3.12）
//   在 1.optimizerconfig 的单算子基础上,演示:
//     ① 多段 workflow —— 一条 workflow 内「交叉段 + 变异段」串联(GA 的组织方式);
//     ② 子种群访问 —— 装配后经 opt->getSubswarm(id) 拿到 Subpopulation 层对象。
//   构建: build.ps1 tutorial\2.subpopulation.cpp   输出英文(纪律 6)。
#include <cstdio>
#include "ecflow.h"
using namespace ECFlow;

static double f_sphere(double** a) { double s = 0; for (int i = 0; i < 5; i++) s += a[0][i] * a[0][i]; return s; }

int main()
{
    const int N = 5, POP = 40, FES = 40000;

    Problem p("sphere");
    p.addVariable("x", -5.0, 5.0, 0.01, N);
    p.addObjective("f", 1, true, "x", f_sphere);

    ConfigBuilder cb;
    auto wf = cb.workflow("ga_wf");
    wf.initializer("Random");
    wf.topology.tournament(2, 1);                                  // 交叉段:锦标赛选亲代
    wf.repair.boundary();
    // —— 段 1:交叉 ——（产出交叉子代,append 进 offspring）
    wf.strategy.sbxCrossover(20, 1.0);   wf.generator.generation();
    // —— 段 2:变异 ——（continueGraph 续接段1产物,generation(true) 原地串联变异）
    wf.topology.continueGraph();
    wf.strategy.pmMutation(20, 0.01);    wf.generator.generation(true);
    // —— 段末:评估 + 选择 ——（(μ+λ) 择优归并）
    wf.evaluator.basic();
    wf.selector.rank();

    cb.name("ga_run").maxFES(FES).manager("Single").gArchive("Basic");
    cb.subpopulation("1").size(POP)
      .workflow("ga_wf").maxFES(FES).archive("Basic");

    Optimizer* opt = cb.buildOptimizer();
    opt->setProblem(&p);

    // 子种群层访问:装配后可按 id 取到 Subpopulation(生成+选择的基本单元)
    Subpopulation* sp = opt->getSubswarm("1");
    std::printf("subpopulation \"1\" found: %s\n", sp ? "yes" : "no");

    opt->exe(42);

    Solution* best = nullptr; int bs = 0;
    opt->getBest(best, bs);
    std::printf("GA(SBX+PM, 2-segment) on sphere(5-D): best = %.6f\n", (best && bs > 0) ? best->fitness[0] : 1e18);

    delete opt;
    std::printf("\n2.subpopulation tutorial: DONE\n");
    return 0;
}
