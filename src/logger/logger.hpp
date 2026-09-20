// logger.h
// System / per-optimizer logging for EvoFlow.
// Migrated from ECFC原代码/logger.h with the following changes:
//   - basicfunc.h  -> basics modules (evoflow-sys.h for dir_create, evoflow-time.h for formatTime)
//   - dropped predefine.h (was empty)
//   - global `sys_logger` made `inline` (C++17) so it has a single definition
//     across translation units (the original header-only build relied on a
//     single TU; the refactored multi-TU build would otherwise multiply-define it)
//
// Log file organization:
//   first level  (per optimizer): .log (execution) / .err (warnings+errors)
//   second level (per problem)  : .rslt (results)  / .prcs (process trace)
// Log line format: time(second) [LEVEL] message

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <string>
#include <iomanip>
#include <fstream>
#include <iostream>   // v1.4.9:控制台回显(CLI -v)
#include <ctime>
#include <mutex>      // v2.5.3 LOG-PRCS-MULTIPOP:一把锁保护写入(主为共享 sys_logger 的并发写)
#include <filesystem>
#include <vector>
#include <set>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <limits>
#include <cctype>

#include "evoflow-sys.h"   // dir_create
#include "evoflow-time.h"  // formatTime

namespace EvoFlow
{
    namespace LoggerDetail
    {
        namespace fs = std::filesystem;

        // One coordinator is shared by the optimizer logger and all per-subpopulation
        // process sinks. This keeps numbering, staging and replacement transactional
        // without teaching Population/Subpopulation about filesystem operations.
        class RunArtifactCoordinator
        {
        private:
            struct Artifact
            {
                int run = -1;
                fs::path finalPath;
                fs::path temporaryPath;
                std::ofstream* stream = nullptr;
                bool result = false;
                bool committed = false;
            };

            struct Backup
            {
                int run = -1;
                fs::path originalPath;
                fs::path backupPath;
            };

            std::string _optimizer;
            std::string _tag;
            std::string _problem;
            fs::path _problemDirectory;
            std::string _filePrefix;
            fs::path _transactionDirectory;
            std::vector<fs::path> _locks;
            std::vector<Artifact> _artifacts;
            std::vector<Backup> _backups;
            int _start = -1;
            int _count = 0;
            bool _replace = false;
            bool _prepared = false;

            static bool startsWith(const std::string& value, const std::string& prefix)
            {
                return value.size() >= prefix.size()
                    && value.compare(0, prefix.size(), prefix) == 0;
            }

            bool parseArtifactRun(const fs::directory_entry& entry, int& run,
                                  bool includeLocks) const
            {
                const std::string name = entry.path().filename().u8string();
                if (!startsWith(name, _filePrefix)) return false;
                const std::string tail = name.substr(_filePrefix.size());
                size_t digits = 0;
                while (digits < tail.size()
                       && std::isdigit(static_cast<unsigned char>(tail[digits])))
                    digits++;
                if (digits == 0) return false;
                const std::string suffix = tail.substr(digits);
                const bool result = entry.is_regular_file() && suffix == ".rslt";
                const bool process = entry.is_regular_file()
                    && (suffix == ".prcs"
                        || (suffix.size() > 6 && suffix.front() == '_'
                            && suffix.substr(suffix.size() - 5) == ".prcs"));
                const bool lock = includeLocks && entry.is_directory()
                    && suffix == ".lock";
                if (!result && !process && !lock) return false;
                try
                {
                    const unsigned long long parsed = std::stoull(tail.substr(0, digits));
                    if (parsed > static_cast<unsigned long long>((std::numeric_limits<int>::max)()))
                        return false;
                    run = static_cast<int>(parsed);
                    return true;
                }
                catch (...) { return false; }
            }

            std::set<int> occupiedRuns(bool includeLocks = true) const
            {
                std::set<int> occupied;
                std::error_code ec;
                if (!fs::is_directory(_problemDirectory, ec)) return occupied;
                for (fs::directory_iterator it(_problemDirectory, ec), end;
                     !ec && it != end; it.increment(ec))
                {
                    int run = -1;
                    if (parseArtifactRun(*it, run, includeLocks)) occupied.insert(run);
                }
                if (ec)
                    throw std::runtime_error("cannot scan result directory: "
                        + _problemDirectory.u8string() + ": " + ec.message());
                return occupied;
            }

