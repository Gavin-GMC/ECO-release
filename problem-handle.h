//------------------------Description------------------------
// This file defines the handle of problem module for optimizer,
//  which provides all the functions related to problem handling, 
// including individual evaluation, constrains propogation,
// feasiable region reduction, heuristic decision, etc.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include<map>
#include"variable.h"
#include"solution.h"
#include"calculator.h"
#include"objective.h"
#include"comparer.h"
#include"feasible-region.h"
#include"constrain.h"
#include"inspiration.h"
#include"logger.h"


namespace ECFC
{
/// <summary>
/// 句柄负责约束、评估和启发式
/// 基本流程：
/// 定位变量位置
/// 将候选解分解为变量
/// 基于变量调用相关方法并返回结果
/// 
/// 可行域与约束的协同更新
/// 生效约束的控制方式：
/// 约束级别，越高的级别适用越复杂的约束（初始化更新可以最高因为在后续不参与）
/// 
/// 待添加：控制参数，计算量相关
/// </summary>

	class Problem;

	class ProblemHandle final
	{
	private:
		std::string _name;

		// 问题的值空间
		int _problem_size; // 决策量的总长度（候选解长度）
		ECElement* variables; // 包含问题的常量，计算量与决策量
		int variable_number;
		// 问题值的可行域
		FeasibleLine* feasible_regions_ini; //每个变量的初始可行域
		FeasibleLine* feasible_regions_cur; //每个变量的当前可行域
		int feasible_regions_dem_index; // 维度可行域的当前索引
        bool* no_dem_reduction_region; // 不需要维度缩减的可行域
        FeasibleLine* feasible_regions_dem; //维度的当前可行域
		FeasibleLine* feasible_regions_dem_ptr; //维度的当前可行域的指针
		// 决策量相关信息
		int decision_variable_number; // 决策量的数目
		int* decision_variable_index; // 决策量在变量中的索引
		int* decision_variable_offset; // 决策量在候选解中的偏置
		int* solution_belong_variable; // 每个决策量维度属于哪个量
		// 计算量相关信息
		// # To be added
		int calculation_variable_number; // 计算量的数目
		// int* calculation_variable_index; // 计算量在变量中的索引，按计算的优先相关顺序排列
		// int** calculation_require_variable; // 计算量计算需要的量的索引（按传入顺序排列）
		// Calculator* calculation_variable_calculator; // 问题计算量的计算容器


		// 问题目标空间
		Objective* _objective;
		int* objective_variable_number; // 计算目标需要的问题量的数目
		int** objective_variable_index; // 计算目标需要的问题量的索引
		int* objective_penalty_number; // 计算目标需要的约束惩罚的数目
		int** objective_penalty_index; // 计算目标需要的约束惩罚的索引		
		double** evaluate_buffer; // 评估目标时的缓存容器
		int _object_number;
		Comparer* _comparer;

		bool full_evaluate; // 是否评估不考虑的目标（负优先级）

		// 问题解解析器
		SolutionDecoder* _decoder;

		// 问题的约束空间
		struct Con4ElePair
		{
			std::vector<Constrain*> constrains; // 问题约束（与变量对应）
			// int variable_offset; // 变量的偏置（涉及只有变量的一部分参与约束的情况）（需要吗？）
			// int* constrain_offset; // 约束的偏置（涉及有多个变量参与同一个约束，因此需要加上偏置）
		};
		Con4ElePair* constrain_pairs; //问题变量约束对(数量是变量数)
		int constrain_number; // 约束数目
		Constrain** constrains; // 句柄中的所有约束，部分约束可能同时约束多个变量，会在拷贝和析构时产生问题，因此需要统一管理
		// int* constrain_variable_number; // 计算约束需要的问题量的数目
		// int** constrain_variable_index; // 计算约束需要的问题量的索引
		int* constrain_variable_index; // 计算约束需要的问题量的数目

		bool constraint_check; // 是否进行实时约束监测
		ConstrianLevel check_level; // 只有低于级别的约束才会进行约束传播与域缩减

