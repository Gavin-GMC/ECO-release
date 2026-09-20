//------------------------Description------------------------
// EvoFlow 可执行入口。
//   evoflow                              启动图形界面
//   evoflow run  <问题文件> <配置文件>   跑一次优化
//   evoflow -h | --help                  用法

// SPDX-License-Identifier: BSD-3-Clause

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <limits>
#include "app-facade.h"
#include "json-writer.h"
#include "config-validator.h"
#include "run-reporter.h"
#include "data-package.h"
#include "ui-app.h"
#include "evoflow-version.h"
using namespace EvoFlow;

static bool parseCliInteger(const std::string& text, long long minimum,
                            long long maximum, long long& value)
{
    if (text.empty()) return false;
    size_t used = 0;
    try { value = std::stoll(text, &used, 10); }
    catch (...) { return false; }
    return used == text.size() && value >= minimum && value <= maximum;
}

static void printUsage(const char* prog)
{
    std::fprintf(stderr,
        "EvoFlow " EVOFLOW_VERSION_STRING "\n"
        "usage: %s <command> [args]\n"
        "commands:\n"
        "  (no args)        launch the graphical interface\n"
        "  run <problem> <config> [options]   run one optimization\n"
        "  validate <path> [options]          validate config files (file or directory)\n"
        "  report <path> [options]            summarise run results (per experiment batch)\n"
        "  data [path]                        inspect minimal/full problem data\n"
        "  --version        show the public product version\n"
        "  -h, --help       show this help\n"
        "\n"
        "run options:\n"
        "  <problem>        EvoFlow instance file (header must contain 'TYPE:')\n"
        "  <config>         optimizer config .cfg (v3.1 text / v3.2 json)\n"
        "  -s <seed>        RNG seed (default 1)\n"
        "  -n <runs>        independent runs (default 1)\n"
        "  --optimizer-name <name>  stable output identity (default: config name)\n"
        "  --run-start <N>  explicit zero-based result index (default: automatic)\n"
        "  --replace-existing  transactionally replace an explicit result range\n"
        "  -o <file>        write result to file (default stdout)\n"
        "  --max-fes <N>    override max function evaluations (total budget)\n"
        "  --max-time <s>   override max run time (seconds)\n"
        "  --max-conv <N>   override max stagnation generations\n"
        "  -q, --quiet      print best fitness only\n"
        "  -v, --verbose    echo per-generation progress to console\n"
        "\n"
        "validate options:\n"
        "  <path>           a .cfg file, or a directory containing .cfg files\n"
        "  -r, --recursive  recurse into subdirectories (default: one level)\n"
        "  --build          deep check: actually assemble once (catches problems that\n"
        "                   static checking cannot; off by default, it constructs objects)\n"
        "  -f, --format <f> txt (default) or json\n"
        "  -o <file>        write report to file (default stdout)\n"
        "  -q, --quiet      summary line only, no per-issue listing\n"
        "  exit code        0 = all passed, 1 = errors found, 2 = usage error\n"
        "\n"
        "data:\n"
        "  [path]           data root (default _pdata)\n"
        "  exit code        0 = minimal/full, 1 = missing/incomplete, 2 = usage error\n"
        "\n"
        "report options:\n"
        "  <path>           log directory (default _log)\n"
        "  --list           list the experiment batches found, then stop\n"
        "  --tag <tag>      report on ONE batch. Required -- results from different\n"
        "                   batches are not comparable, so no batch is picked for you\n"
        "  --charts <dir>   also write charts (table / distribution / average rank)\n"
        "  --png            charts as PNG instead of SVG (needs system fonts)\n"
        "  -f, --format <f> txt (default) / csv / latex / json\n"
        "  -o <file>        write report to file (default stdout)\n"
        "\n"
        "TIP: give 'run' a --tag so its results group into one batch; without it every\n"
        "     run gets a fresh timestamp tag and cannot be aggregated.\n"
        "\n"
        "NOTE: the bare form 'evoflow <problem> <config>' used before v2.3.1.1 is gone;\n"
        "      use 'evoflow run <problem> <config>'. Bare 'evoflow' now launches the UI.\n",
        prog);
}

