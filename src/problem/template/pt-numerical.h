//------------------------Description------------------------
// 数值优化 (Numerical Optimization) 问题模板。
//-------------------------Reference-------------------------
// CEC2013 Niching competition benchmark (CEC2013_N)。
// CEC2013 Large-Scale Global Optimization benchmark (CEC2013_L)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include"problem.h"
#include<memory>
#include<sstream>
#include<fstream>
#include<string>
#include<any>
#include"logger.hpp"
#include"pt-instance-path.h"   // v1.4.9:描述文件 load 的路径自适应
#include"../../_pdata/numerical/CEC2013_N/cec2013.h"
#include"../../_pdata/numerical/CEC2013_L/cec2013-large.h"

// ★ 抑制第三方宏污染:CEC2013 竞赛代码把一批**普通标识符**定义成了全局宏 ——
//   cfunction.h `#define INF MAXDOUBLE`、values.h `#define MAXDOUBLE ...`、
//   CEC2013_L/Benchmarks.h `#define E ...` / `#define PI ...`。
//   它们会把其它模板里名为 INF/E/PI 的**局部变量或常量**(pt-cvrp/pt-fjsp 的 `static constexpr double INF`、
//   pt-wfs 的 `int E`)在预处理期展开成数值 → C2059/C2679。长期潜伏,直到 v1.4.9 把全部模板汇入
//   problem-template.h 才与 numerical 相遇暴露。这些宏仅供 CEC 各 .cpp 独立 TU 使用、本头链的内联代码不引用,
//   故在此汇总边界一律撤销、令污染不逸出 pt-numerical.h。(数值上限类宏 MAX*/MIN* 名字生僻,暂不撤;若日后有
//   模板变量与之重名再补。记 [未来规划 CEC-MACRO-POLLUTION]。)
#ifdef INF
#undef INF
#endif
#ifdef MAXDOUBLE
#undef MAXDOUBLE
#endif
#ifdef E
#undef E
#endif
#ifdef PI
#undef PI
#endif

namespace ECFlow
{
	// template of numerical Optimization
	class PT_Numerical
	{
	public:
		enum class Benmarks { P_default, CEC2013_N, CEC2013_L, P_end };

	private:
		std::string _name;
		Benmarks _benckmark;
		int fid;
		std::shared_ptr<void> _ptr;

		struct evaluateFunc : eccalcul_functor
		{
			std::shared_ptr<void> _pointer;
			Benmarks _type;
			int _v_number;
			double* evaluate_buffer;

			evaluateFunc(Benmarks type, std::shared_ptr<void> func_pointer, int v_number)
			{
				_type = type;
				_pointer = func_pointer;
				_v_number = v_number;
				
				if (_v_number > 1)
					evaluate_buffer = new double[_v_number + 1];
				else
					evaluate_buffer = nullptr;
			}

			~evaluateFunc()
			{
				_pointer.reset();
				delete[] evaluate_buffer;
			}

			double operator()(double** a) const
			{
				switch (_type)
				{
				case ECFlow::PT_Numerical::Benmarks::CEC2013_N:
					if (_v_number == 1)
					{
						return static_cast<CEC2013*>(_pointer.get())->evaluate(a[0]);
					}
					else
					{
						for (int i = 0; i < _v_number; i++)
							evaluate_buffer[i] = a[i][0];
						return static_cast<CEC2013*>(_pointer.get())->evaluate(evaluate_buffer);
					}
                case ECFlow::PT_Numerical::Benmarks::CEC2013_L:
                {
                    static_cast<CEC13L::Benchmarks*>(_pointer.get())->nextRun();
                    return static_cast<CEC13L::Benchmarks*>(_pointer.get())->compute(a[0]);
                }
				case ECFlow::PT_Numerical::Benmarks::P_default:
				case ECFlow::PT_Numerical::Benmarks::P_end:
				default:
					sys_logger.error("benchmark name not exist!");
					return EMPTYVALUE;
				}
			}