            std::vector<fs::path> artifactsForRun(int requestedRun) const
            {
                std::vector<fs::path> found;
                std::error_code ec;
                for (fs::directory_iterator it(_problemDirectory, ec), end;
                     !ec && it != end; it.increment(ec))
                {
                    int run = -1;
                    if (parseArtifactRun(*it, run, false) && run == requestedRun)
                        found.push_back(it->path());
                }
                if (ec)
                    throw std::runtime_error("cannot scan existing run artifacts: "
                        + _problemDirectory.u8string() + ": " + ec.message());
                std::sort(found.begin(), found.end());
                return found;
            }

            fs::path lockPath(int run) const
            {
                return _problemDirectory / fs::u8path(
                    _filePrefix + std::to_string(run) + ".lock");
            }

            void releaseLocks() noexcept
            {
                std::error_code ec;
                for (const fs::path& lock : _locks) fs::remove(lock, ec);
                _locks.clear();
            }

            bool reserveRange(int start, int count)
            {
                std::vector<fs::path> acquired;
                std::error_code ec;
                for (int offset = 0; offset < count; ++offset)
                {
                    fs::path lock = lockPath(start + offset);
                    ec.clear();
                    if (!fs::create_directory(lock, ec))
                    {
                        for (const fs::path& path : acquired) fs::remove(path, ec);
                        return false;
                    }
                    acquired.push_back(std::move(lock));
                }
                _locks = std::move(acquired);
                return true;
            }

            void makeTransactionDirectory()
            {
                const unsigned long long tick = static_cast<unsigned long long>(
                    std::chrono::steady_clock::now().time_since_epoch().count());
                const unsigned long long self = static_cast<unsigned long long>(
                    reinterpret_cast<std::uintptr_t>(this));
                std::error_code ec;
                for (unsigned attempt = 0; attempt < 100; ++attempt)
                {
                    _transactionDirectory = _problemDirectory / fs::u8path(
                        ".evoflow-txn-" + std::to_string(tick) + "-"
                        + std::to_string(self) + "-" + std::to_string(attempt));
                    ec.clear();
                    if (fs::create_directory(_transactionDirectory, ec)) return;
                }
                throw std::runtime_error("cannot create result transaction directory under: "
                    + _problemDirectory.u8string());
            }

            static void closeArtifact(Artifact& artifact)
            {
                if (!artifact.stream) return;
                artifact.stream->flush();
                artifact.stream->close();
            }

            void closeRun(int run)
            {
                for (Artifact& artifact : _artifacts)
                    if (artifact.run == run) closeArtifact(artifact);
            }

            void backupExistingRun(int run)
            {
                if (!_replace) return;
                for (const fs::path& original : artifactsForRun(run))
                {
                    Backup backup;
                    backup.run = run;
                    backup.originalPath = original;
                    backup.backupPath = _transactionDirectory / fs::u8path(
                        "backup-" + std::to_string(_backups.size()));
                    std::error_code ec;
                    fs::rename(backup.originalPath, backup.backupPath, ec);
                    if (ec)
                        throw std::runtime_error("cannot back up existing run artifact: "
                            + backup.originalPath.u8string() + ": " + ec.message());
                    _backups.push_back(std::move(backup));
                }
            }

            void rollbackArtifacts(int run, bool allRuns) noexcept
            {
                std::error_code ec;
                for (auto it = _artifacts.rbegin(); it != _artifacts.rend(); ++it)
                {
                    if (!allRuns && it->run != run) continue;
                    if (it->committed) fs::remove(it->finalPath, ec);
                    else fs::remove(it->temporaryPath, ec);
                    it->committed = false;
                }
                for (auto it = _backups.rbegin(); it != _backups.rend(); ++it)
                {
                    if (!allRuns && it->run != run) continue;
                    if (it->backupPath.empty()) continue;
                    fs::remove(it->originalPath, ec);
                    ec.clear();
                    fs::rename(it->backupPath, it->originalPath, ec);
                    if (!ec) it->backupPath.clear();
                }
            }