		// 问题启发式
		Inspiration** inspirations; //与变量对应，常量的启发式设置为空指针
		int* inspiration_variable_number; // 计算目标需要的问题量的数目
		int** inspiration_variable_index; // 计算目标需要的问题量的索引
		double** inspirate_buffer; // 启发器的缓存容器

		// 私有，只有编译器作为友元可以访问并进行构造
		ProblemHandle()
		{
			_name = "";

			// 问题的值空间
			_problem_size = 0;
			variables = nullptr; 
			variable_number = 0;
			// 问题值的可行域
			feasible_regions_ini = nullptr;
			feasible_regions_cur = nullptr;
            feasible_regions_dem_index = EMPTYVALUE;
            no_dem_reduction_region = nullptr;
			feasible_regions_dem = nullptr;
            feasible_regions_dem_ptr = nullptr;
			// 决策量相关信息
			decision_variable_number = 0;
			decision_variable_index = nullptr;
			decision_variable_offset = nullptr;
			solution_belong_variable = nullptr;
			// 计算量相关信息
			// # To be added
			calculation_variable_number = 0;
			// calculation_variable_index; // 计算量在变量中的索引，按计算的优先相关顺序排列
			// calculation_require_variable; // 计算量计算需要的量的索引（按传入顺序排列）
			// calculation_variable_calculator; // 问题计算量的计算容器


			// 问题目标空间
			_objective = nullptr;
			objective_variable_number = nullptr;
			objective_variable_index = nullptr;
			objective_penalty_number = nullptr;
			objective_penalty_index = nullptr;
			evaluate_buffer = nullptr;
			_object_number = 0;
			_comparer = nullptr;

			full_evaluate = true; // 是否评估不考虑的目标（负优先级）

			// 问题解解析器
			_decoder = nullptr;

			// 问题的约束空间
			constrain_pairs = nullptr;
			constrain_number = 0;
			constrains = nullptr;
			// constrain_variable_number; // 计算约束需要的问题量的数目
			// constrain_variable_index; // 计算约束需要的问题量的索引
			constrain_variable_index = nullptr;

			constraint_check = true;
			check_level = constraints_customization;

			// 问题启发式
			inspirations = nullptr;
			inspiration_variable_number = nullptr;
			inspiration_variable_index = nullptr;
			inspirate_buffer = nullptr;
		}

		void regionReduction(int demensionId)
		{
			int vid = solution_belong_variable[demensionId]; // 变量id
			int did = demensionId - decision_variable_offset[vid]; // 维度在变量内的索引
            
            if (no_dem_reduction_region[vid])// 减少不必要的拷贝，提高运行效率
            {
                feasible_regions_dem_ptr = feasible_regions_cur + vid;
                return;
            }

            feasible_regions_dem->copy(feasible_regions_cur + vid);
            feasible_regions_dem_ptr = feasible_regions_dem;
			for (int i = 0; i < constrain_pairs[vid].constrains.size(); i++)
			{
				if (constrain_pairs[vid].constrains[i]->getConstrainLevel() == constraints_discrete)
				// 只需要考虑离散约束，因为1、暂时没有连续约束；2、其它约束已经体现在变量域了;3、自定义约束还没有很好的处理方式
				{
					constrain_pairs[vid].constrains[i]->regionReduction(did, feasible_regions_dem);
				}
			}

			feasible_regions_dem_index = demensionId; // 完成域缩减，更改索引
		}

		void clearResult()
		{
			for (int i = 0; i < decision_variable_number; i++)
			{
				variables[decision_variable_index[i]].variable.address = nullptr;
			}
		}

		void constructFeasibleRegion()
		{
			delete[] feasible_regions_ini;
			delete[] feasible_regions_cur;
			delete feasible_regions_dem;
            feasible_regions_dem_ptr = nullptr;

			feasible_regions_ini = new FeasibleLine[decision_variable_number];
			feasible_regions_cur = new FeasibleLine[decision_variable_number];
			feasible_regions_dem = new FeasibleLine();

			for (int i = 0; i < decision_variable_number; i++)
			{
				FeasibleLine buffer(&variables[decision_variable_index[i]]._note);
				// sys_logger.debug(variables[decision_variable_index[i]]._note._name);
				feasible_regions_ini[i].copy(&buffer);
				// sys_logger.debug(std::to_string(feasible_regions_ini[i].getLength()));
			}
            
			feasible_regions_dem_index = EMPTYVALUE;
		}

