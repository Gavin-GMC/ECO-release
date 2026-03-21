//------------------------Description------------------------
// This file defines the interface of problem module for user,
//  which provides all the functions related to problem definition, 
// including variable and constant addition, goal definition, 
// constraint addition, heuristic addition, etc.
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
#include"problem-handle.h"

namespace ECFC
{
	// 问题定义逻辑
	// 用户通过接口将进行问题定义，并以语句的形式存储
	// 用户可以手动调用编译方法生成问题句柄或传入优化器后由优化器进行编译
	// 编译时会先进行形式检查，通过后才会进行编译
	// 形式检查包括非法命名（关键字），重复命名，关联定义缺失等内容
	// 

	class Problem
	{
	private:
		std::string name; // 问题名称
		
		std::vector<ECElement*> elements;
		std::vector<bool> decision_variable;
		std::vector<bool> calculate_variable;

		std::vector<Objective*> objectives;
		std::vector<std::vector<std::string>> related_variables_o;

		enum class ConstrainType { user, range, compatibility, capacity, unique, distributed, mindistance };
		std::vector<ConstrainType> constrains;
		std::vector<long long int> addresses_c;
		std::vector<double*> parameters_c;
		std::vector<int> penaltys;
		std::vector<std::vector<std::string>> related_variables_c;
		std::vector<std::vector<std::string>> related_objects_c;
		// std::vector<std::string> related_variables_c;
		// std::vector<int> related_vnumber_c;
		bool not_check_constrain = false;

		// enum class HeuristicType { user_d, user_s, user_m, random, boundary };
		// std::vector<HeuristicType> inspirations;
		// std::vector<long long int> addresses_i;
		std::vector<Inspiration*> heuristics;
		std::vector<std::string> related_variable_i;
		std::vector<std::vector<std::string>> input_variables_i;
		std::vector<bool> stable_heuristic;

		bool compil_check;

		int _findVid(std::string vname)
		{
			int vid = 0;
			while (vid < elements.size())
			{
				if (vname == elements[vid]->getName())
				{
					return vid;
				}
				vid++;
			}
			return -1;
		}

		// 冲突命名检查
		bool _duplicationCheck(std::string name, std::string type)
		{
			// 变量空间检查
			for (int i = 0; i < elements.size(); i++)
			{
				if (name == elements[i]->getName())
				{
					// 系统级日志输出错误
					sys_logger.error("The type name duplicate with existing variable!");
					return false;
				}
			}
			// 目标空间检查
			for (int i = 0; i < objectives.size(); i++)
			{
				if (name == objectives[i]->getName())
				{
					// 系统级日志输出错误
					sys_logger.error("The type name duplicate with existing object!");
					return false;
				}
			}

			return true;
		}