            void cleanupTransaction() noexcept
            {
                std::error_code ec;
                if (!_transactionDirectory.empty())
                    fs::remove_all(_transactionDirectory, ec);
                _transactionDirectory.clear();
            }

            void reset() noexcept
            {
                _artifacts.clear();
                _backups.clear();
                _start = -1;
                _count = 0;
                _replace = false;
                _prepared = false;
            }

        public:
            RunArtifactCoordinator(const std::string& optimizer,
                                   const std::string& tag)
                : _optimizer(optimizer), _tag(tag) {}

            ~RunArtifactCoordinator()
            {
                // Normal callers finish explicitly. This is a final best-effort guard
                // for construction failures or stack unwinding.
                if (_replace) rollbackArtifacts(-1, true);
                else
                {
                    std::error_code ec;
                    for (Artifact& artifact : _artifacts)
                        if (!artifact.committed) fs::remove(artifact.temporaryPath, ec);
                }
                releaseLocks();
                cleanupTransaction();
            }

            void setProblem(const std::string& problem)
            {
                if (_prepared && problem != _problem)
                    throw std::runtime_error("cannot change problem during a run transaction");
                _problem = problem;
                _problemDirectory = fs::u8path("_log") / fs::u8path(_optimizer)
                    / fs::u8path(_problem);
                _filePrefix = _optimizer + "_" + _problem + "(" + _tag + ")_";
            }

            bool prepared() const { return _prepared; }
            int start() const { return _start; }

            int prepare(int requestedStart, int count, bool replaceExisting)
            {
                if (_prepared) throw std::runtime_error("a run transaction is already active");
                if (_problem.empty()) throw std::runtime_error("run identity has no problem name");
                if (count < 1) throw std::runtime_error("run count must be positive");
                if (requestedStart < -1)
                    throw std::runtime_error("run start must be non-negative or automatic");
                if (replaceExisting && requestedStart < 0)
                    throw std::runtime_error("replace-existing requires an explicit run start");
                if (requestedStart >= 0
                    && count - 1 > (std::numeric_limits<int>::max)() - requestedStart)
                    throw std::runtime_error("run number range overflows int");

                std::error_code ec;
                fs::create_directories(_problemDirectory, ec);
                if (ec) throw std::runtime_error("cannot create result directory: "
                    + _problemDirectory.u8string() + ": " + ec.message());

                _replace = replaceExisting;
                if (requestedStart >= 0)
                {
                    const std::set<int> occupied = occupiedRuns(true);
                    if (!replaceExisting)
                    {
                        for (int offset = 0; offset < count; ++offset)
                            if (occupied.find(requestedStart + offset) != occupied.end())
                                throw std::runtime_error("run artifact already exists at index "
                                    + std::to_string(requestedStart + offset));
                    }
                    // Existing result/process files may be replaced, but another active
                    // transaction's lock must still reject this request.
                    if (!reserveRange(requestedStart, count))
                        throw std::runtime_error("run range is reserved by another process");
                    _start = requestedStart;
                }
                else
                {
                    for (int attempt = 0; attempt < 100; ++attempt)
                    {
                        const std::set<int> occupied = occupiedRuns(true);
                        if (!occupied.empty()
                            && *occupied.rbegin() == (std::numeric_limits<int>::max)())
                            throw std::runtime_error("automatic run number range overflows int");
                        int candidate = occupied.empty() ? 0 : (*occupied.rbegin() + 1);
                        if (candidate < 0
                            || count - 1 > (std::numeric_limits<int>::max)() - candidate)
                            throw std::runtime_error("automatic run number range overflows int");
                        if (reserveRange(candidate, count))
                        {
                            _start = candidate;
                            break;
                        }
                    }
                    if (_start < 0)
                        throw std::runtime_error("cannot reserve an automatic run range");
                }

                try { makeTransactionDirectory(); }
                catch (...) { releaseLocks(); throw; }
                _count = count;
                _prepared = true;
                return _start;
            }