        void buildNoReductionTable()
        {
            no_dem_reduction_region = new bool[decision_variable_number];
            for (int vid = 0; vid < decision_variable_number; vid++)
            {
                no_dem_reduction_region[vid] = true;
                for (int i = 0; i < constrain_pairs[vid].constrains.size(); i++)
                {
                    if (constrain_pairs[vid].constrains[i]->getConstrainLevel() == constraints_discrete)
                        // 只需要考虑离散约束，因为1、暂时没有连续约束；2、其它约束已经体现在变量域了;3、自定义约束还没有很好的处理方式
                    {
                        no_dem_reduction_region[vid] = false;
                        break;
                    }
                }
            }
        }

		double getPenalty4Object(int oid)
		{
			double back = 0;

			int cid, vid;
			// 调用前已经链接过了候选解，无需重复链接
			for (int i = 0; i < objective_penalty_number[oid]; i++)
			{
				cid = objective_penalty_index[oid][i];
				vid = constrain_variable_index[cid];
				back += constrains[cid]->violation(variables[vid].variable.address, variables[vid].getLength()) * constrains[cid]->getWeight();
			}

			return back;
		}

		void selfBuild()
		{
			// 构建比较器
			delete _comparer;
			_comparer = new Comparer(_objective, _object_number);

			// 构建解析器
			delete _decoder;
			_decoder = new SolutionDecoder(decision_variable_number, _object_number);
			for (int i = 0; i < decision_variable_number; i++)
			{
				_decoder->notes[i] = variables[decision_variable_index[i]]._note;
				_decoder->variable_sizes[i] = variables[decision_variable_index[i]].getLength();
			}
			for (int i = 0; i < _object_number; i++)
			{
				_decoder->object_name[i] = _objective[i].getName();
			}
		}

		friend class Problem;

