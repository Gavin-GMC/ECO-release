//------------------------Description------------------------
// This file defines the learning strategy of aco, including ant system
// and ant colony system. It is based on pheromone matrix to realize 
// the indirect communication between individuals and the accumulation 
// of population knowledge.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include"ecfc-constant.h"
#include"solution.h"
#include"learning-strategy.h"
#include"sort-helper.h"

namespace ECFC
{
	struct Pheromone
	{
	private:
		double _low_bound;
		double _accuracy;
		double _threshold;

	public:
		std::string name;
		int demension;
		int choice;
		double* matrix;
		bool pre_base;
		bool bidiagraph;

		Pheromone()
		{
			name = "";
			demension = 0;
			choice = 0;
			matrix = nullptr;
		}

		~Pheromone()
		{
			delete[] matrix;
		}

		void setVariable(ElementNote& note, int variable_size)
		{
			switch (note._type)
			{
			case VariableType::normal:
			case VariableType::allocation:
			{
				name = note._name;
				pre_base = false;
				demension = variable_size;
				choice = int((note._upbound - note._lowbound) / note._accuracy) + 1;
				
				int size = demension * choice;
				delete[] matrix;
				matrix = new double[size];
				_low_bound = note._lowbound;
				_accuracy = note._accuracy;
				break;
			}
			case VariableType::sequence_direction:
			case VariableType::sub_module:
			{
				name = note._name;
				pre_base = true;
				bidiagraph = false;
				demension = int((note._upbound - note._lowbound) / note._accuracy) + 1;
				choice = int((note._upbound - note._lowbound) / note._accuracy) + 1;
				int size = demension * choice;
				delete[] matrix;
				matrix = new double[size];
				_low_bound = note._lowbound;
				_accuracy = note._accuracy;
				break;
			}
			case VariableType::sequence_bidiagraph:
			{
				name = note._name;
				pre_base = true;
				bidiagraph = true;
				demension = int((note._upbound - note._lowbound) / note._accuracy) + 1;
				choice = int((note._upbound - note._lowbound) / note._accuracy) + 1;
				int size = demension * choice;
				delete[] matrix;
				matrix = new double[size];
				_low_bound = note._lowbound;
				_accuracy = note._accuracy;
				break;
			}
			default:
				break;
			}
		}

		void setini(double value)
		{
			int count = 0;
			for (int i = 0; i < demension; i++)
			{
				for (int j = 0; j < choice; j++)
				{
					matrix[count++] = value;
				}
			}
			_threshold = value * 1e-100;
		}

		// 将选择id转化为实际值
		double getChoice(int choiceid)
		{
			return _low_bound + choiceid * _accuracy;
		}

		// 将实际选择转化为id
		int getCid(double choice)
		{
			return (choice - _low_bound) / _accuracy;
		}

		void evaporation(int did, int cid, double rate)
		{
			if (did == EMPTYVALUE || cid == EMPTYVALUE)
				return;

			matrix[did * choice + cid] *= rate;
		}

		void evaporation(double rate)
		{
			int size = demension * choice;
			for (int i = 0; i < size; i++)
				matrix[i] *= rate;
		}

		void release(int did, int cid, double value)
		{
			matrix[did * choice + cid] += value;

			if (bidiagraph)
			{
				matrix[cid * choice + did] += value;
			}
		}

		double& getValue(int did, int cid)
		{
			return matrix[did * choice + cid];
		}
	};

	// 对于变量的离散化
	// 其具有三个层级
	// 连续变量根据精确度转化为离散值
	// 离散值根据顺序转化为索引值
	//

	// 蚂蚁系统算法
	class ASStrategy : public LearningStrategy
	{
	private:
		int variable_number;
		Pheromone** _pheromones;
		double _tao_ini;
		bool setted_tao;
		double _alpha;
		double _belta;
		double _rho;
		double* _h_list;
		int* variable_length;
		double* feasible_list;
		int _flist_size;

		double priority(int dimension, double choice, int vid, int decision_base, int choiceid, ProblemHandle* handle)
		{
			double h = handle->getChoiceHeuristic(dimension, choice);
			return pow(_pheromones[vid]->getValue(decision_base, choiceid), _alpha) * pow(h, _belta);
		}

		void _deletePheromones()
		{
			for (int i = 0; i < variable_number; i++)
				delete _pheromones[i];

			delete[] _pheromones;
		}

	public:
		ASStrategy(double alpha = 2, double belta = 2, double rho = 0.5, double tao_ini = EMPTYVALUE)
			:LearningStrategy()
		{
			variable_number = 0;
			_pheromones = nullptr;
			
			if (tao_ini == EMPTYVALUE)
				setted_tao = false;
			else
				setted_tao = true;
			_tao_ini = tao_ini;

			_alpha = alpha;
			_belta = belta;
			_rho = rho;

			_h_list = nullptr;
			variable_length = nullptr;
			feasible_list = nullptr;
			_flist_size = 0;
		}