            fs::path stage(int run, const fs::path& finalPath,
                           std::ofstream* stream, bool result)
            {
                if (!_prepared || run < _start || run >= _start + _count)
                    throw std::runtime_error("run artifact is outside the reserved range");
                Artifact artifact;
                artifact.run = run;
                artifact.finalPath = finalPath;
                artifact.temporaryPath = _transactionDirectory / fs::u8path(
                    "artifact-" + std::to_string(_artifacts.size()) + ".tmp");
                artifact.stream = stream;
                artifact.result = result;
                _artifacts.push_back(artifact);
                return artifact.temporaryPath;
            }

            void detach(std::ofstream* stream) noexcept
            {
                for (Artifact& artifact : _artifacts)
                    if (artifact.stream == stream) artifact.stream = nullptr;
            }

            void commitRun(int run)
            {
                closeRun(run);
                bool hasResult = false;
                for (const Artifact& artifact : _artifacts)
                    if (artifact.run == run && artifact.result)
                    {
                        std::error_code ec;
                        hasResult = fs::is_regular_file(artifact.temporaryPath, ec)
                            && fs::file_size(artifact.temporaryPath, ec) > 0;
                    }
                if (!hasResult)
                    throw std::runtime_error("successful run produced no result content");

                try
                {
                    backupExistingRun(run);
                    for (Artifact& artifact : _artifacts)
                    {
                        if (artifact.run != run) continue;
                        std::error_code ec;
                        if (fs::exists(artifact.finalPath, ec))
                            throw std::runtime_error("run artifact appeared during execution: "
                                + artifact.finalPath.u8string());
                        fs::rename(artifact.temporaryPath, artifact.finalPath, ec);
                        if (ec)
                            throw std::runtime_error("cannot commit run artifact: "
                                + artifact.finalPath.u8string() + ": " + ec.message());
                        artifact.committed = true;
                    }
                }
                catch (...)
                {
                    rollbackArtifacts(run, false);
                    throw;
                }
            }

            void abortRun(int run) noexcept
            {
                closeRun(run);
                std::error_code ec;
                for (Artifact& artifact : _artifacts)
                    if (artifact.run == run && !artifact.committed)
                        fs::remove(artifact.temporaryPath, ec);
            }

            void finish(bool success)
            {
                for (Artifact& artifact : _artifacts) closeArtifact(artifact);
                if (!success && _replace) rollbackArtifacts(-1, true);
                std::error_code ec;
                for (Artifact& artifact : _artifacts)
                    if (!artifact.committed) fs::remove(artifact.temporaryPath, ec);
                if (success)
                    for (const Backup& backup : _backups) fs::remove(backup.backupPath, ec);
                releaseLocks();
                cleanupTransaction();
                reset();
            }
        };
    }

    class Logger
    {
    private:
        std::string _optimzer_name;
        std::string _problem_name;

        std::ofstream _exe_file;     // execution log: all records
        std::ofstream _err_file;     // error log: warning/error records only
        std::ofstream _result_file;  // result log: optimization results
        std::ofstream _process_file; // process log: optimization process trace

        std::string _tag; // run timestamp or user-specified tag

        bool _full_result  = false; // print full result content
        bool _process_log  = false; // print optimization process
        bool _full_process = false; // print full process content
        bool _swarm_log    = false; // print full swarm content
        bool _console_echo = false; // v1.4.9:除写 .prcs 文件外,同时把过程行回显到控制台(CLI -v)

        // v2.5.3 LOG-PRCS-MULTIPOP
        bool _process_only = false;   // 仅过程模式:不开 .log/.err(子群自持 Logger 用),避免 per-子群 .log/.err 噪声
        std::string _sub_id = "";     // 子群判别位:非空则 .prcs 文件名追加 _<sub_id>(子群级 sink)
        std::mutex  _mtx;             // 保护所有写入;真正吃紧的是共享的 sys_logger(多线程写 .err),per-实例几乎无竞争
        std::shared_ptr<LoggerDetail::RunArtifactCoordinator> _run_artifacts;