	public:
		ProblemHandle(ProblemHandle& source_handle)
		{
			// 依次硬拷贝
			_name = source_handle._name;

			// 变量空间初始化
			variable_number = source_handle.variable_number;
			variables = new ECElement[variable_number];

			// 复制变量
			_problem_size = source_handle._problem_size;
			decision_variable_number = source_handle.decision_variable_number;
			calculation_variable_number = source_handle.calculation_variable_number;
			for (int i = 0; i < variable_number; i++)
			{
				variables[i].copy(source_handle.variables + i);
			}

			// 复制决策变量
			decision_variable_index = new int[decision_variable_number];
			decision_variable_offset = new int[decision_variable_number];
			solution_belong_variable = new int[_problem_size];
			
			memcpy(decision_variable_index, source_handle.decision_variable_index, decision_variable_number * sizeof(int));
			memcpy(decision_variable_offset, source_handle.decision_variable_offset, decision_variable_number * sizeof(int));
			memcpy(solution_belong_variable, source_handle.solution_belong_variable, _problem_size * sizeof(int));
			
			// 初始化可行域空间
			// 这里可以考虑直接复制可行域同时跳过后续的可行域缩减
			feasible_regions_ini = nullptr;
			feasible_regions_cur = nullptr;
			feasible_regions_dem_index = EMPTYVALUE;
            no_dem_reduction_region = nullptr;
            feasible_regions_dem = nullptr;
			feasible_regions_dem_ptr = nullptr;
            constructFeasibleRegion();

			// 复制计算量
			// To be added!
			calculation_variable_number = 0;

			// 目标空间初始化
			_object_number = source_handle._object_number;
			_objective = new Objective[_object_number];
			evaluate_buffer = new double* [variable_number];
			objective_penalty_number = new int [_object_number]; // 计算目标需要的约束惩罚的数目
			objective_penalty_index = new int* [_object_number]; // 计算目标需要的约束惩罚的索引	

			full_evaluate = source_handle.full_evaluate;
			_comparer = nullptr;
			_decoder = nullptr;

			// 复制优化目标
			for (int i = 0; i < _object_number; i++)
			{
				_objective[i].copy(source_handle._objective + i);
			}

			// 复制目标函数索引表
			objective_variable_number = new int[_object_number];
			objective_variable_index = new int* [_object_number];
			memcpy(objective_variable_number, source_handle.objective_variable_number, _object_number * sizeof(int));
			for (int i = 0; i < _object_number; i++)
			{
				objective_variable_index[i] = new int[objective_variable_number[i]];
				memcpy(objective_variable_index[i], source_handle.objective_variable_index[i], objective_variable_number[i] * sizeof(int));
			}

			// 复制约束惩罚表
			memcpy(objective_penalty_number, source_handle.objective_penalty_number, _object_number * sizeof(int));
			for (int i = 0; i < _object_number; i++)
			{
				objective_penalty_index[i] = new int[objective_penalty_number[i]];
				memcpy(objective_penalty_index[i], source_handle.objective_penalty_index[i], objective_penalty_number[i] * sizeof(int));
			}

			// 约束空间初始化
			constraint_check = source_handle.constraint_check;
			check_level = source_handle.check_level;
			constrain_pairs = new ProblemHandle::Con4ElePair[variable_number];
			constrain_number = source_handle.constrain_number;
			constrains = new Constrain * [constrain_number];
			constrain_variable_index = new int[constrain_number];

			// 复制约束
			for (int cid = 0; cid < constrain_number; cid++)
			{
				constrains[cid] = source_handle.constrains[cid]->clone();

				// 关联约束
				for (int vid = 0; vid < variable_number; vid++)
				{
                    if (source_handle.constrain_pairs[vid].constrains.size() == constrain_pairs[vid].constrains.size()) // 该变量的所有约束已经拷贝完毕
                        continue;

					if (source_handle.constrain_pairs[vid].constrains[constrain_pairs[vid].constrains.size()]
						== source_handle.constrains[cid]) // 源句柄中约束集合cid与约束对vid中的下一个约束构成匹配，即应当构成链接
					{
						constrain_pairs[vid].constrains.push_back(constrains[cid]);
					}

                    // 应用域级约束
                    if (constrains[cid]->getConstrainLevel() == constraints_range)
                    {
                        constrains[cid]->regionReduction(0, feasible_regions_ini + vid);
                    }

					break;
				}
			
				constrain_variable_index[cid] = source_handle.constrain_variable_index[cid];
			}
            buildNoReductionTable();
            
			// 启发式空间初始化
			inspirations = new Inspiration * [variable_number];
			inspiration_variable_number = new int[variable_number];
			inspiration_variable_index = new int* [variable_number];
			inspirate_buffer = new double* [variable_number + 2];
			// 复制启发式
			for (int i = 0; i < variable_number; i++)
			{
				if (source_handle.inspirations[i] != nullptr)
				{
					inspirations[i] = source_handle.inspirations[i]->clone();
				}
				else
				{
					inspirations[i] = nullptr;
				}
			}

			memcpy(inspiration_variable_number, source_handle.inspiration_variable_number, sizeof(int)* variable_number);
			for (int i = 0; i < variable_number; i++)
			{
				inspiration_variable_index[i] = new int[inspiration_variable_number[i]];
				memcpy(inspiration_variable_index[i], source_handle.inspiration_variable_index[i], sizeof(int) * inspiration_variable_number[i]);
			}			

			// 句柄二级量自构建
			selfBuild();
		}