		// 完备性检查
		bool _completenessCheck()
		{
			// 计算量关联变量声明检查
			// To be add

			// 目标函数关联变量声明检查
			int object_number = int(objectives.size());
			for (int i = 0; i < object_number; i++)
			{
				int v_number = int(related_variables_o[i].size());

				// 查找变量名，并构建索引
				int vid;
				for (int vcounter = 0; vcounter < v_number; vcounter++)
				{
					vid = 0;
					while (vid < elements.size())
					{
						if (related_variables_o[i][vcounter] == elements[vid]->getName()) // find
						{
							break;
						}
						vid++;
					}
					if (vid == elements.size()) // 失败处理
					{
						// 系统级日志输出错误
						sys_logger.error("The variable '" + related_variables_o[i][vcounter] + "' involved in objective '" + objectives[i]->getName() + "' is undefined!");
						return false;
					}
				}
			}

			// 约束关联变量和目标声明检查
			int c_number = constrains.size();
			for (int i = 0; i < c_number; i++)
			{
				int v_number = related_variables_c[i].size();

				if (v_number != 1)// 失败处理
				{
					// 系统级日志输出错误
					sys_logger.error("Found constrain related to more than one variable!");
					return false;
				}

				// 查找变量名
				int vid;
				for (int vcounter = 0; vcounter < v_number; vcounter++)
				{
					vid = 0;
					while (vid < elements.size())
					{
						if (related_variables_c[i][vcounter] == elements[vid]->getName()) // find
						{
							break;
						}
						vid++;
					}
					if (vid == elements.size()) // 失败处理
					{
						// 系统级日志输出错误
						sys_logger.error("The variable '" + related_variables_c[i][vcounter] + "' involved in constrains '" + "' is undefined!");
						return false;
					}
				}

				// 查找目标名
				int o_number = related_objects_c[i].size();
				int oid;
				for (int ocounter = 0; ocounter < o_number; ocounter++)
				{
					oid = 0;
					while (oid < objectives.size())
					{
						if (related_objects_c[i][ocounter] == objectives[oid]->getName()) // find
						{
							break;
						}
						oid++;
					}
					if (oid == objectives.size()) // 失败处理
					{
						// 系统级日志输出错误
						sys_logger.error("The objective '" + related_objects_c[i][ocounter] + "' involved in constrains '" + "' is undefined!");
						return false;
					}
				}
			}



			// 启发式关联变量声明检查
			int h_number = heuristics.size();
			for (int i = 0; i < h_number; i++)
			{
				int v_number = input_variables_i[i].size();

				// 查找变量名
				int vid;
				for (int vcounter = 0; vcounter < v_number; vcounter++)
				{
					vid = 0;
					while (vid < elements.size())
					{
						if (input_variables_i[i][vcounter] == elements[vid]->getName()) // find
						{
							break;
						}
						vid++;
					}
					if (vid == elements.size()) // 失败处理
					{
						// 系统级日志输出错误
						sys_logger.error("The variable '" + input_variables_i[i][vcounter] + "' involved in heuristic information for '" + related_variable_i[i] + "' is undefined!");
						return false;
					}
				}
			}

			// 完成检查
			return true;
		}
		
		// 计算合法性检查
		bool _computationCheck()
		{
			// 变量形状检查

			// 除零检查

			return true;
		}

		// 编译检查
		bool _compilationCheck()
		{
			if (compil_check)
			{
				if (!_completenessCheck())
				{
					sys_logger.error("The completeness check fails!");
					return false;
				}

				if (!_computationCheck())
				{
					sys_logger.error("The computation check fails!");
					return false;
				}
			}
			else
			{
				sys_logger.warning("Compilation checks are disabled!");
			}

			return true;
		}

		// 变量编译
		void _compileVariable(ProblemHandle* target)
		{
			// 变量空间初始化
			target->variable_number = elements.size();
			target->variables = new ECElement[target->variable_number];

			// 设置变量
			target->_problem_size = 0;
			target->decision_variable_number = 0;
			target->calculation_variable_number = 0;
			for (int i = 0; i < elements.size(); i++)
			{
				target->variables[i].copy(elements[i]);
				if (decision_variable[i])
				{
					target->decision_variable_number++;
					target->_problem_size += target->variables[i].getLength();
				}
				if (calculate_variable[i])
				{
					target->calculation_variable_number++;
				}
					
			}

			// 设置决策变量
			target->decision_variable_index = new int[target->decision_variable_number];
			target->decision_variable_offset = new int[target->decision_variable_number];
			target->solution_belong_variable = new int[target->_problem_size];
			int offset_ptr = 0;
			int decison_ptr = 0;
			int v_length;
			for (int i = 0; i < elements.size(); i++)
			{
				if (decision_variable[i])
				{
					target->decision_variable_index[decison_ptr] = i;
					target->decision_variable_offset[decison_ptr] = offset_ptr;
					v_length = elements[i]->getLength();
					for (int j = 0; j < v_length; j++)
					{
						target->solution_belong_variable[offset_ptr++] = i;
					}
					decison_ptr++;
				}
			}
			// 初始化可行域空间
			target->constructFeasibleRegion();

			// 设置计算量
			// To be added!
		}