        // create the output directories on demand
        void _directoryDetection() const
        {
            if (_optimzer_name == "system")
            {
                // LOG-DIRORDER:system 日志直落 _log/system(tag).*,需确保 _log 存在,
                //   否则全新工作区首次(尚无任何优化器建过 _log)sys_logger 落盘即失败且不恢复。
                dir_create("_log");
                return;
            }

            std::string log_path = "_log/" + _optimzer_name;
            // first-level directory
            dir_create(log_path);

            // second-level directory
            if (_problem_name != "")
            {
                log_path += "/" + _problem_name;
                dir_create(log_path);
            }
        }

        // result-log path prefixes
        std::string _get_first_prefix() const
        {
            if (_optimzer_name == "system")
            {
                return "_log/" + _optimzer_name + "(" + _tag + ")";
            }

            return "_log/" + _optimzer_name + "/"
                + _optimzer_name + "(" + _tag + ")";
        }

        std::string _get_second_prefix() const
        {
            return "_log/" + _optimzer_name + "/" + _problem_name + "/"
                + _optimzer_name + "_" + _problem_name + "(" + _tag + ")";
        }

        Logger(std::string name, std::string tag, bool full_print,
               bool process_print, bool full_process_print, bool console_echo,
               bool swarm_print, bool process_only,
               const std::shared_ptr<LoggerDetail::RunArtifactCoordinator>& coordinator)
        {
            _swarm_log = swarm_print;   // v2.2.2.3:此前构造函数根本不收该参 ⇒ 整群分支恒不可达(死代码)
            _optimzer_name = name;
            _problem_name = "";
            _tag = tag;
            _console_echo = console_echo;
            _process_only = process_only;   // v2.5.3:子群自持 Logger 用 → 只写 .prcs,不开 .log/.err
            _run_artifacts = coordinator ? coordinator
                : std::make_shared<LoggerDetail::RunArtifactCoordinator>(name, tag);

            // LOG-DIRORDER 修:原始/稳定版都先开 .log/.err 再建目录 → 首次用新优化器名时 _log/<name>/ 不存在,
            //   两个 ofstream 打开即失败且永不恢复(.log/.err 全丢,info/error 落空)。故先建目录再开流。
            _directoryDetection();
            if (!_process_only)   // v2.5.3:仅过程模式不开执行/错误流(否则每子群平白多出 .log/.err)
            {
                // Execution/error logs are an audit trail shared by repeated App/Python
                // invocations. They must never be truncated when the same identity is rebuilt.
                _exe_file = std::ofstream(_get_first_prefix() + ".log", std::ios::app);
                _err_file = std::ofstream(_get_first_prefix() + ".err", std::ios::app);
            }

            _full_result = full_print;
            _process_log = process_print;
            _full_process = full_process_print;
        }

    public:
        Logger(std::string name, std::string tag, bool full_print = false,
               bool process_print = false, bool full_process_print = false,
               bool console_echo = false, bool swarm_print = false,
               bool process_only = false)
            : Logger(name, tag, full_print, process_print, full_process_print,
                     console_echo, swarm_print, process_only, nullptr) {}

        ~Logger()
        {
            _exe_file.close();
            _err_file.close();
            _result_file.close();
            _process_file.close();
            if (_run_artifacts)
            {
                _run_artifacts->detach(&_result_file);
                _run_artifacts->detach(&_process_file);
            }
        }

        void setProblem(std::string problem_name)
        {
            _problem_name = problem_name;
            _run_artifacts->setProblem(problem_name);
            _result_file.close();
            if (_process_log)
            {
                _process_file.close();
            }

            _directoryDetection();
        }

