//------------------------Description------------------------
// ECFlow 可执行入口(收尾#4,v1.4.9)——可被外部(cmd/终端)调用的优化程序。
//   用法:  ecflow <问题文件> <配置文件> [选项]
//     必需位置参数:
//       <问题文件>  ECFlow 标准算例文件(头部含 `TYPE:`),经 loadProblem 按家族分派构造 Problem。
//       <配置文件>  优化器配置 .cfg,经 loadConfig 读入。
//     选项:
//       -s <seed>       RNG 种子(默认 1)
//       -n <runs>       独立运行次数(默认 1;多次结果落 .rslt 日志,控制台报最后一次最优)
//       -o <file>       结果写文件(默认 stdout)
//       --max-fes <N>   覆盖配置的最大评估次数(总预算,同时覆盖全局与各子群)
//       --max-time <s>  覆盖最大运行秒数
//       --max-conv <N>  覆盖最大停滞代数
//       -q, --quiet     只输出最优 fitness(机读友好)
//       -v, --verbose   每代进度回显到控制台(经 Logger 控制台回显)
//       -h, --help      显示用法
//   流程:解析参数 → 读问题 → 读配置(+覆盖) → 装配 Optimizer → 播种运行 → 输出最优解。
//   注:框架日志(.log/.err/.rslt/.prcs)始终落 `_log/`;本 exe 的 stdout 是独立于日志文件的控制台通道。
//-------------------------Reference-------------------------
// loadProblem = problem-template.h(T-LOAD);loadConfig = config-setter.h(CONFIG-IO);-v 回显 = logger.hpp。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "ecflow.h"
#include "problem-template.h"
using namespace ECFlow;

static void printUsage(const char* prog)
{
    std::fprintf(stderr,
        "ECFlow optimizer runner\n"
        "usage: %s <problem-file> <config-file> [options]\n"
        "  <problem-file>   ECFlow instance file (header must contain 'TYPE:')\n"
        "  <config-file>    optimizer config .cfg \n"
        "options:\n"
        "  -s <seed>        RNG seed (default 1)\n"
        "  -n <runs>        independent runs (default 1)\n"
        "  -o <file>        write result to file (default stdout)\n"
        "  --max-fes <N>    override max function evaluations (total budget)\n"
        "  --max-time <s>   override max run time (seconds)\n"
        "  --max-conv <N>   override max stagnation generations\n"
        "  -q, --quiet      print best fitness only\n"
        "  -v, --verbose    echo per-generation progress to console\n"
        "  -h, --help       show this help\n",
        prog);
}

// 把配置文件路径拆成 loadConfig(name, dir) 需要的 (dir, name):去 .cfg 扩展 + 按最后分隔符切目录。
static void splitConfigPath(const std::string& in, std::string& dir, std::string& name)
{
    std::string s = in;
    if (s.size() > 4 && s.substr(s.size() - 4) == ".cfg") s = s.substr(0, s.size() - 4);
    size_t slash = s.find_last_of("/\\");
    if (slash == std::string::npos) { dir = "config"; name = s; }
    else { dir = s.substr(0, slash); name = s.substr(slash + 1); }
}

