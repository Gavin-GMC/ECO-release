//   文件路径与内存文本最终都进入 validateLoadedConfig;matcher、原因键、定位索引仅一套。

// SPDX-License-Identifier: BSD-3-Clause

#include "config-validator.h"
#include "component-catalog.h"
#include "evoflow.h"
#include "assert-matcher.h"
#include "param-matcher.h"
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <sstream>

namespace EvoFlow
{
    namespace App
    {
        namespace fs = std::filesystem;

        namespace
        {
            int statusRank(const std::string& status)
            {
                if (status == "error") return 3;
                if (status == "warning") return 2;
                if (status == "info") return 1;
                return 0;
            }

            void promoteStatus(std::string& current, const std::string& candidate)
            {
                if (statusRank(candidate) > statusRank(current)) current = candidate;
            }

            void appendIssues(const ValidateResult& vr, const std::string& file,
                              const std::string& scope, const std::string& scopeType,
                              int scopeIndex,
                              ValidateReport& rep, int& errs, int& warns, int& infos)
            {
                for (size_t i = 0; i < vr.issues.size(); i++)
                {
                    const ValidateIssue& s = vr.issues[i];
                    ConfigIssue c;
                    c.file = file;
                    c.scope = scope;
                    c.scopeType = scopeType;
                    c.scopeIndex = scopeIndex;
                    c.component = s.component;
                    c.category = App::categoryKeyOf(s.type);
                    c.componentIndex = s.componentIndex;
                    c.reason = s.reason;
                    c.message = s.message;
                    c.severity = validationLevelName(s.severity);
                    c.isError = s.isError;
                    rep.issues.push_back(c);
                    if (s.severity == ValidationLevel::Error) errs++;
                    else if (s.severity == ValidationLevel::Warning) warns++;
                    else if (s.severity == ValidationLevel::Info) infos++;
                }
            }

            FullConfig decodeConfigText(const std::string& text)
            {
                const std::string version = ConfigBuilder::detectVersion(text);
                if (version == "v3.1") return ConfigCodecV31::decode(text);
                if (version == "v3.2") return ConfigCodecV32::decode(text);
                throw std::runtime_error("[config] unsupported/undetected version: '" + version + "'");
            }

            void appendLoadFailure(const std::string& shown, const std::string& message,
                                   ValidateReport& rep)
            {
                FileReport fr;
                fr.file = shown;
                fr.ok = false;
                fr.errorCount = 1;
                fr.status = "error";
                fr.loadError = message;
                rep.files.push_back(fr);
                rep.failed++;
                rep.errorFiles++;
                promoteStatus(rep.status, "error");

                ConfigIssue c;
                c.file = shown;
                c.scope = "(load)";
                c.scopeType = "load";
                c.reason = "config_load_failed";
                c.message = message;
                c.severity = "error";
                c.isError = true;
                rep.issues.push_back(c);
            }

            void validateLoadedConfig(const FullConfig& fc, const std::string& shown,
                                      bool deepBuild, ValidateReport& rep)
            {
                FileReport fr;
                fr.file = shown;
                int errs = 0, warns = 0, infos = 0;

                for (size_t w = 0; w < fc.workflows.size(); w++)
                {
                    const WorkflowConfig& wf = fc.workflows[w];
                    const std::string scope = "workflow '" + wf.tag + "'";
                    appendIssues(AssertMatcher::validate(wf), shown, scope, "workflow",
                                 static_cast<int>(w), rep, errs, warns, infos);
                    appendIssues(ParamMatcher::validate(wf), shown, scope, "workflow",
                                 static_cast<int>(w), rep, errs, warns, infos);
                }

                for (size_t s = 0; s < fc.optimizer.subpopulations.size(); s++)
                {
                    const SubpopulationConfig& sp = fc.optimizer.subpopulations[s];
                    const std::string scope = "subpopulation '" + sp.tag + "'";
                    appendIssues(ParamMatcher::validate(sp), shown, scope, "subpopulation",
                                 static_cast<int>(s), rep, errs, warns, infos);
                }

                if (deepBuild)
                {
                    try
                    {
                        OptimizerBuilder b;
                        for (size_t w = 0; w < fc.workflows.size(); w++)
                            b.registerWorkflow(fc.workflows[w]);
                        Optimizer* optimizer = b.buildOptimizer(fc.optimizer);
                        delete optimizer;
                    }
                    catch (const std::exception& ex)
                    {
                        ConfigIssue c;
                        c.file = shown;
                        c.scope = "(build)";
                        c.scopeType = "build";
                        c.reason = "build_failed";
                        c.message = ex.what();
                        c.severity = "error";
                        c.isError = true;
                        rep.issues.push_back(c);
                        errs++;
                    }
                    catch (...)
                    {
                        ConfigIssue c;
                        c.file = shown;
                        c.scope = "(build)";
                        c.scopeType = "build";
                        c.reason = "build_failed";
                        c.message = "unknown error during assembly";
                        c.severity = "error";
                        c.isError = true;
                        rep.issues.push_back(c);
                        errs++;
                    }
                }

                fr.errorCount = errs;
                fr.warningCount = warns;
                fr.infoCount = infos;
                fr.ok = (errs == 0);
                if (errs > 0) fr.status = "error";
                else if (warns > 0) fr.status = "warning";
                else if (infos > 0) fr.status = "info";
                else fr.status = "pass";
                rep.files.push_back(fr);
                if (fr.ok) rep.passed++; else rep.failed++;
                if (fr.status == "error") rep.errorFiles++;
                else if (fr.status == "warning") rep.warningFiles++;
                else if (fr.status == "info") rep.infoFiles++;
                else rep.passFiles++;
                promoteStatus(rep.status, fr.status);
            }