		// 目标编译
		void _compileObjective(ProblemHandle* target)
		{
			// 目标空间初始化
			target->_object_number = objectives.size();
			target->_objective = new Objective[objectives.size()];
			target->evaluate_buffer = new double* [target->variable_number];

			// 设置优化目标
			for (int i = 0; i < objectives.size(); i++)
			{
				target->_objective[i].copy(objectives[i]);
			}

			// 设置目标函数索引表
			target->objective_variable_number = new int[target->_object_number];
			target->objective_variable_index = new int* [target->_object_number];
			
			for (int i = 0; i < target->_object_number; i++)
			{
				int v_number = related_variables_o[i].size();
				target->objective_variable_index[i] = new int[v_number];
				target->objective_variable_number[i] = v_number;

				// 查找变量名，并构建索引
				int vid;
				for (int vcounter = 0; vcounter < v_number; vcounter++)
				{
					vid = 0;
					while (vid < elements.size())
					{
						if (related_variables_o[i][vcounter] == elements[vid]->getName())
						{
							target->objective_variable_index[i][vcounter] = vid;
							break;
						}
						vid++;
					}
				}
			}
		}

		// 约束编译
		void _compileConstrain(ProblemHandle* target)
		{
			// 约束空间初始化
			target->constraint_check = !not_check_constrain;
			target->constrain_pairs = new ProblemHandle::Con4ElePair[elements.size()];
			target->constrain_number = constrains.size();
			target->constrains = new Constrain * [constrains.size()];
			target->constrain_variable_index = new int [constrains.size()];

			target->objective_penalty_number = new int[objectives.size()];
			target->objective_penalty_index = new int* [objectives.size()];

			// 构建约束
			int address_ptr = 0;
			int parameter_ptr = 0;
			Constrain* constrain_buffer = nullptr;
			for (int cid = 0; cid < constrains.size(); cid++)
			{
				// 构造约束
				switch (constrains[cid])
				{
				case ConstrainType::user:
					constrain_buffer = new ConstrainUserDefined(penaltys[cid], (void(*)())(addresses_c[address_ptr++]),
						(double(*)(int, double))(addresses_c[address_ptr++]),
						(void(*)(int, double))(addresses_c[address_ptr++]));
					break;
				case ConstrainType::range:
				{
					int vid = -1;
					if (parameters_c[parameter_ptr][0] == EMPTYVALUE)
					{
						vid = _findVid(related_variables_c[cid][0]);
						parameters_c[parameter_ptr][0] = elements[vid]->_note._lowbound;
					}
					if (parameters_c[parameter_ptr][1] == EMPTYVALUE)
					{
						if (vid < 0)
							vid = _findVid(related_variables_c[cid][0]);
						parameters_c[parameter_ptr][1] = elements[vid]->_note._upbound;
					}
					constrain_buffer = new ConstrainRange(penaltys[cid], parameters_c[parameter_ptr][0], parameters_c[parameter_ptr][1]);
					parameter_ptr++;
					break;
				}					
				case ConstrainType::compatibility:
					constrain_buffer = new ConstrainCompatibility(penaltys[cid], parameters_c[parameter_ptr] + 1, parameters_c[parameter_ptr][0]);
					parameter_ptr++;
					break;
				case ConstrainType::capacity:
					constrain_buffer = new ConstrainCapacity(penaltys[cid], parameters_c[parameter_ptr] + 1, parameters_c[parameter_ptr][0],
						parameters_c[parameter_ptr + 1] + 1, parameters_c[parameter_ptr + 1][0]);
					parameter_ptr += 2;
					break;
				case ConstrainType::unique:
				{
					// 判断关联量的长度
					int vid;
					int vlength = 0;
					for (int vcounter = 0; vcounter < related_variables_c[cid].size(); vcounter++)
					{
						vid = 0;
						while (vid < elements.size())
						{
							if (related_variables_c[cid][vcounter] == elements[vid]->getName())
							{
								vlength += target->feasible_regions_ini[vid].getLength();
								break;
							}
							vid++;
						}
					}
					if (vlength < 1000)
					{
						int len = 0;
						double* v_buffer = target->feasible_regions_ini[vid].getFeasibleList(len);
						constrain_buffer = new ConstrainUnique(penaltys[cid], v_buffer, target->feasible_regions_ini->getLength());
						delete[] v_buffer;
					}
					else
					{
						constrain_buffer = new ConstrainUniqueLarge(penaltys[cid]);
					}
					break;
				}
				case ConstrainType::distributed:
					constrain_buffer = new ConstrainDistributed(penaltys[cid], parameters_c[parameter_ptr] + 1, parameters_c[parameter_ptr][0],
						parameters_c[parameter_ptr + 1]);
					parameter_ptr += 2;
					break;
				case ConstrainType::mindistance:
				{
					// 查找相关量
					int vid;
					for (int vcounter = 0; vcounter < related_variables_c[cid].size(); vcounter++)
					{
						vid = 0;
						while (vid < elements.size())
						{
							if (related_variables_c[cid][vcounter] == elements[vid]->getName())
							{
								if (parameters_c[parameter_ptr][0]) // 指针定义
								{
									constrain_buffer = new ConstrainMinDistance(penaltys[cid],
										target->variables[vid].getLowbound(), target->variables[vid].getUpbound(),
										(double*)(addresses_c[address_ptr++]), target->variables[vid].getLength());
									parameter_ptr++;
								}
								else // 值定义
								{
									constrain_buffer = new ConstrainMinDistance(penaltys[cid],
										target->variables[vid].getLowbound(), target->variables[vid].getUpbound(),
										parameters_c[parameter_ptr][1], target->variables[vid].getLength());
									parameter_ptr++;
								}
								break;
							}
							vid++;
						}
					}
				}
				break;
				default:
					constrain_buffer = nullptr;
					break;
				}
				// 添加约束
				target->constrains[cid] = constrain_buffer;

				// 关联约束
				// 查找变量名，并构建索引
				int vid;
				for (int vcounter = 0; vcounter < related_variables_c[cid].size(); vcounter++)
				{
					vid = 0;
					while (vid < elements.size())
					{
						if (related_variables_c[cid][vcounter] == elements[vid]->getName())
						{
							// 添加约束
							target->constrain_pairs[vid].constrains.push_back(constrain_buffer);

							target->constrain_variable_index[cid] = vid; // 当有多个关联变量的时候就会出错！！！，后续版本需要注意

							// 应用域级约束
							if (constrain_buffer->getConstrainLevel() == constraints_range)
							{
								constrain_buffer->regionReduction(0, target->feasible_regions_ini + vid);
							}
							break;
						}
						vid++;
					}
				}
			}
            // 构建无缩减表
            target->buildNoReductionTable();

			// 查找目标名，并关联惩罚项
			std::vector<int> penalty_list;
			for (int oid = 0; oid < objectives.size(); oid++)
			{
				penalty_list.clear();

				// 查找所有关联约束
				for (int cid = 0; cid < constrains.size(); cid++)
				{
					for (std::vector<std::string>::iterator it = related_objects_c[cid].begin(); it != related_objects_c[cid].end(); it++)
					{
						if (objectives[oid]->getName() == *it)
						{
							penalty_list.push_back(cid);
						}
					}
				}

				target->objective_penalty_number[oid] = penalty_list.size();
				target->objective_penalty_index[oid] = new int[penalty_list.size()];
				for (int i = 0; i < penalty_list.size(); i++)
				{
					target->objective_penalty_index[oid][i] = penalty_list[i];
				}
			}
			
		}

