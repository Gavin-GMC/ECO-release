// 6.elite-local-search.cpp — 教程:如何给已有算法**追加精英局部搜索段**。
//   精英局搜 = 每代取若干精英、在其邻域生成候选、择优后归属替换 → 对已有算法(PSO/GA/…)的**强化(intensification)**。
//   落法:纯配置——把 [拓扑→策略→生成→评估→选择] 一段**接在主算法各段之后**(WorkflowSetter 调用顺序 = 执行顺序)。
//   构建: build.ps1 tutorial\6.elite-local-search.cpp   输出英文(纪律 6)。
#include <cstdio>
#include "ecflow.h"
#include "config-setter.h"
using namespace ECFlow;

static double f_sphere(double** a) { double s = 0; for (int i = 0; i < 5; i++) s += a[0][i] * a[0][i]; return s; }

// 跑一个配置、返回 best(seed 固定,便于对比)
static double run(ConfigBuilder& cb, Problem& p, const char* wf_tag, int fes)
{
    cb.name("run").maxFES(fes).manager("Single").gArchive("Basic");
    cb.subpopulation("1").size(20).workflow(wf_tag).maxFES(fes).archive("Basic");
    cb.logging(false, false, false);
    Optimizer* opt = cb.buildOptimizer();
    opt->setProblem(&p); opt->exe(42);
    Solution* b = nullptr; int bs = 0; opt->getBest(b, bs);
    double best = (b && bs > 0) ? b->fitness[0] : 1e18;
    delete opt; return best;
}

int main()
{
    const int N = 5, FES = 40000;
    Problem p("sphere");
    p.addVariable("x", -5.0, 5.0, 0.01, N);
    p.addObjective("f", 1, true, "x", f_sphere);   // 最小化 Σx²

    // ================= 基线:纯 PSO(见 1.optimizerconfig.cpp)=================
    double best_pso;
    {
        ConfigBuilder cb;
        auto wf = cb.workflow("pso");
        wf.initializer("Random"); wf.repair.boundary();
        wf.topology.pgBest(); wf.strategy.velocityDriven(2, 2.0, 0.9, 0.5 * 20.0 / FES);
        wf.generator.orderedConstruct(); wf.evaluator.basic(); wf.selector.index(false);
        best_pso = run(cb, p, "pso", FES);
    }

    // ================= 路径①:种群 top-e 精英局搜(血缘定向) =================
    //   关键:在 PSO 各段之后,再接一段——
    //     topology.topIndividual(e, r)  取当代种群 top-e,每精英作起点重复 r 次(起点=**种群槽**)
    //     strategy.<邻域算子>            在精英邻域生成候选(此处复用 ABC 的 differencePerturbation)
    //     selector.kinshipGreedy()       **血缘定向**:读候选的 parent_id 归属回各自精英槽,优则替换(E6)
    //   血缘 id 由生成管线自动盖(id=槽序号),无需手工;kinship 特性由 kinshipGreedy 声明、装配期自动挂载。
    double best_elite_pop;
    {
        ConfigBuilder cb;
        auto wf = cb.workflow("pso_elite_pop");
        wf.initializer("Random"); wf.repair.boundary();
        // —— 主算法段:PSO ——
        wf.topology.pgBest(); wf.strategy.velocityDriven(2, 2.0, 0.9, 0.5 * 20.0 / FES);
        wf.generator.orderedConstruct(); wf.evaluator.basic(); wf.selector.index(false);
        // —— 追加:精英局搜段(种群 top-e)——
        wf.topology.topIndividual(3, 3);          // top-3 精英,各生成 3 个邻居
        wf.strategy.differencePerturbation();     // 邻域算子(连续;排列问题可换 overturn/exchange 等)
        wf.generator.generation(); wf.evaluator.basic();
        wf.selector.kinshipGreedy();              // 血缘定向 + 择优替换精英
        best_elite_pop = run(cb, p, "pso_elite_pop", FES);
    }

    // ================= 路径②:档案精英局搜(全局归并) =================
    //   若精英来自**最优档案**(非当代种群槽),用 topology.elite(e, r)(内部 buffer 承载档案精英)
    //   + selector.rank()(全局择优归并)——起点非种群槽、不走血缘归属。select 层区分两条路径。
    double best_elite_arc;
    {
        ConfigBuilder cb;
        auto wf = cb.workflow("pso_elite_arc");
        wf.initializer("Random"); wf.repair.boundary();
        wf.topology.pgBest(); wf.strategy.velocityDriven(2, 2.0, 0.9, 0.5 * 20.0 / FES);
        wf.generator.orderedConstruct(); wf.evaluator.basic(); wf.selector.index(false);
        // —— 追加:精英局搜段(档案精英)——
        wf.topology.elite(3, 3);                  // 档案精英,各重复 3 次
        wf.strategy.gaussMutation(1.0, 1);        // 邻域:高斯小扰动(elite 端点=自身,不能用差分)
        wf.generator.generation(); wf.evaluator.basic();
        wf.selector.rank();                       // 全局择优归并
        best_elite_arc = run(cb, p, "pso_elite_arc", FES);
    }

    std::printf("=== 精英局部搜索教程(sphere,N=5,FES=%d,seed=42)===\n", FES);
    std::printf("  基线 PSO            best = %.6f\n", best_pso);
    std::printf("  +种群 top-e 精英段  best = %.6f  (topIndividual + kinshipGreedy)\n", best_elite_pop);
    std::printf("  +档案精英段         best = %.6f  (elite + rank)\n", best_elite_arc);
    std::printf("\n要点:精英局搜是**追加段**——按调用顺序接在主算法各段之后即可(components = 执行序);\n");
    std::printf("      种群路径用 topIndividual+kinshipGreedy(血缘归属替换精英槽),档案路径用 elite+rank(全局归并)。\n");
    return 0;
}