static int cmdData(int argc, char** argv, const char* prog)
{
    if (argc > 3)
    {
        std::fprintf(stderr, "ERROR: data accepts at most one path\n");
        printUsage(prog);
        return 2;
    }
    if (argc == 3 && (std::strcmp(argv[2], "-h") == 0
                      || std::strcmp(argv[2], "--help") == 0))
    {
        printUsage(prog);
        return 0;
    }
    const std::filesystem::path root = argc == 3
        ? std::filesystem::u8path(argv[2]) : std::filesystem::path("_pdata");
    const App::DataPackageReport report = App::inspectDataPackage(root);
    std::cout << "data_root: " << report.root.u8string() << "\n"
              << "status: " << report.status << "\n"
              << "manifest_files: " << report.manifestPresent << "/"
              << report.manifestExpected << "\n"
              << "loadable_instances: " << report.loadableInstances << "\n";
    for (const std::string& issue : report.issues)
        std::cout << "data_error: " << issue << "\n";
    return report.ok() ? 0 : 1;
}

// run 子命令:解析其选项 → 调 facade → 渲染
static int cmdRun(int argc, char** argv, const char* prog)
{
    App::RunOptions o;
    std::string output_file;
    bool quiet = false;
    std::vector<std::string> positional;

    auto nextVal = [&](int& i, const char* opt) -> std::string {
        if (i + 1 >= argc) { std::fprintf(stderr, "ERROR: option %s needs a value\n", opt); std::exit(2); }
        return argv[++i];
    };

    for (int i = 2; i < argc; i++)          // 自 2 起:argv[1] 是子命令名
    {
        std::string a = argv[i];
        if      (a == "-h" || a == "--help")    { printUsage(prog); return 0; }
        else if (a == "-q" || a == "--quiet")   { quiet = true; }
        else if (a == "-v" || a == "--verbose") { o.verbose = true; }
        else if (a == "-s")            { o.seed = (unsigned)std::strtoul(nextVal(i, "-s").c_str(), nullptr, 10); }
        else if (a == "-n")
        {
            const std::string value = nextVal(i, "-n");
            long long parsed = 0;
            if (!parseCliInteger(value, 1, (std::numeric_limits<int>::max)(), parsed))
            { std::fprintf(stderr, "ERROR: -n must be a positive integer\n"); return 2; }
            o.runs = static_cast<int>(parsed);
        }
        else if (a == "-o")            { output_file = nextVal(i, "-o"); }
        else if (a == "--tag")         { o.tag = nextVal(i, "--tag"); }
        else if (a == "--optimizer-name") { o.optimizerName = nextVal(i, "--optimizer-name"); }
        else if (a == "--run-start")
        {
            const std::string value = nextVal(i, "--run-start");
            long long parsed = 0;
            if (!parseCliInteger(value, 0, (std::numeric_limits<int>::max)(), parsed))
            { std::fprintf(stderr, "ERROR: --run-start must be a non-negative integer\n"); return 2; }
            o.runStart = static_cast<int>(parsed);
        }
        else if (a == "--replace-existing") { o.replaceExisting = true; }
        else if (a == "--max-fes")     { o.maxFES  = std::atoi(nextVal(i, "--max-fes").c_str()); }
        else if (a == "--max-time")    { o.maxTime = std::atoi(nextVal(i, "--max-time").c_str()); }
        else if (a == "--max-conv")    { o.maxConv = std::atoi(nextVal(i, "--max-conv").c_str()); }
        else if (!a.empty() && a[0] == '-')
        { std::fprintf(stderr, "ERROR: unknown option '%s'\n", a.c_str()); printUsage(prog); return 2; }
        else positional.push_back(a);
    }

    if (positional.size() < 2) { printUsage(prog); return 2; }
    o.problemPath = positional[0];
    o.configPath  = positional[1];
    if (o.replaceExisting && o.runStart < 0)
    {
        std::fprintf(stderr, "ERROR: --replace-existing requires --run-start <N>\n");
        return 2;
    }

    App::RunResult r = App::runOptimization(o);      // 逻辑层:不输出、不抛
    if (!r.ok)
    {
        std::fprintf(stderr, "ERROR: %s\n", r.error.c_str());
        return 1;
    }

    std::ofstream ofs;
    std::ostream* os = &std::cout;
    if (!output_file.empty())
    {
        ofs.open(output_file);
        if (!ofs) { std::fprintf(stderr, "ERROR: cannot open output file: %s\n", output_file.c_str()); return 1; }
        os = &ofs;
    }

    if (quiet)
    {
        *os << r.bestFitness << "\n";
    }
    else
    {
        *os << "problem: " << o.problemPath << "\n"
            << "seed: " << o.seed << "\nruns: " << o.runs << "\n"
            << "optimizer: " << r.optimizerName << "\nresult_start: " << r.runStart << "\n"
            << "best_fitness: " << r.bestFitness << "\nsolutions: " << r.best.size() << "\n";
        for (size_t i = 0; i < r.best.size(); i++)
            *os << "  [" << (i + 1) << "] " << r.best[i].text << "\n";
    }
    return 0;
}