		// 启发式编译
		void _compileHeuristic(ProblemHandle* target)
		{
			// 启发式空间初始化
			target->inspirations = new Inspiration * [elements.size()];
			target->inspiration_variable_number = new int[elements.size()];
			target->inspiration_variable_index = new int* [elements.size()];
			target->inspirate_buffer = new double* [elements.size() + 2]; // +2是因为启发式的默认输入有两项
			
			// 链接启发式
			for (int i = 0; i < elements.size(); i++)
			{
				// 只有决策量需要启发式
				if (!decision_variable[i])
				{
					target->inspirations[i] = nullptr;
					target->inspiration_variable_number[i] = 0;
					target->inspiration_variable_index[i] = nullptr;
					continue;
				}

				// 查找变量
				bool connected = false;
				for (int j = 0; j < heuristics.size(); j++)
				{
					if (elements[i]->getName() == related_variable_i[j]) // 找到对应启发式
					{
						// 构建启发式参数空间
						int v_number = input_variables_i[j].size();
						target->inspiration_variable_number[i] = v_number;
						target->inspiration_variable_index[i] = new int[v_number];
						// 查找变量名，并构建索引
						int vid;
						for (int vcounter = 0; vcounter < v_number; vcounter++)
						{
							vid = 0;
							while (vid < elements.size())
							{
								if (input_variables_i[j][vcounter] == elements[vid]->getName())
								{
									target->inspiration_variable_index[i][vcounter] = vid;
									break;
								}
								vid++;
							}
						}

						// 拷贝启发式
						if (stable_heuristic[j])
						{
							// 设置参数
							for (int pid = 0; pid < target->inspiration_variable_number[i]; pid++)
							{
								target->inspirate_buffer[pid] = target->variables[target->inspiration_variable_index[i][pid]].variable.address;
							}

							target->inspirations[i] = static_cast<NormalInspiration*>(heuristics[j])->toStable(
								target->feasible_regions_ini + i, target->variables[i].getLength(), target->inspirate_buffer);
						}
						else
						{
							target->inspirations[i] = heuristics[j]->clone();
						}
						connected = true;

						break; //设置完成
					}
				}
				if (!connected) // 缺省设置为随机启发式
				{
					target->inspirations[i] = new RandomInspiration();
					target->inspiration_variable_number[i] = 0;
					target->inspiration_variable_index[i] = nullptr;
				}
			}
		}