		~ASStrategy()
		{
			_deletePheromones();
			delete[] _h_list;
			delete[] variable_length;
			delete[] feasible_list;
		}

		static void preAssert(AssertList& list, double* paras)
		{
			list.add(new Assert(ModuleType::T_learntopology, "objects", 0, MatchType::notLessButNotice)); // 不需要学习目标
		}

		static void postAssert(AssertList& list, double* paras)
		{
			// none
		}

		void ini(ProblemHandle* problem_handle)
		{
			for (int i = 0; i < variable_number; i++)
			{
				_pheromones[i]->setini(_tao_ini);
			}
		}

		void setProblem(ProblemHandle* problem_handle)
		{
			// 这里可以考虑改进一下，把仍存在的变量的信息进行保留
			SolutionDecoder* decoder = problem_handle->getSolutionDecoder();
			if (!setted_tao)
			{
				Solution solution;
				solution.setSize(problem_handle->getProblemSize(), problem_handle->getObjectNumber());
				problem_handle->getGreedyResult(solution);
				problem_handle->solutionEvaluate(solution);
				_tao_ini = 1 / (solution.fitness[0]);
			}
			
			if (variable_number != decoder->getVariableNumber())
			{
				_deletePheromones();
				variable_number = decoder->getVariableNumber();
				_pheromones = new Pheromone*[variable_number];
				for (int i = 0; i < variable_number; i++)
					_pheromones[i] = new Pheromone();

				delete[] variable_length;
				variable_length = new int[variable_number];
			}

			for (int i = 0; i < variable_number; i++)
			{
				_pheromones[i]->setVariable(decoder->notes[i], decoder->variable_sizes[i]);
				variable_length[i] = decoder->variable_sizes[i];
			}

			int max_size = -1;
			for (int i = 0; i < variable_number; i++)
			{
				if (_pheromones[i]->choice > max_size)
				{
					max_size = _pheromones[i]->choice;
				}
			}
			_h_list = new double[max_size + 1];
			_flist_size = max_size + 1;
			feasible_list = new double[_flist_size];
		}

		double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual* child)
		{
			double total = 0;

			int feasible_number = _flist_size;
			problem_handle->getFeasibleList(decision_d, feasible_list, feasible_number);

			if (feasible_number == 0)
				return EMPTYVALUE;

			// 轮盘赌
			int vid = problem_handle->getBelongVariableId(decision_d);
			int cid;

			// 计算启发式依据
			int decision_base;
			if (_pheromones[vid]->pre_base)
			{
				int pre_decision;
				int id_in_variable = problem_handle->getWithinVariableId(decision_d);
				if (id_in_variable == 0)
					pre_decision = problem_handle->getRandomChoiceInspace(decision_d);
				else
					pre_decision = child->solution.result[decision_d - 1];

				decision_base = _pheromones[vid]->getCid(pre_decision);
			}
			else
			{
				decision_base = decision_d;
			}
			// 选择概率计算
			for (int j = 0; j < feasible_number; j++)
			{
				cid = _pheromones[vid]->getCid(feasible_list[j]);
				_h_list[j] = priority(decision_d, feasible_list[j], vid, decision_base, cid, problem_handle);
				total += _h_list[j];
			}
			if (total == 0)
			{
				int backid = rand() % feasible_number;
				double back = feasible_list[backid];
				// delete[] feasible_list;
				return back;
			}
			else
			{
				for (int j = 0; j < feasible_number; j++)
					_h_list[j] /= total;
				total = rand01();
				// 依照概率选择
				for (int j = 0; j < feasible_number; j++)
				{
					total -= _h_list[j];
					if (notlarge(total, 0))
					{
						double back = feasible_list[j];
						// delete[] feasible_list;
						return back;
					}
				}
			}

			return EMPTYVALUE;
		}

