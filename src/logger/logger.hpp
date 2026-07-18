// logger.h
// System / per-optimizer logging for ECFlow.
//   - basicfunc.h  -> basics modules (ecflow-sys.h for dir_create, ecflow-time.h for formatTime)
//   - dropped predefine.h (was empty)
//   - global `sys_logger` made `inline` (C++17) so it has a single definition
//     across translation units (the original header-only build relied on a
//     single TU; the refactored multi-TU build would otherwise multiply-define it)
//
// Log file organization:
//   first level  (per optimizer): .log (execution) / .err (warnings+errors)
//   second level (per problem)  : .rslt (results)  / .prcs (process trace)
// Log line format: time(second) [LEVEL] message
#pragma once
#include <string>
#include <iomanip>
#include <fstream>
#include <iostream>   // v1.4.9:控制台回显(CLI -v)
#include <ctime>

#include "ecflow-sys.h"   // dir_create
#include "ecflow-time.h"  // formatTime

namespace ECFlow
{
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

    public:
        Logger(std::string name, std::string tag, bool full_print = false, bool process_print = false, bool full_process_print = false, bool console_echo = false)
        {
            _optimzer_name = name;
            _problem_name = "";
            _tag = tag;
            _console_echo = console_echo;

            // LOG-DIRORDER 修:原始/稳定版都先开 .log/.err 再建目录 → 首次用新优化器名时 _log/<name>/ 不存在,
            //   两个 ofstream 打开即失败且永不恢复(.log/.err 全丢,info/error 落空)。故先建目录再开流。
            _directoryDetection();
            _exe_file = std::ofstream(_get_first_prefix() + ".log");
            _err_file = std::ofstream(_get_first_prefix() + ".err");

            _full_result = full_print;
            _process_log = process_print;
            _full_process = full_process_print;
        }

        ~Logger()
        {
            _exe_file.close();
            _err_file.close();
            _result_file.close();
            _process_file.close();
        }

        void setProblem(std::string problem_name)
        {
            _problem_name = problem_name;
            _result_file.close();
            if (_process_log)
            {
                _process_file.close();
            }

            _directoryDetection();
        }

        void newOptimization(int exe_counter)
        {
            _result_file.close();
            _result_file = std::ofstream(_get_second_prefix() + "_" + std::to_string(exe_counter) + ".rslt");

            if (_process_log)
            {
                _process_file.close();
                _process_file = std::ofstream(_get_second_prefix() + "_" + std::to_string(exe_counter) + ".prcs");
            }
        }

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
            _process_file << message << std::endl;
            if (_console_echo) std::cout << message << std::endl;   // v1.4.9:-v 进度回显
        }

        void setConsoleEcho(bool on) { _console_echo = on; }
        bool console_echo() const { return _console_echo; }

        void logresult(std::string message)
        {
            _result_file << message << std::endl;
        }

        void info(std::string message)
        {
            _exe_file << std::left << std::setw(24) << formatTime() << std::setw(12) << "[INFO]" << message << std::endl;
        }

        void debug(std::string message)
        {
            _err_file << std::left << std::setw(24) << formatTime() << std::setw(12) << "[DEBUG]" << message << std::endl;
        }

        void warning(std::string message)
        {
            _err_file << std::left << std::setw(24) << formatTime() << std::setw(12) << "[WARN]" << message << std::endl;
            _exe_file << std::left << std::setw(24) << formatTime() << std::setw(12) << "[WARN]" << message << std::endl;
        }

        void error(std::string message)
        {
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