// ---- validate 子命令:解析选项 → 调 facade → 渲染(txt / json) ----
static void renderValidateTxt(std::ostream& os, const App::ValidateReport& r, bool quiet)
{
    os << "scanned: " << r.scanned
       << "   pass: " << r.passFiles << "   info: " << r.infoFiles
       << "   warning: " << r.warningFiles << "   error: " << r.errorFiles
       << "   status: " << r.status
       << "   depth: " << (r.deepBuild ? "build" : "static") << "\n";
    if (quiet) return;
    std::string cur;
    for (size_t i = 0; i < r.issues.size(); i++)
    {
        const App::ConfigIssue& c = r.issues[i];
        if (c.file != cur) { os << "\n" << c.file << "\n"; cur = c.file; }
        const std::string shownSeverity = c.severity == "warning" ? "warn "
            : (c.severity == "info" ? "info " : "error");
        os << "  [" << shownSeverity << "] " << c.scope;
        if (c.scopeIndex >= 0) os << " [scope " << c.scopeIndex << "]";
        if (!c.component.empty())
        {
            os << " . ";
            if (c.componentIndex >= 0) os << "component[" << c.componentIndex << "] ";
            os << c.category << " '" << c.component << "'";
        }
        os << "\n         " << c.reason << ": " << c.message << "\n";
    }
    if (r.issues.empty()) os << "\nno issues found.\n";
}

static void renderValidateJson(std::ostream& os, const App::ValidateReport& r)
{
    App::JsonWriter json(os);
    json.beginObject()
        .key("scanned").value(r.scanned)
        .key("passed").value(r.passed)
        .key("failed").value(r.failed)
        .key("status").value(r.status)
        .key("passFiles").value(r.passFiles)
        .key("infoFiles").value(r.infoFiles)
        .key("warningFiles").value(r.warningFiles)
        .key("errorFiles").value(r.errorFiles)
        .key("depth").value(r.deepBuild ? "build" : "static");
    if (!r.fatal.empty()) json.key("fatal").value(r.fatal);
    json.key("files").beginArray();
    for (size_t i = 0; i < r.files.size(); i++)
    {
        const App::FileReport& f = r.files[i];
        json.beginObject()
            .key("file").value(f.file)
            .key("ok").value(f.ok)
            .key("status").value(f.status)
            .key("errors").value(f.errorCount)
            .key("warnings").value(f.warningCount)
            .key("infos").value(f.infoCount);
        if (!f.loadError.empty()) json.key("loadError").value(f.loadError);
        json.endObject();
    }
    json.endArray().key("issues").beginArray();
    for (size_t i = 0; i < r.issues.size(); i++)
    {
        const App::ConfigIssue& c = r.issues[i];
        json.beginObject()
            .key("file").value(c.file)
            .key("scope").value(c.scope)
            .key("scopeType").value(c.scopeType)
            .key("scopeIndex").value(c.scopeIndex)
            .key("component").value(c.component)
            .key("category").value(c.category)
            .key("componentIndex").value(c.componentIndex)
            .key("reason").value(c.reason)
            .key("message").value(c.message)
            .key("severity").value(c.severity)
            .endObject();
    }
    json.endArray().endObject();
    os << "\n";
}