		void update_s(Population& population, Population& offspring)
		{
			int counter = 0;
			int cid;
			int did;
			double added;
			Solution* solution;

			// 信息素蒸发
			for (int vid = 0; vid < variable_number; vid++)
			{
				_pheromones[vid]->evaporation(1 - _rho);
			}

			// 信息素释放
			int p_size = population.getSize();
			for (int i = 0; i < p_size; i++)
			{
				solution = &population[i].solution;
				counter = 0; // 指针重置
				// 计算释放值
				added = 1 / solution->fitness[0];

				for (int vid = 0; vid < variable_number; vid++)
				{
					for (int d = 0; d < variable_length[vid]; d++)
					{
						// 计算决策基础
						if (_pheromones[vid]->pre_base)
						{
							if (d == 0)
							{
								did = _pheromones[vid]->getCid((*solution)[counter + variable_length[vid] - 1]);
							}
							else
							{
								did = _pheromones[vid]->getCid((*solution)[counter - 1]);
							}
						}
						else
						{
							did = d;
						}

						cid = _pheromones[vid]->getCid((*solution)[counter]);

						_pheromones[vid]->release(did, cid, added);
						counter++;
					}	
				}
			}
		}
	};

	// 蚁群系统算法
	class ACSStrategy : public LearningStrategy
	{
	private:
		int variable_number;
		Pheromone** _pheromones;
		double _tao_ini;
		bool setted_tao;
		int* variable_length;
		double _alpha;
		double _belta;
		double _rho_g;
		double _rho_l;
		double _q0;
		double* _h_list;

		Individual gbest;

		double priority(int dimension, double choice, int vid, int decision_base, int choiceid, ProblemHandle* handle)
		{
			double h = handle->getChoiceHeuristic(dimension, choice);
			return pow(_pheromones[vid]->getValue(decision_base, choiceid), _alpha) * pow(h, _belta);
		}

		void _deletePheromones()
		{
			for (int i = 0; i < variable_number; i++)
				delete _pheromones[i];

			delete[] _pheromones;
		}

	public:
		ACSStrategy(double alpha = 1, double belta = 2, double rho_g = 0.1, double rho_l = 0.1, double q0 = 0.9, double tao_ini = EMPTYVALUE)
			:LearningStrategy()
		{
			variable_number = 0;
			_pheromones = nullptr;
			variable_length = nullptr;
			
			if (equal(tao_ini, EMPTYVALUE))
				setted_tao = false;
			else
				setted_tao = true;
			_tao_ini = tao_ini;

			_alpha = alpha;
			_belta = belta;
			_rho_g = rho_g;
			_rho_l = rho_l;
			_q0 = q0;

			_h_list = nullptr;
		}

		~ACSStrategy()
		{
			_deletePheromones();
			delete[] _h_list;
			delete[] variable_length;
		}

		static void preAssert(AssertList& list, double* paras)
		{
			list.add(new Assert(ModuleType::T_learntopology, "objects", 0, MatchType::notLessButNotice)); // 不需要学习目标
		}

		static void postAssert(AssertList& list, double* paras)
		{
			// none
		}

		void ini(ProblemHandle* problem_handle)
		{
			for (int i = 0; i < variable_number; i++)
			{
				_pheromones[i]->setini(_tao_ini);
			}

			problem_handle->getGreedyResult(gbest.solution);
			problem_handle->solutionEvaluate(gbest.solution);
		}

		void setProblem(ProblemHandle* problem_handle)
		{
			// 这里可以考虑改进一下，把仍存在的变量的信息进行保留
			SolutionDecoder* decoder = problem_handle->getSolutionDecoder();
			if (!setted_tao)
			{
				Solution solution;
				solution.setSize(problem_handle->getProblemSize(), problem_handle->getObjectNumber());
				problem_handle->getGreedyResult(solution);
				problem_handle->solutionEvaluate(solution);
				_tao_ini = 1.0 / solution.fitness[0] / solution.getSolutionSize();
			}

			if (variable_number != decoder->getVariableNumber())
			{
				_deletePheromones();
				variable_number = decoder->getVariableNumber();
				_pheromones = new Pheromone * [variable_number];
				for (int i = 0; i < variable_number; i++)
					_pheromones[i] = new Pheromone();

				delete[] variable_length;
				variable_length = new int[variable_number];
			}

			for (int i = 0; i < variable_number; i++)
			{
				_pheromones[i]->setVariable(decoder->notes[i], decoder->variable_sizes[i]);
				variable_length[i] = decoder->variable_sizes[i];
			}

			int max_size = -1;
			for (int i = 0; i < variable_number; i++)
			{
				if (_pheromones[i]->choice > max_size)
				{
					max_size = _pheromones[i]->choice;
				}
			}
			delete[] _h_list;
			_h_list = new double[max_size + 1];

			gbest.setProblem(problem_handle);
		}

		double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual* child)
		{
			int feasible_number = problem_handle->getChoiceNumber(decision_d);
			double* feasible_list = problem_handle->getFeasibleList(decision_d);

			if (feasible_number == 0)
				return EMPTYVALUE;

			int vid = problem_handle->getBelongVariableId(decision_d);

			// 计算启发式依据
			int decision_base;
			if (_pheromones[vid]->pre_base)
			{
				int pre_decision;
				int id_in_variable = problem_handle->getWithinVariableId(decision_d);
				if (id_in_variable == 0)
					pre_decision = problem_handle->getRandomChoiceInspace(decision_d);
				else
					pre_decision = child->solution.result[decision_d - 1];

				decision_base = _pheromones[vid]->getCid(pre_decision);
			}
			else
			{
				decision_base = decision_d;
			}

			int cid;
			for (int j = 0; j < feasible_number; j++)
			{
				cid = _pheromones[vid]->getCid(feasible_list[j]);
				_h_list[j] = priority(decision_d, feasible_list[j], vid, decision_base, cid, problem_handle);
			}

			if (rand01() < _q0) // 选择最优
			{
				int backid = 0;
				double max_p = _h_list[0];

				for (int j = 1; j < feasible_number; j++)
				{
					if (_h_list[j] > max_p)
					{
						max_p = _h_list[j];
						backid = j;
					}
				}

				double back = feasible_list[backid];
				delete[] feasible_list;

				return back;
			}
			else // 轮盘赌
			{
				int next = 0;
				double total = 0;

				for (int j = 0; j < feasible_number; j++)
				{
					total += _h_list[j];
				}

				if (total == 0)
				{
					int backid = rand() % feasible_number;
					double back = feasible_list[backid];
					delete[] feasible_list;
					return back;
				}
				else
				{
					for (int j = 0; j < feasible_number; j++)
						_h_list[j] /= total;
					total = rand01();
					// 依照概率选择
					for (int j = 0; j < feasible_number; j++)
					{
						total -= _h_list[j];
						if (notlarge(total, 0))
						{
							double back = feasible_list[j];
							delete[] feasible_list;
							return back;
						}
					}
				}
			}

			return EMPTYVALUE;
		}

		void update_d(Individual* child, const int decision_d)
		{
			// 局部更新

			// 计算变量id
			int vid = 0;
			int offset = variable_length[0];
			while (offset < decision_d)
			{
				vid++;
				offset += variable_length[vid];
			}

			// 计算变量内维度
			offset -= variable_length[vid];
			int did = decision_d - offset;

			if (_pheromones[vid]->pre_base)
			{
				// 首维无前决策
				if (did == 0)
					return;

				int preid = _pheromones[vid]->getCid(child->solution[decision_d - 1]);
				int cid = _pheromones[vid]->getCid(child->solution[decision_d]);

				_pheromones[vid]->evaporation(preid, cid, 1 - _rho_l);
				_pheromones[vid]->release(preid, cid, _rho_l * _tao_ini);
			}
			else
			{
				int cid = _pheromones[vid]->getCid(child->solution[decision_d]);

				_pheromones[vid]->evaporation(did, cid, 1 - _rho_l);
				_pheromones[vid]->release(did, cid, _rho_l * _tao_ini);
			}
		}

		void update_s(Population& population, Population& offspring)
		{
			// 全局更新

			// 寻找最优个体
			int bestid = 0;
			int p_size = population.getSize();
			for (int i = 1; i < p_size; i++)
			{
				if (population[i] < population[bestid])
				{
					bestid = i;
				}
			}

			if (population[bestid] < gbest)
				gbest.copy(population[bestid]);

			// 信息素更新(种群最优)
			int cid;
			int did;
			Solution* solution = &population[bestid].solution;
			// 计算释放值
			double added = _rho_g / solution->fitness[0];
			// 释放
			int counter = 0;
			for (int vid = 0; vid < variable_number; vid++)
			{
				for (int d = 0; d < variable_length[vid]; d++)
				{
					// 计算决策基础
					if (_pheromones[vid]->pre_base)
					{
						if (d == 0)
						{
							did = _pheromones[vid]->getCid((*solution)[counter + variable_length[vid] - 1]);
						}
						else
						{
							did = _pheromones[vid]->getCid((*solution)[counter - 1]);
						}
					}
					else
					{
						did = d;
					}

					cid = _pheromones[vid]->getCid((*solution)[counter]);

					_pheromones[vid]->evaporation(did, cid, 1 - _rho_g);
					_pheromones[vid]->release(did, cid, added);
					counter++;
				}
			}

			// 信息素更新(历史最优)
			solution = &gbest.solution;
			// 计算释放值
			added = _rho_g / solution->fitness[0];
			// 释放
			counter = 0;
			for (int vid = 0; vid < variable_number; vid++)
			{
				for (int d = 0; d < variable_length[vid]; d++)
				{
					// 计算决策基础
					if (_pheromones[vid]->pre_base)
					{
						if (d == 0)
						{
							did = _pheromones[vid]->getCid((*solution)[counter + variable_length[vid] - 1]);
						}
						else
						{
							did = _pheromones[vid]->getCid((*solution)[counter - 1]);
						}
					}
					else
					{
						did = d;
					}

					cid = _pheromones[vid]->getCid((*solution)[counter]);

					_pheromones[vid]->evaporation(did, cid, 1 - _rho_g);
					_pheromones[vid]->release(did, cid, added);
					counter++;
				}
			}
			/**/
		}
	};
}