		~ProblemHandle()
		{
			clearResult();
			delete[] variables;
			// 问题值的可行域
			delete[] feasible_regions_ini;
			delete[] feasible_regions_cur;
			delete feasible_regions_dem;
            delete[] no_dem_reduction_region;
            feasible_regions_dem_ptr = nullptr;
			// 决策量相关信息
			delete[] decision_variable_index;
			delete[] decision_variable_offset;
			delete[] solution_belong_variable;
			// 计算量相关信息
			// delete[] calculation_variable_index; // 计算量在变量中的索引，按计算的优先相关顺序排列
			// for (int i = 0; i < calculation_variable_number; i++)
			//     delete[] calculation_require_variable[i];
			// delete[] calculation_require_variable;
			// delete[] calculation_variable_calculator; // 问题计算量的计算容器

			// 问题目标空间
			delete[] _objective;
			delete[] objective_variable_number;
			for (int i = 0; i < _object_number; i++)
				delete[] objective_variable_index[i];
			delete[] objective_variable_index;

			delete[] objective_penalty_number;
			for (int i = 0; i < _object_number; i++)
				delete[] objective_penalty_index[i];
			delete[] objective_penalty_index;

			delete[] evaluate_buffer;


			delete _comparer;
			delete _decoder;

			// 问题的约束空间
			delete[] constrain_pairs; //问题变量约束对(数量是变量数)
			for (int i = 0; i < constrain_number; i++)
				delete constrains[i];
			delete[] constrains;
			delete[] constrain_variable_index;

			for (int i = 0; i < variable_number; i++)
				delete inspirations[i];
			delete[] inspirations;
			delete[] inspiration_variable_number;
			for (int i = 0; i < variable_number; i++)
				delete[] inspiration_variable_index[i];
			delete[] inspiration_variable_index;
			delete[] inspirate_buffer;
		}

		std::string getName()
		{
			return _name;
		}

		int getProblemSize()
		{
			return _problem_size;
		}

		int getObjectNumber()
		{
			return _object_number;
		}

		int getVariableNumber()
		{
			return variable_number;
		}

		int getBelongVariableId(int demensionId)
		{
			return solution_belong_variable[demensionId];
		}

		int getWithinVariableId(int demensionId)
		{
			int vid = solution_belong_variable[demensionId];
			return demensionId - decision_variable_offset[vid];
		}

		int getVariableOffset(int vid)
		{
			return decision_variable_offset[vid];
		}

		int getVariableLength(int vid)
		{
			return variables[vid].getLength();
		}

		SolutionDecoder* getSolutionDecoder()
		{
			return _decoder;
		}

		Comparer* getSolutionComparer()
		{
			return _comparer;
		}

		// 将候选解中的决策量同步到问题环境之中
		void setResult(const Solution& solution)
		{
			for (int i = 0; i < decision_variable_number; i++)
			{
				variables[decision_variable_index[i]].variable.address = solution.result + decision_variable_offset[i];
			}
		}

		// 将候选解中的决策量同步到问题环境之中
		void setResult(double* solution)
		{
			for (int i = 0; i < decision_variable_number; i++)
			{
				variables[decision_variable_index[i]].variable.address = solution + decision_variable_offset[i];
			}
		}

		void constrainReset()
		{
			for (int vid = 0; vid < variable_number; vid++)
			{
				for (size_t i = 0; i < constrain_pairs[vid].constrains.size(); i++)
				{
					constrain_pairs[vid].constrains[i]->ini();
				}
			}

			for (int i = 0; i < decision_variable_number; i++)
			{
				feasible_regions_cur[i].copy(feasible_regions_ini + i);
			}

			// 约束状态重置，当前可行域已经过时，进行重置
			feasible_regions_dem_index = EMPTYVALUE;
		}

		bool constrainCheck(const int demensionId, const double value)
		{
			if (value == EMPTYVALUE) // 空量不检查
				return true;

			// 基于维度索引寻找对应的约束
			int vid = solution_belong_variable[demensionId]; // 变量id
			int did = demensionId - decision_variable_offset[vid]; // 维度在变量内的索引

			// 检查约束
			for (size_t i = 0; i < constrain_pairs[vid].constrains.size(); i++)
			{
				// 基于或逻辑，有一个不符合即检查不通过
				if (!constrain_pairs[vid].constrains[i]->meet(did, value))
				{
					return false;
				}
			}

			// 通过约束检查
			return true;
		}