            bool collectFiles(const ValidateOptions& opt, std::vector<fs::path>& out,
                              std::string& fatal)
            {
                std::error_code ec;
                const fs::path path = fs::u8path(opt.path);
                if (!fs::exists(path, ec))
                {
                    fatal = "path does not exist: " + opt.path;
                    return false;
                }
                if (fs::is_regular_file(path, ec))
                {
                    out.push_back(path);
                    return true;
                }
                if (!fs::is_directory(path, ec))
                {
                    fatal = "not a file or directory: " + opt.path;
                    return false;
                }

                const auto take = [&](const fs::path& file)
                {
                    if (file.extension() == ".cfg" || file.extension() == ".json")
                        out.push_back(file);
                };
                if (opt.recursive)
                {
                    for (const auto& entry : fs::recursive_directory_iterator(path, ec))
                        if (entry.is_regular_file()) take(entry.path());
                }
                else
                {
                    for (const auto& entry : fs::directory_iterator(path, ec))
                        if (entry.is_regular_file()) take(entry.path());
                }

                std::sort(out.begin(), out.end());
                if (out.empty())
                {
                    fatal = "no .cfg/.json config found under: " + opt.path;
                    return false;
                }
                return true;
            }

            std::string readTextFile(const fs::path& path)
            {
                std::ifstream input(path, std::ios::binary);
                if (!input)
                    throw std::runtime_error("[config] file not found: " + path.string());
                return std::string((std::istreambuf_iterator<char>(input)),
                                   std::istreambuf_iterator<char>());
            }
        }

        ValidateReport validateConfigText(const std::string& text, bool deepBuild,
                                          const std::string& fileName)
        {
            ValidateReport rep;
            rep.deepBuild = deepBuild;
            rep.scanned = 1;
            try
            {
                const FullConfig config = decodeConfigText(text);
                validateLoadedConfig(config, fileName, deepBuild, rep);
            }
            catch (const std::exception& ex)
            {
                appendLoadFailure(fileName, ex.what(), rep);
            }
            catch (...)
            {
                appendLoadFailure(fileName, "unknown error while loading", rep);
            }
            return rep;
        }

        ValidateReport validatePath(const ValidateOptions& opt)
        {
            ValidateReport rep;
            rep.deepBuild = opt.deepBuild;

            std::vector<fs::path> files;
            if (!collectFiles(opt, files, rep.fatal))
            {
                if (!rep.fatal.empty()) rep.status = "error";
                return rep;
            }

            for (size_t i = 0; i < files.size(); i++)
            {
                const std::string shown = files[i].filename().string();
                try
                {
                    ValidateReport one =
                        validateConfigText(readTextFile(files[i]), opt.deepBuild, shown);
                    rep.scanned += one.scanned;
                    rep.passed += one.passed;
                    rep.failed += one.failed;
                    rep.errorFiles += one.errorFiles;
                    rep.warningFiles += one.warningFiles;
                    rep.infoFiles += one.infoFiles;
                    rep.passFiles += one.passFiles;
                    promoteStatus(rep.status, one.status);
                    rep.files.insert(rep.files.end(), one.files.begin(), one.files.end());
                    rep.issues.insert(rep.issues.end(), one.issues.begin(), one.issues.end());
                }
                catch (const std::exception& ex)
                {
                    rep.scanned++;
                    appendLoadFailure(shown, ex.what(), rep);
                }
            }
            return rep;
        }
    }
}