        void newOptimization(int exe_counter, const std::string& header_extra = "")
        {
            if (!_process_only)   // v2.5.3:子群自持 Logger 只写过程,不产 .rslt
            {
                _result_file.close();
                const std::string final = _get_second_prefix() + "_"
                    + std::to_string(exe_counter) + ".rslt";
                if (_run_artifacts->prepared())
                {
                    const LoggerDetail::fs::path staged = _run_artifacts->stage(
                        exe_counter, LoggerDetail::fs::u8path(final), &_result_file, true);
                    _result_file.open(staged, std::ios::out | std::ios::trunc);
                }
                else
                {
                    // Direct Logger probes are internal test/debug callers without an
                    // Optimizer batch lifecycle. Preserve their immediate-file contract.
                    _result_file.open(final, std::ios::out | std::ios::trunc);
                }
            }

            if (_process_log)
            {
                _process_file.close();
                // v2.5.3:子群级 .prcs 文件名带 _<子群id> 判别位(全局级 _sub_id 为空 → 沿用 _<run>.prcs)
                std::string suffix = _sub_id.empty() ? "" : ("_" + _sub_id);
                const std::string final = _get_second_prefix() + "_"
                    + std::to_string(exe_counter) + suffix + ".prcs";
                if (_run_artifacts->prepared())
                {
                    const LoggerDetail::fs::path staged = _run_artifacts->stage(
                        exe_counter, LoggerDetail::fs::u8path(final), &_process_file, false);
                    _process_file.open(staged, std::ios::out | std::ios::trunc);
                }
                else _process_file.open(final, std::ios::out | std::ios::trunc);
                _processHeader(exe_counter, header_extra);   // v2.2.2.2 LOG-PRCS-FORMAT:开流即落版本头
            }
        }

        int prepareRunBatch(int requested_start, int count, bool replace_existing)
        {
            if (_process_only)
                throw std::runtime_error("a process-only logger cannot prepare a run batch");
            return _run_artifacts->prepare(requested_start, count, replace_existing);
        }

        void commitOptimization(int run) { _run_artifacts->commitRun(run); }
        void abortOptimization(int run) noexcept { _run_artifacts->abortRun(run); }
        void finishRunBatch(bool success) { _run_artifacts->finish(success); }

        bool full_result() const
        {
            return _full_result;
        }

        bool swarm_print() const
        {
            return _swarm_log;
        }

        bool process_print() const
        {
            return _process_log;
        }

        bool full_process_print() const
        {
            return _full_process;
        }

        void logprocess(std::string message)
        {
            std::lock_guard<std::mutex> lk(_mtx);   // v2.5.3:统一走锁(过程写;per-实例几乎无竞争)
            _process_file << message << std::endl;
            if (_console_echo) std::cout << message << std::endl;   // v1.4.9:-v 进度回显
        }

        // v2.2.2.2 LOG-PRCS-FORMAT:过程日志的【元数据行】(记录头 `#rec …`)。
        //   与 logprocess 的唯一区别 = 【不走 -v 回显】—— -v 是给人看进度的通道,
        //   塞元数据会把它变吵,故 -v 的观感与结构化之前完全一致。
        void logProcessMeta(std::string message)
        {
            std::lock_guard<std::mutex> lk(_mtx);   // v2.5.3:统一走锁
            _process_file << message << std::endl;
        }

        void setSwarmPrint(bool on) { _swarm_log = on; }     // v2.2.2.3:与其它标志对称
        void setConsoleEcho(bool on) { _console_echo = on; }
        bool console_echo() const { return _console_echo; }
        void setSubId(const std::string& id) { _sub_id = id; }   // v2.5.3:子群级 .prcs 文件名判别位

        // v2.5.3 LOG-PRCS-MULTIPOP:派生一个【仅过程】子 sink(子群自持),继承本 Logger 的
        //   name/tag 与全部打印开关,只写自己的 .prcs(sub_id 定文件名后缀)、不开 .log/.err。
        //   调用方(builder)拥有返回的 Logger,交由 Subpopulation 持有并析构。
        Logger* spawnProcessSink(const std::string& sub_id) const
        {
            Logger* s = new Logger(_optimzer_name, _tag, _full_result, _process_log,
                                   _full_process, _console_echo, _swarm_log,
                                   /*process_only=*/true, _run_artifacts);
            s->setSubId(sub_id);
            return s;
        }