		void constrainChange(const int demensionId, const double value)
		{
			// 先假设所有的约束的判定与维护都在约束对象内部完成而不涉及句柄内部量

			// 基于维度索引寻找对应的约束
			int vid = solution_belong_variable[demensionId]; // 变量id
			int did = demensionId - decision_variable_offset[vid]; // 维度在变量内的索引

			// 约束更新
			for (int i = 0; i < constrain_pairs[vid].constrains.size(); i++)
			{
				constrain_pairs[vid].constrains[i]->update(did, value);

				// 变量级别的可行域缩减
				if (constrain_pairs[vid].constrains[i]->getConstrainLevel() == constrains_variable)
				{
					constrain_pairs[vid].constrains[i]->regionReduction(did, feasible_regions_cur + vid);
				}
			}

			// 约束修改可能导致当前维度可行域过时，因此进行断联
			feasible_regions_dem_index = EMPTYVALUE;
		}

		double constraintViolation(const Solution& solution)
		{
			double back = 0;
			// 分解候选解
			setResult(solution);
			// 检查约束
			for (int vid = 0; vid < variable_number; vid++)
			{
				for (int i = 0; i < constrain_pairs[vid].constrains.size(); i++)
				{
					back += constrain_pairs[vid].constrains[i]->violation(variables[vid].variable.address, variables[vid].getLength());
				}
			}
			return back;
		}

		bool solutionLegality(const Solution& solution)
		{
			// 分解候选解
			setResult(solution);
			double violation;
			// 检查约束
			for (int vid = 0; vid < variable_number; vid++)
			{
				for (int i = 0; i < constrain_pairs[vid].constrains.size(); i++)
				{
					// 计算违反度（0表示不违反）
					violation = constrain_pairs[vid].constrains[i]->violation(variables[vid].variable.address, variables[vid].getLength());
					// 基于或逻辑，有一个不符合即检查不通过
					if (!equal(violation, 0))
						return false;
				}
			}
			// 通过合法性检查
			return true;
		}

		void solutionEvaluate(Solution& solution)
		{
			// 设置候选解
			setResult(solution);
			for (int oid = 0; oid < _object_number; oid++)
			{
				// 构建缓存
				for (int i = 0; i < objective_variable_number[oid]; i++)
				{
					evaluate_buffer[i] = variables[objective_variable_index[oid][i]].variable.address;
				}
				// 评估目标
				solution.fitness[oid] = _objective[oid].getFitness(evaluate_buffer);

				// 施加惩罚
				double penalty = getPenalty4Object(oid);
								
				if (!_objective[oid].IsMin())
					penalty *= -1;

				solution.fitness[oid] += penalty;
			}
		}

		int getChoiceNumber(const int demensionId)
		{
			if (feasible_regions_dem_index != demensionId)
				regionReduction(demensionId);
			return feasible_regions_dem_ptr->getLength();
		}

		double* getFeasibleList(const int demensionId)
		{
			int size;
			if (feasible_regions_dem_index != demensionId)
				regionReduction(demensionId);
			return feasible_regions_dem_ptr->getFeasibleList(size);
		}

		void getFeasibleList(const int demensionId, double* &list_buffer, int& size)
		{
			if (feasible_regions_dem_index != demensionId)
				regionReduction(demensionId);
			feasible_regions_dem_ptr->getFeasibleList(list_buffer, size);
		}

		double choiceDiscretized(int demension, double value)
		{
			int vid = solution_belong_variable[demension];
			double begin = variables[vid].getLowbound();
			double accuracy = variables[vid].getAccuracy();

			int length = intelliTrunc((value - begin) / accuracy);
			return begin + length * accuracy;

		}

		double getChoiceHeuristic(const int demensionId, const double choice)
		{
			// 基于维度索引寻找对应的约束
			int vid = solution_belong_variable[demensionId]; // 变量id
			int did = demensionId - decision_variable_offset[vid]; // 维度在变量内的索引

			// 更新可行域
			if (feasible_regions_dem_index != demensionId)
				regionReduction(demensionId);
			
			// 设置参数
			for (int i = 0; i < inspiration_variable_number[vid]; i++)
			{
				inspirate_buffer[i] = variables[inspiration_variable_index[vid][i]].variable.address;
			}

			// 返回启发式值
			return inspirations[vid]->getHeuristic(feasible_regions_dem_ptr, did, choice, inspirate_buffer);
		}