	public:
		Problem(std::string problem_name)
		{
			name = problem_name;
			compil_check = true;
		}

		~Problem()
		{
			clear();
		}

		void clear()
		{
			for (int i = 0; i < elements.size(); i++)
				delete elements[i];
			elements.clear();
			decision_variable.clear();
			calculate_variable.clear();

			for (int i = 0; i < objectives.size(); i++)
				delete objectives[i];
			objectives.clear();
			for (int i = 0; i < related_variables_o.size(); i++)
				related_variables_o[i].clear();
			related_variables_o.clear();

			constrains.clear();
			addresses_c.clear();
			for (int i = 0; i < parameters_c.size(); i++)
				delete parameters_c[i];
			parameters_c.clear();
			penaltys.clear();
			for (int i = 0; i < related_variables_c.size(); i++)
				related_variables_c[i].clear();
			related_variables_c.clear();
			for (int i = 0; i < related_objects_c.size(); i++)
				related_objects_c[i].clear();
			related_objects_c.clear();
			not_check_constrain = false;

			for (int i = 0; i < heuristics.size(); i++)
				delete heuristics[i];
			heuristics.clear();
			related_variable_i.clear();
			for (int i = 0; i < input_variables_i.size(); i++)
				input_variables_i[i].clear();
			input_variables_i.clear();
			stable_heuristic.clear();
		}

		void addConstant(std::string name, double* values, int length, int height = 1)
		{
			if (!_duplicationCheck(name, "constant"))
			{
				return;
			}

			ECElement* buffer = new ECElement(name, length, height, 0, 0, 0);
			int size = length * height;
			double* v_buffer = new double[size];
			memcpy(v_buffer, values, sizeof(double) * size);
			buffer->variable.setAddress(v_buffer);
			elements.push_back(buffer);
			decision_variable.push_back(false);
			calculate_variable.push_back(false);
		}