			eccalcul_functor* copy()
			{
				return new evaluateFunc(_type, _pointer, _v_number);
			}
		};

		evaluateFunc* _evaluate;

	public:
		PT_Numerical()
		{
			_name = "numerical";
			_evaluate = nullptr;
		}

		~PT_Numerical()
		{
			delete _evaluate;
			_ptr.reset();
		}

		void setName(std::string name)
		{
			_name = name;
		}

		Problem* getProblem()
		{
			// 问题定义
			Problem* back = new Problem(_name);

			switch (_benckmark)
			{
			case ECFlow::PT_Numerical::Benmarks::CEC2013_N:
			{
				// 统计并添加变量
				int variable_number = 1;
				switch (fid)
				{
				case 1:
					variable_number = 1;
					back->addVariable("x", 0, 30, 1e-5, 1, 1, VariableType::continuous);
					break;
				case 2:
					variable_number = 1;
					back->addVariable("x", 0, 1, 1e-5, 1, 1, VariableType::continuous);
					break;
				case 3:
					variable_number = 1;
					back->addVariable("x", 0, 1, 1e-5, 1, 1, VariableType::continuous);
					break;
				case 4:
					variable_number = 1;
					back->addVariable("x", -6, 6, 1e-5, 2, 1, VariableType::continuous);
					break;
				case 5:
					variable_number = 2;
					back->addVariable("x", -1.9, 1.9, 1e-5, 1, 1, VariableType::continuous);
					back->addVariable("y", -1.1, 1.1, 1e-5, 1);
					break;
				case 6:
					variable_number = 1;
					back->addVariable("x", -10, 10, 1e-5, 2, 1, VariableType::continuous);
					break;
				case 7:
					variable_number = 1;
					back->addVariable("x", 0.25, 10, 1e-5, 2, 1, VariableType::continuous);
					break;
				case 8:
					variable_number = 1;
					back->addVariable("x", -10, 10, 1e-5, 3, 1, VariableType::continuous);
					break;
				case 9:
					variable_number = 1;
					back->addVariable("x", 0.25, 10, 1e-5, 3, 1, VariableType::continuous);
					break;
				case 10:
					variable_number = 1;
					back->addVariable("x", 0, 1, 1e-5, 2, 1, VariableType::continuous);
					break;
				case 11:
				case 12:
				case 13:
					variable_number = 1;
					back->addVariable("x", -5, 5, 1e-5, 2, 1, VariableType::continuous);
					break;
				case 14:
				case 15:
					variable_number = 1;
					back->addVariable("x", -5, 5, 1e-5, 3, 1, VariableType::continuous);
				case 16:
				case 17:
					variable_number = 1;
					back->addVariable("x", -5, 5, 1e-5, 5, 1, VariableType::continuous);
				case 18:
				case 19:
					variable_number = 1;
					back->addVariable("x", -5, 5, 1e-5, 10, 1, VariableType::continuous);
				case 20:
					variable_number = 1;
					back->addVariable("x", -5, 5, 1e-5, 20, 1, VariableType::continuous);
				default:
					break;
				}		
				// 添加目标函数
				delete _evaluate;
				_evaluate = new evaluateFunc(Benmarks::CEC2013_N, _ptr, variable_number);
				if (fid == 5)
					back->addObjective("fitness", 1, false, "x,y", _evaluate);
				else
					back->addObjective("fitness", 1, false, "x", _evaluate);

				// 添加约束
				if (fid == 5)
				{
					back->addConstrainRange("x", ECFlow::EMPTYVALUE, ECFlow::EMPTYVALUE, 1e6, "fitness");
					back->addConstrainRange("y", ECFlow::EMPTYVALUE, ECFlow::EMPTYVALUE, 1e6, "fitness");
				}
				else
					back->addConstrainRange("x", ECFlow::EMPTYVALUE, ECFlow::EMPTYVALUE, 1e6, "fitness");
				
				break;
			}
            case ECFlow::PT_Numerical::Benmarks::CEC2013_L:
            {
                // 统计并添加变量
                int variable_number = 1;
                switch (fid)
                {
                case 1:
                    variable_number = 1;
                    back->addVariable("x", -100, 100, 1e-5, 1000, 1, VariableType::continuous);
                    break;
                case 2:
                    variable_number = 1;
                    back->addVariable("x", -5, 5, 1e-5, 1000, 1, VariableType::continuous);
                    break;
                case 3:
                    variable_number = 1;
                    back->addVariable("x", -32, 32, 1e-5, 1000, 1, VariableType::continuous);
                    break;
                case 4:
                    variable_number = 1;
                    back->addVariable("x", -100, 100, 1e-5, 1000, 1, VariableType::continuous);
                    break;
                case 5:
                    variable_number = 1;
                    back->addVariable("x", -5, 5, 1e-5, 1000, 1, VariableType::continuous);
                    break;
                case 6:
                    variable_number = 1;
                    back->addVariable("x", -32, 32, 1e-5, 1000, 1, VariableType::continuous);
                    break;
                case 7:
                case 8:
                    variable_number = 1;
                    back->addVariable("x", -100, 100, 1e-5, 1000, 1, VariableType::continuous);
                    break;
                case 9:
                    variable_number = 1;
                    back->addVariable("x", -5, 5, 1e-5, 1000, 1, VariableType::continuous);
                    break;
                case 10:
                    variable_number = 1;
                    back->addVariable("x", -32, 32, 1e-5, 1000, 1, VariableType::continuous);
                    break;
                case 11:
                case 12:
                case 13:
                case 14:
                case 15:
                    variable_number = 1;
                    back->addVariable("x", -100, 100, 1e-5, 1000, 1, VariableType::continuous);
                default:
                    break;
                }
                // 添加目标函数
                delete _evaluate;
                _evaluate = new evaluateFunc(Benmarks::CEC2013_L, _ptr, variable_number);
                back->addObjective("fitness", 1, true, "x", _evaluate);
                    
                // 添加约束
                back->addConstrainRange("x", ECFlow::EMPTYVALUE, ECFlow::EMPTYVALUE, 1e6, "fitness");

                break;
            }
			case ECFlow::PT_Numerical::Benmarks::P_default:
			case ECFlow::PT_Numerical::Benmarks::P_end:
			default:
				sys_logger.error("The numerical benchmark is not loaded!");
				return nullptr;
				break;
			}
			return back;
		}