		double getPrioriChoice(const int demensionId)
		{
			// 基于维度索引寻找对应的约束
			int vid = solution_belong_variable[demensionId]; // 变量id
			int did = demensionId - decision_variable_offset[vid]; // 维度在变量内的索引

			// 更新可行域
			if (feasible_regions_dem_index != demensionId)
				regionReduction(demensionId);

			// 设置参数
			for (int i = 0; i < inspiration_variable_number[vid]; i++)
			{
				inspirate_buffer[i] = variables[inspiration_variable_index[vid][i]].variable.address;
			}

			// 返回优先选择
			return inspirations[vid]->getPrioriDecision(feasible_regions_dem_ptr, did, inspirate_buffer);
		}

		void getPrioriChoice(const int demensionId, int size, double* output)
		{
			// 基于维度索引寻找对应的约束
			int vid = solution_belong_variable[demensionId]; // 变量id
			int did = demensionId - decision_variable_offset[vid]; // 维度在变量内的索引

			// 更新可行域
			if (feasible_regions_dem_index != demensionId)
				regionReduction(demensionId);

			// 设置参数
			for (int i = 0; i < inspiration_variable_number[vid]; i++)
			{
				inspirate_buffer[i] = variables[inspiration_variable_index[vid][i]].variable.address;
			}

			// 获取优先选择
			int buffersize;
			double* prioribuffer = inspirations[vid]->getPrioriOrder(feasible_regions_dem_ptr, did, inspirate_buffer, buffersize);

			// 获取输出，size为最大允许输出
			if (buffersize < size)
				size = buffersize;
			memcpy(output, prioribuffer, size * sizeof(double));

			// 清除内存
			delete[] prioribuffer;
		}

		double getCloseChoice(const int demensionId, const double value)
		{
			if (feasible_regions_dem_index != demensionId)
				regionReduction(demensionId);
			return feasible_regions_dem_ptr->getClosestFeasibleDecision(value);
		}

		double getBoundaryChoice(const int demensionId, bool left = true)
		{
			if (feasible_regions_dem_index != demensionId)
				regionReduction(demensionId);
			return feasible_regions_dem_ptr->getBoundaryDecision(left);
		}

		double getVariableUpbound(const int demensionId)
		{
			int vid = solution_belong_variable[demensionId];
			return variables[vid].getUpbound();
		}

		double getVariableLowbound(const int demensionId)
		{
			int vid = solution_belong_variable[demensionId];
			return variables[vid].getLowbound();
		}

		double getRandomChoice(const int demensionId)
		{
			int vid = solution_belong_variable[demensionId];
			
			return feasible_regions_ini[vid].getRandomDecision();
		}

		double getRandomChoiceInspace(const int demensionId)
		{
			if (feasible_regions_dem_index != demensionId)
				regionReduction(demensionId);
			return feasible_regions_dem_ptr->getRandomDecision();
		}

		void getGreedyResult(Solution& solution) 
		{
			setResult(solution);
			
			constrainReset();
			for (int i = 0; i < _problem_size; i++)
			{
				solution[i] = getPrioriChoice(i);
				constrainChange(i, solution[i]);
			}
		}

		// 改变环境量
		void changeEnv(const double* values, int variableId)
		{
			memcpy(variables[variableId].variable.address, values, variables[variableId].getLength() * sizeof(double));
		}

		// 将当前问题转化为一个只优化部分决策变量的子问题
		void getSubproblemForVariable(int* remove_variableId, int variable_number)
		{
			// To be added

			// 更新决策量索引
			decision_variable_index;
			// 更新决策量偏置
			decision_variable_offset;
			// 更新维度归属
			solution_belong_variable;
			// 更新决策量的数目
			decision_variable_number;
		}

		// 将当前问题转化为一个只优化部分目标的子问题
		void getSubproblemForObject(int* remove_objectId, int object_number)
		{
			// 将不属于优化目标的优先度设为-1（不考虑）
			for (int i = 0; i < object_number; i++)
			{
				_objective[remove_objectId[i]].setPriority(-1);
			}
		}
	};
}