		void addVariable(std::string name, double low_bound, double up_bound, double accuracy, int length, int height = 1, VariableType type = VariableType::normal)
		{
			if (!_duplicationCheck(name, "variable"))
			{
				return;
			}

			ECElement* buffer = new ECElement(name, length, height, up_bound, low_bound, accuracy, type);
			elements.push_back(buffer);
			decision_variable.push_back(true);
			calculate_variable.push_back(false);
		}

		void addObjective(std::string name, int priority, bool min_is_better, std::string input_elements, double (*evaluate_func)(double** inputs))
		{
			if (!_duplicationCheck(name, "objective"))
			{
				return;
			}

			Objective* buffer = new Objective(name, priority, min_is_better);

			// 分离变量名
			std::vector<std::string> vname;
			if (input_elements != "")
				stringSplit(input_elements, ',', vname);
			related_variables_o.push_back(vname);//深拷贝，不用担心被析构
			
			// 构建对象
			buffer->setCalculator(new FuncCalculator(evaluate_func, related_variables_o.size()));
			objectives.push_back(buffer);
		}

		void addObjective(std::string name, int priority, bool min_is_better, std::string input_elements, eccalcul_functor* evaluate_func)
		{
			if (!_duplicationCheck(name, "objective"))
			{
				return;
			}

			Objective* buffer = new Objective(name, priority, min_is_better);

			// 分离变量名
			std::vector<std::string> vname;
			if (input_elements != "")
				stringSplit(input_elements, ',', vname);
			related_variables_o.push_back(vname);//深拷贝，不用担心被析构

			// 构建对象
			buffer->setCalculator(new FunctorCalculator(evaluate_func, related_variables_o.size()));
			objectives.push_back(buffer);
		}

		void changeConstrainDeal(bool no_check)
		{
			not_check_constrain = no_check;
		}

		// 现版本约束都限制单变量
		// 不在添加函数中构造对象是因为不确定关联的变量是否完成了声明
		
		// 添加用户定义约束
		void addConstrain(std::string input_elements, void (*model_ini)(void), double (*check_func)(int, double), void (*model_change)(int, double), double penalty_w = 1, std::string related_objects = "")
		{
			penaltys.push_back(penalty_w);
			constrains.push_back(ConstrainType::user);
			addresses_c.push_back(intptr_t(model_ini));
			addresses_c.push_back(intptr_t(check_func));
			addresses_c.push_back(intptr_t(model_change));

			// 分离变量名
			std::vector<std::string> vname;
			if (input_elements != "")
				stringSplit(input_elements, ',', vname);
			related_variables_c.push_back(vname);

			// 分离目标名
			std::vector<std::string> oname;
			if (related_objects != "")
				stringSplit(related_objects, ',', oname);
			related_objects_c.push_back(oname);
		}

		//void addConstrain(std::string input_elements, std::string formula);

		// 添加范围约束
		void addConstrainRange(std::string input_elements, const double _left = EMPTYVALUE, const int _right = EMPTYVALUE, double penalty_w = 1, std::string related_objects = "")
		{
			penaltys.push_back(penalty_w);
			constrains.push_back(ConstrainType::range);
			double* buffer;
			buffer = new double[2];
			buffer[0] = _left;
			buffer[1] = _right;
			parameters_c.push_back(buffer);

			// 分离变量名
			std::vector<std::string> vname;
			if (input_elements != "")
				stringSplit(input_elements, ',', vname);
			related_variables_c.push_back(vname);

			// 分离目标名
			std::vector<std::string> oname;
			if (related_objects != "")
				stringSplit(related_objects, ',', oname);
			related_objects_c.push_back(oname);
		}

