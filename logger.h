//------------------------Description------------------------
// This file defines the logger of ECFC, which provides the log
// recording and output for system and each optimizer.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include<string>
#include<iomanip>
#include<fstream>
#include"basicfunc.h"
#include"predefine.h"


// 日志文件的组织和命名
// 一级文件夹：优化器命名（包括log和err文件）
// 二级文件夹：问题命名（包括rslt和prcs文件）
// 
// 不同输出参数的处理，包括输出精细程度及过程输出
// 过程输出组织到logger里面
// 精细程度由logger提供参数，解析器进行控制
// 
// 日志输出格式
// 时间（具体到秒），级别，内容，发生位置(先忽略这个)
//


namespace ECFC {
	class Logger
	{
	private:
		std::string _optimzer_name;
		std::string _problem_name;
		
		std::ofstream _exe_file;     // 总体日志文件，各级别的日志均有记录
		std::ofstream _err_file;     // 错误日志文件，只记录warning和error级别的记录
		std::ofstream _result_file;  // 结果日志文件，用于优化结果的输出
		std::ofstream _process_file; // 过程日志文件，用于优化过程的记录
		
		std::string _tag; // 生成时的时间戳or用户指定标签

		bool _full_result = false; // 结果文件是否打印全部决策量
		bool _process_log = false; // 是否打印优化过程
		bool _full_process = false; // 过程文件是否打印全部决策量
		bool _swarm_log = false; // 过程文件是否打印种群全部个体

		// 输出路径的检查与完善
		void _directoryDetection() const
		{
			if (_optimzer_name == "system")
				return;

			std::string log_path = "_log/" + _optimzer_name;
			// 一级目录检查
			dir_create(log_path);

			// 二级目录检查
			if (_problem_name != "")
			{
				log_path += "/" + _problem_name;
				dir_create(log_path);
			}
		}

		// 获得日志的路径及命名
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
		Logger(std::string name, std::string tag, bool full_print = false, bool process_print = false, bool full_process_print = false)
		{
			_optimzer_name = name;
			_problem_name = "";
			_tag = tag;
			_exe_file = std::ofstream(_get_first_prefix() + ".log");
			_err_file = std::ofstream(_get_first_prefix() + ".err");
			
			_full_result = full_print;
			_process_log = process_print;
			_full_process = full_process_print;

			_directoryDetection();
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
		}

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
			_err_file << std::left << std::setw(24) << formatTime() << std::setw(12) << "[ERROR]" << message << std::endl;
			_exe_file << std::left << std::setw(24) << formatTime() << std::setw(12) << "[ERROR]" << message << std::endl;
		}
	};

	Logger sys_logger("system", std::to_string(time(NULL)));
}