static int cmdValidate(int argc, char** argv, const char* prog)
{
    App::ValidateOptions o;
    std::string outFile, format = "txt";
    bool quiet = false;
    std::vector<std::string> positional;

    auto nextVal = [&](int& i, const char* opt) -> std::string {
        if (i + 1 >= argc) { std::fprintf(stderr, "ERROR: option %s needs a value\n", opt); std::exit(2); }
        return argv[++i];
    };
    for (int i = 2; i < argc; i++)
    {
        std::string a = argv[i];
        if      (a == "-h" || a == "--help")      { printUsage(prog); return 0; }
        else if (a == "-r" || a == "--recursive") { o.recursive = true; }
        else if (a == "--build")                  { o.deepBuild = true; }
        else if (a == "-q" || a == "--quiet")     { quiet = true; }
        else if (a == "-f" || a == "--format")    { format = nextVal(i, "-f"); }
        else if (a == "-o")                       { outFile = nextVal(i, "-o"); }
        else if (!a.empty() && a[0] == '-')
        { std::fprintf(stderr, "ERROR: unknown option '%s'\n", a.c_str()); return 2; }
        else positional.push_back(a);
    }
    if (positional.empty()) { printUsage(prog); return 2; }
    if (format != "txt" && format != "json")
    { std::fprintf(stderr, "ERROR: --format must be txt or json\n"); return 2; }
    o.path = positional[0];

    App::ValidateReport r = App::validatePath(o);

    std::ofstream ofs;
    std::ostream* os = &std::cout;
    if (!outFile.empty())
    {
        ofs.open(outFile);
        if (!ofs) { std::fprintf(stderr, "ERROR: cannot open output file: %s\n", outFile.c_str()); return 1; }
        os = &ofs;
    }
    if (format == "json") renderValidateJson(*os, r);
    else                  renderValidateTxt(*os, r, quiet);

    if (!r.fatal.empty()) { std::fprintf(stderr, "ERROR: %s\n", r.fatal.c_str()); return 1; }
    return (r.failed > 0) ? 1 : 0;      // 便于 CI 直接用退出码判断
}

// ---- report 子命令 ----
static void renderBatchList(std::ostream& os, const App::ReportResult& r)
{
    os << "experiment batches under the log directory (tag = one batch):\n\n";
    for (size_t i = 0; i < r.batches.size(); i++)
    {
        const App::ExperimentBatch& b = r.batches[i];
        os << "  --tag " << b.tag << "\n"
           << "      optimizers: " << b.optimizers.size()
           << "   problems: " << b.problems.size()
           << "   runs each: " << b.repeats
           << "   files: " << b.rsltCount;
        if (b.ragged) os << "   [ragged: run counts differ across combinations]";
        os << "\n";
        for (const std::string& diagnostic : b.runDiagnostics)
            os << "        " << diagnostic << "\n";
    }
    os << "\npick one with --tag; results from different batches are NOT comparable\n"
          "(they may come from different problems, budgets or code versions).\n";
}

