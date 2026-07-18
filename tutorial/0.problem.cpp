// 0.problem.cpp — 问题模块用例（参照稳定版 tutorial/0.problem.cpp，适配 v3.12 API）
//   演示:变量/目标/约束/启发的定义 → compile() → 解评估/比较/约束违反/约束传播/启发排序。
//   仅用问题模块(不涉及优化器)。构建: build.ps1 tutorial\0.problem.cpp
//   输出为英文(开发纪律 6:示例输出用英文避免乱码);注释为中文。
#include <cstdio>
#include <cmath>
#include "ecflow.h"
using namespace ECFlow;

// 目标/启发用的普通函数(double** = 各输入变量的元素数组;a[k][j]=第 k 个输入变量的第 j 维)
static double f_sphere(double** a) { double s = 0; for (int i = 0; i < 5; i++) s += a[0][i] * a[0][i]; return s; }      // Σx²（对变量 x）
static double f_sum(double** a)    { double s = 0; for (int i = 0; i < 5; i++) s += a[0][i] + a[1][i]; return s; }      // Σ(x+y)（对 x,y）
static double f_zero(double**)     { return 0; }                                                                       // 占位目标(优先级 0=不参与)
static double heu_absneg(double** in) { return -std::fabs(in[0][0]); }                                                 // 启发:元素绝对值的负

int main()
{
    // ---------- 1. 问题定义 ----------
    Problem problem("test_problem");
    problem.addVariable("x", -5, 5, 0.001, 5);                 // 名, 下界, 上界, 精度, 维数
    problem.addVariable("y", -5, 5, 0.001, 5);

    problem.addObjective("f1", 1, true,  "x",   f_sphere);     // 优先级 1, 最小化, 输入=x
    problem.addObjective("f2", 1, false, "x,y", f_sum);        // 优先级 1, 最大化, 输入=x,y
    problem.addObjective("f3", 0, true,  "x",   f_zero);       // 优先级 0 → 不参与择优(演示 comparer 过滤)
    problem.addObjective("f4", 0, true,  "x",   f_zero);

    problem.addInspirationFunc("x", "x", heu_absneg);          // 变量 x 的启发 = -|x|
    problem.addInspirationRandom("y");                          // 变量 y 用随机启发(缺省即此)

    problem.addConstrainUnique("x", 1, "f3");                  // x 各维取值互异(惩罚权 1, 记入目标 f3)
    problem.addConstrainMinDistance("y", 0.1, 2, "f3");        // y 各维两两最小间距 0.1
    problem.addConstrainRange("x", EMPTYVALUE, EMPTYVALUE, 1, "f4");  // x 域约束(此处不缩窄,仅演示挂载)

    // ---------- 2. 编译 ----------
    ProblemHandle* h = problem.compile();
    const int PS = h->getProblemSize();      // 总维数 = 10 (x:5 + y:5)
    const int ON = h->getObjectNumber();
    std::printf("problem size=%d, objectives=%d\n", PS, ON);

    // ---------- 3. 构建两个解 ----------
    Solution s1, s2;
    s1.setSize(PS, ON); s1.setDecoder(h->getSolutionDecoder());
    s2.setSize(PS, ON); s2.setDecoder(h->getSolutionDecoder());
    double v1[10] = { 0,1,2,3,4, 5,6,7,8,9 };
    double v2[10] = { 5,6,7,8,9, 0,0.1,0.3,0.2,-0.1 };
    for (int i = 0; i < PS; i++) { s1[i] = v1[i]; s2[i] = v2[i]; }

    // ---------- 4. 解评估 + 比较 ----------
    h->solutionEvaluate(s1);
    h->solutionEvaluate(s2);
    std::printf("s1 fitness[0]=%.3f  s2 fitness[0]=%.3f\n", s1.fitness[0], s2.fitness[0]);
    Comparer* cmp = h->getSolutionComparer();
    std::printf("s1 better than s2 ? %s\n", cmp->isBetter(s1.fitness, s2.fitness) ? "yes" : "no");

    // ---------- 5. 约束违反程度 ----------
    std::printf("s1 violation=%.4f  s2 violation=%.4f\n",
                h->constraintViolation(s1), h->constraintViolation(s2));

    // ---------- 6. 约束传播 + 启发排序(逐维贪心构造演示) ----------
    h->constrainReset();
    h->setResult(s1);
    std::printf("greedy construct on s1:\n");
    for (int d = 0; d < PS; d++)
    {
        double* feas = h->getFeasibleList(d);          // 当前维可行域(调用方负责 delete[])
        int nfeas = h->getChoiceNumber(d);
        double priori = h->getPrioriChoice(d);         // 启发最优取值
        double hinfo = h->getChoiceHeuristic(d, priori);

        if (h->constrainCheck(d, priori))              // 通过约束检查 → 落子并传播
        {
            s1[d] = priori;
            h->constrainChange(d, s1[d]);
        }
        if (d < 3)                                     // 只打印前几维,避免刷屏
            std::printf("  dim %d: feasible=%d, priori=%.3f, heuristic=%.3f\n", d, nfeas, priori, hinfo);

        delete[] feas;
    }

    delete h;
    std::printf("\n0.problem tutorial: DONE\n");
    return 0;
}