		// 添加相容约束
		void addConstrainCompatibility(std::string input_elements, const double* value, const int length, double penalty_w = 1, std::string related_objects = "")
		{
			penaltys.push_back(penalty_w);
			constrains.push_back(ConstrainType::compatibility);
			double* buffer;
			buffer  = new double[length + 1];
			buffer[0] = length;
			memcpy(buffer + 1, value, length * sizeof(double));
			parameters_c.push_back(buffer);

			// 分离变量名
			std::vector<std::string> vname;
			if (input_elements != "")
				stringSplit(input_elements, ',', vname);
			related_variables_c.push_back(vname);

			// 分离目标名
			std::vector<std::string> oname;
			if (related_objects != "")
				stringSplit(related_objects, ',', oname);
			related_objects_c.push_back(oname);
		}

		// 添加独一约束
		void addConstrainUnique(std::string input_elements, double penalty_w = 1, std::string related_objects = "")
		{
			penaltys.push_back(penalty_w);
			constrains.push_back(ConstrainType::unique);

			// 分离变量名
			std::vector<std::string> vname;
			if (input_elements != "")
				stringSplit(input_elements, ',', vname);
			related_variables_c.push_back(vname);

			// 分离目标名
			std::vector<std::string> oname;
			if (related_objects != "")
				stringSplit(related_objects, ',', oname);
			related_objects_c.push_back(oname);
		}

		// 添加最小距约束
		void addConstrainMinDistance(std::string input_elements, double gap_width, double penalty_w = 1, std::string related_objects = "")
		{
			penaltys.push_back(penalty_w);
			constrains.push_back(ConstrainType::mindistance);
			double* buffer;
			buffer = new double[2];
			buffer[0] = 0;
			buffer[1] = gap_width;
			parameters_c.push_back(buffer);

			// 分离变量名
			std::vector<std::string> vname;
			if (input_elements != "")
				stringSplit(input_elements, ',', vname);
			related_variables_c.push_back(vname);

			// 分离目标名
			std::vector<std::string> oname;
			if (related_objects != "")
				stringSplit(related_objects, ',', oname);
			related_objects_c.push_back(oname);
		}

		// 添加最小距约束
		void addConstrainMinDistance(std::string input_elements, double* gap_width, double penalty_w = 1, std::string related_objects = "")
		{
			penaltys.push_back(penalty_w);
			constrains.push_back(ConstrainType::compatibility);
			double* buffer;
			buffer = new double(1);
			parameters_c.push_back(buffer);
			addresses_c.push_back(intptr_t(gap_width)); // gap_width 未拷贝，存在风险

			// 分离变量名
			std::vector<std::string> vname;
			if (input_elements != "")
				stringSplit(input_elements, ',', vname);
			related_variables_c.push_back(vname);

			// 分离目标名
			std::vector<std::string> oname;
			if (related_objects != "")
				stringSplit(related_objects, ',', oname);
			related_objects_c.push_back(oname);
		}

		// 添加容量约束
		void addConstrainCapacity(std::string input_elements, double* capacitys, int container_number, double* volumes, int item_number, double penalty_w = 1, std::string related_objects = "")
		{
			penaltys.push_back(penalty_w);
			constrains.push_back(ConstrainType::capacity);
			double* buffer;
			buffer = new double[container_number + 1];
			buffer[0] = container_number;
			memcpy(buffer + 1, capacitys, container_number * sizeof(double));
			parameters_c.push_back(buffer);
			buffer = new double[item_number + 1];
			buffer[0] = item_number;
			memcpy(buffer + 1, volumes, item_number * sizeof(double));
			parameters_c.push_back(buffer);

			// 分离变量名
			std::vector<std::string> vname;
			if (input_elements != "")
				stringSplit(input_elements, ',', vname);
			related_variables_c.push_back(vname);

			// 分离目标名
			std::vector<std::string> oname;
			if (related_objects != "")
				stringSplit(related_objects, ',', oname);
			related_objects_c.push_back(oname);
		}

