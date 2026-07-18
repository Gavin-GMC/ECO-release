// 5.template.cpp — 用问题模板端到端跑优化
//   演示:用 PT_TSP 一键构造一个旅行商问题 → getProblem() → 配 ACO(蚁系统) 优化器 → exe → 取最优巡回长度。
//   模板省去手工 addVariable/addObjective/addConstrain;排列编码问题配 ACO(isolate + 构造式生成器)。
//   构建: build.ps1 tutorial\5.template.cpp   输出英文(纪律 6)。
#include <cstdio>
#include <cmath>
#include "ecflow.h"          // 含 Problem/ConfigBuilder
#include "pt-tsp.h"        // 问题模板按需单独 include(ecflow.h 不含)
using namespace ECFlow;

int main()
{
    const int CITY = 8, POP = 20, FES = 30000;

    // ---------- 1. 用模板构造 TSP：8 座城均匀分布在单位圆上（最优巡回=沿圆周） ----------
    double pos[CITY * 2];
    for (int i = 0; i < CITY; i++) {
        double ang = 2.0 * 3.14159265358979 * i / CITY;
        pos[i * 2] = std::cos(ang);
        pos[i * 2 + 1] = std::sin(ang);
    }
    PT_TSP tsp;
    tsp.setName("circle8");
    tsp.setCitys(pos, CITY, 2);                          // 城市坐标(点数, 维度=2)
    tsp.setEdgeWeightType(PT_TSP::DistanceType::Eula);   // 欧氏距离
    Problem* p = tsp.getProblem();                       // 拿到 Problem*(调用方拥有)

    double optimal = CITY * 2.0 * std::sin(3.14159265358979 / CITY);  // 圆周长参考
    std::printf("TSP circle8: problem built, optimal tour ~= %.4f\n", optimal);

    // ---------- 2. 配 ACO(蚁系统) 优化器（排列编码 → isolate + 构造式生成器） ----------
    ConfigBuilder cb;
    auto wf = cb.workflow("as_wf");
    wf.initializer("Random");
    wf.topology.isolate();                               // ACO 自构造,无学习对象
    wf.repair.random();
    wf.strategy.antSystem({ 1, 2, 0.5, EMPTYVALUE });    // AS: alpha=1,belta=2,rho=0.5,tao_ini 缺省
    wf.generator.orderedConstruct();                    // 逐维顺序构造(蚂蚁逐步建路径)
    wf.evaluator.basic();
    wf.selector.index(false);
    cb.name("tsp_as").maxFES(FES).manager("Single").gArchive("Basic");
    cb.subpopulation("1").size(POP).workflow("as_wf").maxFES(FES).archive("Basic");

    // ---------- 3. 运行 ----------
    Optimizer* opt = cb.buildOptimizer();
    opt->setProblem(p);                                  // setProblem(Problem*) 内部 compile
    opt->exe(42);

    Solution* best = nullptr; int bs = 0;
    opt->getBest(best, bs);
    double tour = (best && bs > 0) ? best->fitness[0] : 1e18;
    std::printf("ACO(AntSystem) best tour = %.4f  (optimal ~= %.4f)\n", tour, optimal);

    delete opt;
    delete p;                                            // 模板 getProblem() 的产物,调用方释放
    std::printf("\n5.template tutorial: DONE\n");
    return 0;
}