        // v2.2.2.2:`.prcs` 文件头。对齐 `.rslt` 的 `<ver= 1.X >` 既有范式。
        //   【无版本行 = 旧版裸行流】,读取端据此天然分派,无需额外标记。
        void _processHeader(int exe_counter, const std::string& extra = "")
        {
            // v1.2(v2.5.3 LOG-PRCS-MULTIPOP):过程日志两级化 —— 文件区分【子群级】(逐代,每子群一文件)
            //   与【全局级】(逐 epoch,全局 best+统一 FES);头部经 `extra` 带 Level/Swarm/SwarmNumber 等元信息。
            //   为何升版:1.1 只有单一 .prcs 流,读取端无从区分两级;结构又变 ⇒ 按既有版本分派范式升 1.2。
            // v1.1(v2.2.2.3):记录内 p 块前加分割线。整群块在 1.0 里不可达(_swarm_log 通路不存在)⇒
            //   已产出 1.0 文件必不含 p 块;本批首次让它可达且结构又变,两种结构同叫 1.0 正是版本字段该防的歧义。
            _process_file << "<ver= 1.2 >" << std::endl;
            // Full_Process 必须进头:full_process=1 时 decoder->toString 产出的是【多行块】
            //   (目标值行 + 逐变量行 + "EOS",同 .rslt),角色前缀只盖得住首行 ⇒
            //   读取端须据此决定"按行解析"还是"读到 EOS 为一块"。
            _process_file << "Optimizer:\t" << _optimzer_name
                          << "\tTag:\t" << _tag
                          << "\tProblem:\t" << _problem_name
                          << "\tRun:\t" << exe_counter
                          << "\tFull_Process:\t" << (_full_process ? 1 : 0)
                          << extra << std::endl;   // v2.5.3:Level/Swarm/SwarmSize(子群级)或 Level/Manager/SwarmNumber(全局级)
            _process_file << "----------------------------------------" << std::endl;
        }

        void logresult(std::string message)
        {
            if (_process_only) return;   // v2.5.3:子群自持 Logger 不写结果
            std::lock_guard<std::mutex> lk(_mtx);
            _result_file << message << std::endl;
        }

        void info(std::string message)
        {
            if (_process_only) return;   // v2.5.3:仅过程模式无 .log/.err
            std::lock_guard<std::mutex> lk(_mtx);   // v2.5.3:保护共享 sys_logger 的并发写
            _exe_file << std::left << std::setw(24) << formatTime() << std::setw(12) << "[INFO]" << message << std::endl;
        }

        void debug(std::string message)
        {
            if (_process_only) return;
            std::lock_guard<std::mutex> lk(_mtx);
            _err_file << std::left << std::setw(24) << formatTime() << std::setw(12) << "[DEBUG]" << message << std::endl;
        }

        void warning(std::string message)
        {
            if (_process_only) return;
            std::lock_guard<std::mutex> lk(_mtx);
            _err_file << std::left << std::setw(24) << formatTime() << std::setw(12) << "[WARN]" << message << std::endl;
            _exe_file << std::left << std::setw(24) << formatTime() << std::setw(12) << "[WARN]" << message << std::endl;
        }

        void error(std::string message)
        {
            if (_process_only) return;
            std::lock_guard<std::mutex> lk(_mtx);
            // std::endl 即 flush → .err/.log 立即落盘,崩溃/异常前的错误不丢(LOG-DETAIL 崩溃安全)
            _err_file << std::left << std::setw(24) << formatTime() << std::setw(12) << "[ERROR]" << message << std::endl;
            _exe_file << std::left << std::setw(24) << formatTime() << std::setw(12) << "[ERROR]" << message << std::endl;
        }

        // 显式刷新全部日志文件(关键节点/异常前可主动调用)
        void flush()
        {
            _exe_file.flush(); _err_file.flush(); _result_file.flush(); _process_file.flush();
        }
    };

    // Global system logger (inline => single definition across all TUs).
    inline Logger sys_logger("system", std::to_string(time(NULL)));
}