		// 添加分布约束
		void addConstrainDistributed(std::string input_elements, double* feasible_values, int size, int* appear_numbers, double penalty_w = 1, std::string related_objects = "")
		{
			penaltys.push_back(penalty_w);
			constrains.push_back(ConstrainType::distributed);
			double* buffer;
			buffer = new double[size + 1];
			buffer[0] = size;
			memcpy(buffer + 1, feasible_values, size * sizeof(double));
			parameters_c.push_back(buffer);
			buffer = new double[size];
			for (int i = 0; i < size; i++)
			{
				buffer[i] = appear_numbers[i];
			}
			parameters_c.push_back(buffer);

			// 分离变量名
			std::vector<std::string> vname;
			if (input_elements != "")
				stringSplit(input_elements, ',', vname);
			related_variables_c.push_back(vname);

			// 分离目标名
			std::vector<std::string> oname;
			if (related_objects != "")
				stringSplit(related_objects, ',', oname);
			related_objects_c.push_back(oname);
		}

		void addInspirationRandom(std::string variable_name)
		{
			input_variables_i.push_back(std::vector<std::string> ());
			related_variable_i.push_back(variable_name);
			heuristics.push_back(new RandomInspiration());
			stable_heuristic.push_back(false);			
		}

		void addInspirationBoundary(std::string variable_name)
		{
			input_variables_i.push_back(std::vector<std::string>());
			related_variable_i.push_back(variable_name);
			heuristics.push_back(new BoundaryInspiration());
			stable_heuristic.push_back(false);
		}

		void addInspirationFunc(std::string variable_name, std::string input_elements, double (*inspiration_func)(double** inputs), bool stable = false)
		{
			// 分离变量名
			std::vector<std::string> vname;
			if (variable_name != "") // 输入变量不为空
			{
				stringSplit(input_elements, ',', vname);
			}
			input_variables_i.push_back(vname);
			related_variable_i.push_back(variable_name);
			heuristics.push_back(new NormalInspiration(new FuncCalculator(inspiration_func, int(vname.size()) + 2))); // +2是启发式自带的两个输入 

			if (stable)
			{
				stable_heuristic.push_back(true);
			}
			else {
				stable_heuristic.push_back(false);
			}

		}

		void addInspirationFunc(std::string variable_name, std::string input_elements, eccalcul_functor* inspiration_func, bool stable = false)
		{
			// 分离变量名
			std::vector<std::string> vname;
			if (variable_name != "") // 输入变量不为空
			{
				stringSplit(input_elements, ',', vname);
			}
			input_variables_i.push_back(vname);
			related_variable_i.push_back(variable_name);

			FunctorCalculator* functor = new FunctorCalculator(inspiration_func, int(vname.size()) + 2); // +2是启发式自带的两个输入 
			heuristics.push_back(new NormalInspiration(functor));
			delete functor;

			if (stable)
			{
				stable_heuristic.push_back(true);
			}
			else {
				stable_heuristic.push_back(false);
			}

		}

		// void addInspirationFunc(std::string variable_name, std::string input_elements, std::string formula, bool stable = false);

		void addInspirationMatrix(std::string variable_name, double* matrix, int decision_number, int demenion_number)
		{
			input_variables_i.push_back(std::vector<std::string>());
			related_variable_i.push_back(variable_name);
			heuristics.push_back(new StableInspiration(matrix, decision_number, demenion_number));
			stable_heuristic.push_back(false);
		}	

		ProblemHandle* compile()
		{
			if (!_compilationCheck()) // 编译检查
				return nullptr;

			ProblemHandle* back = new ProblemHandle();

			back->_name = name;

			// 逐模块编译
			_compileVariable(back);
			_compileObjective(back);
			_compileConstrain(back);
			_compileHeuristic(back);

			// 句柄二级量自构建
			back->selfBuild();

			return back;
		}
	};
}