		void load(Benmarks benchmark_name, int func_id)
		{
			clear();

			std::string b_name;
			// 类型识别与预处理
			switch (benchmark_name)
			{
			case ECFlow::PT_Numerical::Benmarks::CEC2013_N:
				b_name = "CEC2013-N";
                _benckmark = benchmark_name;
				if (func_id < 1 || func_id>20)
				{
					sys_logger.error("benchmark func_id out of range!");
					return;
				}
				fid = func_id;
				_ptr = std::make_shared<CEC2013>(fid);
				break;
            case ECFlow::PT_Numerical::Benmarks::CEC2013_L:
                b_name = "CEC2013-L";
                _benckmark = benchmark_name;
                if (func_id < 1 || func_id>15)
                {
                    sys_logger.error("benchmark func_id out of range!");
                    return;
                }
                fid = func_id;
                if (fid == 1) {
                    _ptr = std::make_shared<CEC13L::F1>();
                }
                else if (fid == 2) {
                    _ptr = std::make_shared<CEC13L::F2>();
                }
                else if (fid == 3) {
                    _ptr = std::make_shared<CEC13L::F3>();
                }
                else if (fid == 4) {
                    _ptr = std::make_shared<CEC13L::F4>();
                }
                else if (fid == 5) {
                    _ptr = std::make_shared<CEC13L::F5>();
                }
                else if (fid == 6) {
                    _ptr = std::make_shared<CEC13L::F6>();
                }
                else if (fid == 7) {
                    _ptr = std::make_shared<CEC13L::F7>();
                }
                else if (fid == 8) {
                    _ptr = std::make_shared<CEC13L::F8>();
                }
                else if (fid == 9) {
                    _ptr = std::make_shared<CEC13L::F9>();
                }
                else if (fid == 10) {
                    _ptr = std::make_shared<CEC13L::F10>();
                }
                else if (fid == 11) {
                    _ptr = std::make_shared<CEC13L::F11>();
                }
                else if (fid == 12) {
                    _ptr = std::make_shared<CEC13L::F12>();
                }
                else if (fid == 13) {
                    _ptr = std::make_shared<CEC13L::F13>();
                }
                else if (fid == 14) {
                    _ptr = std::make_shared<CEC13L::F14>();
                }
                else if (fid == 15) {
                    _ptr = std::make_shared<CEC13L::F15>();
                }
                
                break;
			case ECFlow::PT_Numerical::Benmarks::P_default:
			case ECFlow::PT_Numerical::Benmarks::P_end:
			default:
				sys_logger.error("benchmark name not exist!");
				return;
			}
			setName(b_name + "(f" + std::to_string(func_id) + ")");
		}