static void renderTable(std::ostream& os, const App::ReportResult& r, const std::string& fmt)
{
    size_t ncol = r.subheader.empty() ? r.header.size() : r.header.size() * r.subheader.size();
    size_t sub  = r.subheader.empty() ? 1 : r.subheader.size();
    if (fmt == "txt")
    {
        // 终端可读性优先:先铺成网格算列宽再对齐输出(csv 不能这么干 —— 那是给程序读的)。
        // 注:cells[i] 可能短于 ncol(如 Rank 行每个优化器只有一个值),按等跨度摊开。
        std::vector<std::vector<std::string>> g;
        if (!r.subheader.empty())
        {
            std::vector<std::string> l2(ncol + 1, "");
            for (size_t h = 0; h < r.header.size(); h++)
                for (size_t s = 0; s < sub; s++) l2[1 + h * sub + s] = r.subheader[s];
            g.push_back(l2);
        }
        for (size_t i = 0; i < r.rows.size(); i++)
        {
            std::vector<std::string> ln(ncol + 1, "");
            ln[0] = r.rows[i];
            if (i < r.cells.size())
            {
                size_t n = r.cells[i].size();
                size_t span = (n > 0 && ncol >= n) ? ncol / n : 1;
                for (size_t c = 0; c < n && 1 + c * span <= ncol; c++) ln[1 + c * span] = r.cells[i][c];
            }
            g.push_back(ln);
        }
        std::vector<size_t> w(ncol + 1, 0);
        for (size_t i = 0; i < g.size(); i++)
            for (size_t c = 0; c <= ncol; c++) if (g[i][c].size() > w[c]) w[c] = g[i][c].size();

        // 表头**跨其统计量组居中**(不是压在第一个统计量上 —— 那会读成"ACS 只属于 mean")。
        // 组窄于表头时把组末列撑开,免得两行错位。
        std::vector<size_t> gw(r.header.size(), 0);
        for (size_t h = 0; h < r.header.size(); h++)
        {
            for (size_t s = 0; s < sub; s++) gw[h] += w[1 + h * sub + s] + 2;
            if (r.header[h].size() + 2 > gw[h])
            {
                size_t need = r.header[h].size() + 2 - gw[h];
                w[1 + h * sub + sub - 1] += need;
                gw[h] += need;
            }
        }
        os << std::string(w[0], ' ');
        for (size_t h = 0; h < r.header.size(); h++)
        {
            size_t pad = gw[h] - r.header[h].size();
            os << std::string(pad - pad / 2, ' ') << r.header[h] << std::string(pad / 2, ' ');
        }
        os << "\n";

        for (size_t i = 0; i < g.size(); i++)
        {
            for (size_t c = 0; c <= ncol; c++)
            {
                const std::string& s = g[i][c];
                if (c == 0) { os << s << std::string(w[c] - s.size(), ' '); }   // 行名左对齐
                else        { os << "  " << std::string(w[c] - s.size(), ' ') << s; }  // 数值右对齐
            }
            os << "\n";
        }
        return;
    }
    if (fmt == "csv")
    {
        const char* d = ",";
        os << d;
        for (size_t h = 0; h < r.header.size(); h++)
        {
            os << r.header[h];
            for (size_t s = 1; s < (r.subheader.empty() ? 1 : r.subheader.size()); s++) os << d;
            if (h + 1 < r.header.size()) os << d;
        }
        os << "\n";
        if (!r.subheader.empty())
        {
            os << d;
            for (size_t h = 0; h < r.header.size(); h++)
                for (size_t s = 0; s < r.subheader.size(); s++)
                { os << r.subheader[s]; if (!(h + 1 == r.header.size() && s + 1 == r.subheader.size())) os << d; }
            os << "\n";
        }
        for (size_t i = 0; i < r.rows.size(); i++)
        {
            os << r.rows[i] << d;
            if (i < r.cells.size())
            {
                size_t n = r.cells[i].size();
                size_t span = (n > 0 && ncol >= n) ? ncol / n : 1;
                for (size_t c = 0; c < n; c++)
                {
                    os << r.cells[i][c];
                    for (size_t k = 1; k < span; k++) os << d;
                    if (c + 1 < n) os << d;
                }
            }
            os << "\n";
        }
    }
    else if (fmt == "latex")
    {
        os << "\\begin{tabular}{l" << std::string(ncol, 'r') << "}\n\\toprule\n";
        for (size_t h = 0; h < r.header.size(); h++) os << " & \\multicolumn{"
            << (r.subheader.empty() ? 1 : r.subheader.size()) << "}{c}{" << r.header[h] << "}";
        os << " \\\\\n";
        if (!r.subheader.empty())
        {
            for (size_t h = 0; h < r.header.size(); h++)
                for (size_t s = 0; s < r.subheader.size(); s++) os << " & " << r.subheader[s];
            os << " \\\\\n";
        }
        os << "\\midrule\n";
        for (size_t i = 0; i < r.rows.size(); i++)
        {
            os << r.rows[i];
            if (i < r.cells.size()) for (size_t c = 0; c < r.cells[i].size(); c++) os << " & " << r.cells[i][c];
            os << " \\\\\n";
        }
        os << "\\bottomrule\n\\end{tabular}\n";
    }
    else   // json
    {
        App::JsonWriter json(os);
        json.beginObject().key("tag").value(r.chosen.tag).key("optimizers").beginArray();
        for (size_t i = 0; i < r.chosen.optimizers.size(); i++)
            json.value(r.chosen.optimizers[i]);
        json.endArray().key("problems").beginArray();
        for (size_t i = 0; i < r.chosen.problems.size(); i++)
            json.value(r.chosen.problems[i]);
        json.endArray()
            .key("repeats").value(r.chosen.repeats)
            .key("runNumbers").beginArray();
        for (int run : r.chosen.runNumbers) json.value(run);
        json.endArray().key("runDiagnostics").beginArray();
        for (const std::string& diagnostic : r.chosen.runDiagnostics)
            json.value(diagnostic);
        json.endArray()
            .key("ragged").value(r.chosen.ragged)
            .key("rsltCount").value(r.chosen.rsltCount)
            .key("statistics").beginArray();
        for (size_t i = 0; i < r.subheader.size(); i++)
            json.value(r.subheader[i]);
        json.endArray().key("rows").beginArray();
        for (size_t i = 0; i < r.rows.size(); i++)
        {
            json.beginObject().key("name").value(r.rows[i]).key("values").beginArray();
            if (i < r.cells.size())
                for (size_t c = 0; c < r.cells[i].size(); c++)
                    json.value(r.cells[i][c]);
            json.endArray().endObject();
        }
        json.endArray().key("charts").beginArray();
        for (size_t i = 0; i < r.chartFiles.size(); i++)
            json.value(r.chartFiles[i]);
        json.endArray().endObject();
        os << "\n";
    }
}

