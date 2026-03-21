//------------------------Description------------------------
// This file defines the templates of classic problem for user,
//  which provides the functions to define specific problem.
// Templates including benchmark optimization, multi-knapsack problem,
// travelling salesman problem, target coverage problem, etc.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include"problem.h"
#include<memory>
#include<sstream>
#include<string>
#include<any>
#include"./_pdata/numerical/CEC2013_N/cec2013.h"
#include"./_pdata/numerical/CEC2013_L/cec2013-large.h"

namespace ECFC
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
				case ECFC::PT_Numerical::Benmarks::CEC2013_N:
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
                case ECFC::PT_Numerical::Benmarks::CEC2013_L:
                {
                    static_cast<CEC13L::Benchmarks*>(_pointer.get())->nextRun();
                    return static_cast<CEC13L::Benchmarks*>(_pointer.get())->compute(a[0]);
                }
				case ECFC::PT_Numerical::Benmarks::P_default:
				case ECFC::PT_Numerical::Benmarks::P_end:
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
			case ECFC::PT_Numerical::Benmarks::CEC2013_N:
			{
				// 统计并添加变量
				int variable_number = 1;
				switch (fid)
				{
				case 1:
					variable_number = 1;
					back->addVariable("x", 0, 30, 1e-5, 1);
					break;
				case 2:
					variable_number = 1;
					back->addVariable("x", 0, 1, 1e-5, 1);
					break;
				case 3:
					variable_number = 1;
					back->addVariable("x", 0, 1, 1e-5, 1);
					break;
				case 4:
					variable_number = 1;
					back->addVariable("x", -6, 6, 1e-5, 2);
					break;
				case 5:
					variable_number = 2;
					back->addVariable("x", -1.9, 1.9, 1e-5, 1);
					back->addVariable("y", -1.1, 1.1, 1e-5, 1);
					break;
				case 6:
					variable_number = 1;
					back->addVariable("x", -10, 10, 1e-5, 2);
					break;
				case 7:
					variable_number = 1;
					back->addVariable("x", 0.25, 10, 1e-5, 2);
					break;
				case 8:
					variable_number = 1;
					back->addVariable("x", -10, 10, 1e-5, 3);
					break;
				case 9:
					variable_number = 1;
					back->addVariable("x", 0.25, 10, 1e-5, 3);
					break;
				case 10:
					variable_number = 1;
					back->addVariable("x",0, 1, 1e-5, 2);
					break;
				case 11:
				case 12:
				case 13:
					variable_number = 1;
					back->addVariable("x", -5, 5, 1e-5, 2);
					break;
				case 14:
				case 15:
					variable_number = 1;
					back->addVariable("x", -5, 5, 1e-5, 3);
				case 16:
				case 17:
					variable_number = 1;
					back->addVariable("x", -5, 5, 1e-5, 5);
				case 18:
				case 19:
					variable_number = 1;
					back->addVariable("x", -5, 5, 1e-5, 10);
				case 20:
					variable_number = 1;
					back->addVariable("x", -5, 5, 1e-5, 20);
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
					back->addConstrainRange("x", ECFC::EMPTYVALUE, ECFC::EMPTYVALUE, 1e6, "fitness");
					back->addConstrainRange("y", ECFC::EMPTYVALUE, ECFC::EMPTYVALUE, 1e6, "fitness");
				}
				else
					back->addConstrainRange("x", ECFC::EMPTYVALUE, ECFC::EMPTYVALUE, 1e6, "fitness");
				
				break;
			}
            case ECFC::PT_Numerical::Benmarks::CEC2013_L:
            {
                // 统计并添加变量
                int variable_number = 1;
                switch (fid)
                {
                case 1:
                    variable_number = 1;
                    back->addVariable("x", -100, 100, 1e-5, 1000);
                    break;
                case 2:
                    variable_number = 1;
                    back->addVariable("x", -5, 5, 1e-5, 1000);
                    break;
                case 3:
                    variable_number = 1;
                    back->addVariable("x", -32, 32, 1e-5, 1000);
                    break;
                case 4:
                    variable_number = 1;
                    back->addVariable("x", -100, 100, 1e-5, 1000);
                    break;
                case 5:
                    variable_number = 1;
                    back->addVariable("x", -5, 5, 1e-5, 1000);
                    break;
                case 6:
                    variable_number = 1;
                    back->addVariable("x", -32, 32, 1e-5, 1000);
                    break;
                case 7:
                case 8:
                    variable_number = 1;
                    back->addVariable("x", -100, 100, 1e-5, 1000);
                    break;
                case 9:
                    variable_number = 1;
                    back->addVariable("x", -5, 5, 1e-5, 1000);
                    break;
                case 10:
                    variable_number = 1;
                    back->addVariable("x", -32, 32, 1e-5, 1000);
                    break;
                case 11:
                case 12:
                case 13:
                case 14:
                case 15:
                    variable_number = 1;
                    back->addVariable("x", -100, 100, 1e-5, 1000);
                default:
                    break;
                }
                // 添加目标函数
                delete _evaluate;
                _evaluate = new evaluateFunc(Benmarks::CEC2013_L, _ptr, variable_number);
                back->addObjective("fitness", 1, true, "x", _evaluate);
                    
                // 添加约束
                back->addConstrainRange("x", ECFC::EMPTYVALUE, ECFC::EMPTYVALUE, 1e6, "fitness");

                break;
            }
			case ECFC::PT_Numerical::Benmarks::P_default:
			case ECFC::PT_Numerical::Benmarks::P_end:
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
			case ECFC::PT_Numerical::Benmarks::CEC2013_N:
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
            case ECFC::PT_Numerical::Benmarks::CEC2013_L:
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
			case ECFC::PT_Numerical::Benmarks::P_default:
			case ECFC::PT_Numerical::Benmarks::P_end:
			default:
				sys_logger.error("benchmark name not exist!");
				return;
			}
			setName(b_name + "(f" + std::to_string(func_id) + ")");
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