		//   故描述文件只含说明段(NAME/TYPE/BENCHMARK/FUNC_ID),按 BENCHMARK+FUNC_ID 选定具体函数、NAME 作问题名。
		//   路径自适应同其它模板:短名 → _pdata/numerical/<name>.num,含 `/ \ .` 则原样。
		void load(std::string name)
		{
			std::string path = resolveInstancePath(name, "numerical", "num");
			std::ifstream f(path);
			if (!f) { sys_logger.error("Numerical instance not found: " + path); return; }

			std::string tok, label, bench; int func = 0; bool hasFunc = false;
			while (f >> tok)
			{
				if      (tok == "NAME" || tok == "NAME:")           { std::getline(f, label); }
				else if (tok == "BENCHMARK" || tok == "BENCHMARK:") { f >> bench; }
				else if (tok == "FUNC_ID" || tok == "FUNC_ID:")     { f >> func; hasFunc = true; }
				else if (!tok.empty() && tok.back() == ':')         { std::string rest; std::getline(f, rest); } // 跳过 TYPE: 等其它字段
			}

			Benmarks bmk = Benmarks::P_default;
			if      (bench == "CEC2013_N" || bench == "CEC2013-N") bmk = Benmarks::CEC2013_N;
			else if (bench == "CEC2013_L" || bench == "CEC2013-L") bmk = Benmarks::CEC2013_L;
			else { sys_logger.error("Numerical '" + name + "': unknown/missing BENCHMARK '" + bench + "'"); return; }
			if (!hasFunc) { sys_logger.error("Numerical '" + name + "': missing FUNC_ID"); return; }

			load(bmk, func);   // 复用两参 load:构造 _ptr/_benckmark/fid + 设默认名

			// 用描述文件的 NAME 覆盖问题名(若提供)
			size_t a = label.find_first_not_of(" \t");
			size_t b = label.find_last_not_of(" \t\r\n");
			if (a != std::string::npos) setName(label.substr(a, b - a + 1));
		}

		void save(bool overwrite = false)
		{
			std::string path = "_pdata/numerical/" + _name + ".num";
			if (!overwrite) { std::ifstream ex(path); if (ex.good()) { sys_logger.error("Numerical save: file exists (use overwrite): " + path); return; } }

			std::string bench = (_benckmark == Benmarks::CEC2013_N) ? "CEC2013_N"
			                  : (_benckmark == Benmarks::CEC2013_L) ? "CEC2013_L" : "UNKNOWN";
			std::ofstream out(path);
			if (!out) { sys_logger.error("Numerical save: cannot write " + path); return; }
			out << "NAME: " << _name << "\n"
			    << "TYPE: NUMERICAL\n"
			    << "BENCHMARK: " << bench << "\n"
			    << "FUNC_ID: " << fid << "\n";
		}

		void clear()
		{
			_name = "numerical";
			delete _evaluate;
			_evaluate = nullptr;
			_benckmark = Benmarks::P_default;
			fid = 0;
			_ptr.reset();
		}
	};
}