static int cmdReport(int argc, char** argv, const char* prog)
{
    App::ReportOptions o;
    std::string outFile, format = "txt";
    bool listOnly = false;
    std::vector<std::string> positional;

    auto nextVal = [&](int& i, const char* opt) -> std::string {
        if (i + 1 >= argc) { std::fprintf(stderr, "ERROR: option %s needs a value\n", opt); std::exit(2); }
        return argv[++i];
    };
    for (int i = 2; i < argc; i++)
    {
        std::string a = argv[i];
        if      (a == "-h" || a == "--help") { printUsage(prog); return 0; }
        else if (a == "--list")              { listOnly = true; }
        else if (a == "--tag")               { o.tag = nextVal(i, "--tag"); }
        else if (a == "--charts")            { o.chartsDir = nextVal(i, "--charts"); }
        else if (a == "--png")               { o.chartsPng = true; }
        else if (a == "-f" || a == "--format") { format = nextVal(i, "-f"); }
        else if (a == "-o")                  { outFile = nextVal(i, "-o"); }
        else if (!a.empty() && a[0] == '-')
        { std::fprintf(stderr, "ERROR: unknown option '%s'\n", a.c_str()); return 2; }
        else positional.push_back(a);
    }
    if (!positional.empty()) o.logPath = positional[0];
    if (format != "txt" && format != "csv" && format != "latex" && format != "json")
    { std::fprintf(stderr, "ERROR: --format must be txt, csv, latex or json\n"); return 2; }

    std::ofstream ofs;
    std::ostream* os = &std::cout;
    if (!outFile.empty())
    {
        ofs.open(outFile);
        if (!ofs) { std::fprintf(stderr, "ERROR: cannot open output file: %s\n", outFile.c_str()); return 1; }
        os = &ofs;
    }

    if (listOnly)
    {
        App::ReportResult r = App::discoverBatches(o.logPath);
        if (!r.ok) { std::fprintf(stderr, "ERROR: %s\n", r.error.c_str()); return 1; }
        renderBatchList(*os, r);
        return 0;
    }

    App::ReportResult r = App::buildReport(o);
    if (!r.ok)
    {
        if (!r.batches.empty() && o.tag.empty()) renderBatchList(*os, r);
        std::fprintf(stderr, "ERROR: %s\n", r.error.c_str());
        return 1;
    }
    renderTable(*os, r, format);
    if (!r.chartFiles.empty() && format != "json")
    {
        *os << "\ncharts written:\n";
        for (size_t i = 0; i < r.chartFiles.size(); i++) *os << "  " << r.chartFiles[i] << "\n";
    }
    return 0;
}

int main(int argc, char** argv)
{
    if (UI::isUiHostInvocation(argc, argv))
        return UI::runUiHostInvocation(argc, argv);

    if (argc == 2 && std::strcmp(argv[1], "--version") == 0)
    {
        std::printf("EvoFlow %s\n", ProductVersion::String);
        return 0;
    }

    if (argc < 2)
    {
        UI::UiLaunchResult result = UI::launchUiDetached();
        if (!result.ok)
        {
            std::fprintf(stderr, "ERROR: %s\n", result.error.c_str());
            return 1;
        }
        return 0;
    }

    std::string cmd = argv[1];
    if (cmd == "-h" || cmd == "--help") { printUsage(argv[0]); return 0; }
    if (cmd == "run") return cmdRun(argc, argv, argv[0]);
    if (cmd == "validate") return cmdValidate(argc, argv, argv[0]);
    if (cmd == "report") return cmdReport(argc, argv, argv[0]);
    if (cmd == "data") return cmdData(argc, argv, argv[0]);

    std::fprintf(stderr, "ERROR: unknown command '%s'\n", cmd.c_str());
    printUsage(argv[0]);
    return 2;
}