int main(int argc, char** argv)
{
    // ---- 参数解析 ----
    std::string problem_path, config_path, output_file;
    unsigned seed = 1;
    int runs = 1;
    int max_fes = -1, max_time = -1, max_conv = -1;   // -1 = 不覆盖,用配置的
    bool quiet = false, verbose = false;
    std::vector<std::string> positional;

    // 取 -x 的值(下一个实参);缺失则报错退出
    auto nextVal = [&](int& i, const char* opt) -> std::string {
        if (i + 1 >= argc) { std::fprintf(stderr, "ERROR: option %s requires a value\n", opt); std::exit(2); }
        return argv[++i];
    };

    for (int i = 1; i < argc; ++i)
    {
        std::string a = argv[i];
        if      (a == "-h" || a == "--help")    { printUsage(argv[0]); return 0; }
        else if (a == "-q" || a == "--quiet")   { quiet = true; }
        else if (a == "-v" || a == "--verbose") { verbose = true; }
        else if (a == "-s")            { seed = (unsigned)std::strtoul(nextVal(i, "-s").c_str(), nullptr, 10); }
        else if (a == "-n")            { runs = std::atoi(nextVal(i, "-n").c_str()); }
        else if (a == "-o")            { output_file = nextVal(i, "-o"); }
        else if (a == "--max-fes")     { max_fes  = std::atoi(nextVal(i, "--max-fes").c_str()); }
        else if (a == "--max-time")    { max_time = std::atoi(nextVal(i, "--max-time").c_str()); }
        else if (a == "--max-conv")    { max_conv = std::atoi(nextVal(i, "--max-conv").c_str()); }
        else if (!a.empty() && a[0] == '-') { std::fprintf(stderr, "ERROR: unknown option '%s'\n", a.c_str()); printUsage(argv[0]); return 2; }
        else positional.push_back(a);
    }

    if (positional.size() < 2) { printUsage(argv[0]); return 2; }
    problem_path = positional[0];
    config_path  = positional[1];
    if (runs < 1) runs = 1;

    Problem*   problem = nullptr;
    Optimizer* opt     = nullptr;
    try
    {
        // 1) 读问题
        problem = loadProblem(problem_path);

        // 2) 读配置
        std::string cdir, cname;
        splitConfigPath(config_path, cdir, cname);
        FullConfig fc = ConfigBuilder::loadConfig(cname, cdir);

        // 3) 覆盖:预算(总预算 → 全局与各子群同置) + verbose(进度回显)
        auto applyTerm = [&](int idx, int val) {
            if (val < 0) return;
            fc.optimizer.terminate_conditions[idx] = val;
            for (auto& sp : fc.optimizer.subpopulations) sp.terminate_conditions[idx] = val;
        };
        applyTerm(0, max_fes);
        applyTerm(1, max_conv);
        applyTerm(2, max_time);
        if (verbose) { fc.optimizer.logger_process = true; fc.optimizer.logger_console_echo = true; }

        // 4) 装配 Optimizer
        OptimizerBuilder builder;
        for (auto& wf : fc.workflows) builder.registerWorkflow(wf);
        opt = builder.buildOptimizer(fc.optimizer);

        // 5) 运行(runs 次;n>1 逐次落 .rslt 日志)
        opt->setProblem(problem);
        opt->exe(runs, (time_t)seed);

        // 6) 输出(-o 文件 / 默认 stdout;-q 仅 fitness)
        std::ofstream ofs;
        std::ostream* os = &std::cout;
        if (!output_file.empty())
        {
            ofs.open(output_file);
            if (!ofs) { std::fprintf(stderr, "ERROR: cannot open output file: %s\n", output_file.c_str()); delete opt; delete problem; return 1; }
            os = &ofs;
        }

        Solution* best = nullptr; int bs = 0;
        if (opt->getBest(best, bs) && best && bs > 0)
        {
            if (quiet)
            {
                *os << best[0].fitness[0] << "\n";
            }
            else
            {
                *os << "problem: " << problem_path << "\n"
                    << "seed: " << seed << "\nruns: " << runs << "\n"
                    << "best_fitness: " << best[0].fitness[0] << "\nsolutions: " << bs << "\n";
                for (int i = 0; i < bs; i++)
                    *os << "  [" << (i + 1) << "] "
                        << best[i].decoder_pointer->toString(best[i].result, best[i].fitness, 1) << "\n";
            }
        }
        else
        {
            std::fprintf(stderr, "no solution produced\n");
        }
    }
    catch (const std::exception& ex)
    {
        std::fprintf(stderr, "ERROR: %s\n", ex.what());
        delete opt;
        delete problem;
        return 1;
    }

    delete opt;
    delete problem;
    return 0;
}
