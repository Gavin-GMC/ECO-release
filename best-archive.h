//------------------------Description------------------------
// This file defines the best archive of optimizer and swarms,
// which records all optimal or acceptable solutions.
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
#include"comparer.h"
#include"problem-handle.h"

namespace ECFC
{
	class BestArchive
	{
	protected:
		Solution* _best_solution;
		Comparer* _comparer_pointer;
		int _best_size;

	public:
		BestArchive()
		{
			_best_solution = nullptr;
			_comparer_pointer = nullptr;
			_best_size = 0;
		};

		virtual ~BestArchive() {
			
		};

		virtual Solution* getBest()
		{
			return _best_solution;
		}

		virtual Solution* getElite() = 0;

		virtual bool updateBest(const Solution& new_best) = 0;

		virtual int getBestSize()
		{
			return _best_size;
		}

		virtual void clear()
		{
			_best_size = 0;
		}

		virtual void setProblem(ProblemHandle* handle) = 0;
	};

	// 基础最优解管理器
	class BasicArchive final :public BestArchive
	{
	public:
		BasicArchive() 
		{
			_best_solution = new Solution();
		}

		~BasicArchive() 
		{
			delete _best_solution;
		}

		Solution* getElite()
		{
			return _best_solution;
		}

		bool updateBest(const Solution& new_best)
		{
			if (_best_size == 0)
			{
				_best_solution->copy(new_best);
				_best_size = 1;
				return true;
			}
			if (_comparer_pointer->isBetter(new_best.fitness, _best_solution->fitness))
			{
				_best_solution->copy(new_best);
				return true;
			}
			return false;
		}

		void setProblem(ProblemHandle* handle)
		{
			_comparer_pointer = handle->getSolutionComparer();
		}
	};

	// 偏序最优解档案管理器（自定义尺寸）
	class HistoricArchive final : public BestArchive
	{
	private:
		int _max_size;
		int _worst_id;

		void updateWorstId()
		{
			_worst_id = 0;
			for (int i = 0; i < _best_size; i++)
			{
				if (_comparer_pointer->isBetter(_best_solution[_worst_id].fitness, _best_solution[i].fitness))
				{
					_worst_id = i;
				}
			}
		}

		void updateBestSolution(int challenger_id)
		{
			if (_comparer_pointer->isBetter(_best_solution[challenger_id].fitness, _best_solution[0].fitness))
			{
				_best_solution[0].swap(_best_solution[challenger_id]);
			}
		}

	public:
		HistoricArchive(int max_size)
		{
			_max_size = max_size;
			_worst_id = -1;
			_best_solution = new Solution[_max_size + 1];
		}

		~HistoricArchive()
		{
			delete[] _best_solution;
		}

		Solution* getElite()
		{
			return _best_solution + (rand() % _best_size);
		}

		bool updateBest(const Solution& new_best)
		{
			// 当前档案未满，插入队尾
			if (_best_size < _max_size)
			{
				_best_solution[_best_size].copy(new_best);
				updateBestSolution(_best_size);
				_best_size++;

				if (_best_size == _max_size)
				{
					updateWorstId();
				}
				return true;
			}
			else // 档案已满，与最差解进行比较替换
			{
				if (_comparer_pointer->isBetter(new_best.fitness, _best_solution[_worst_id].fitness))
				{
					_best_solution[_worst_id].copy(new_best);
					updateBestSolution(_worst_id);
					updateWorstId();
					return true;
				}
			}

			return false;
		}

		void setProblem(ProblemHandle* handle)
		{

		}
	};

	// 多目标最优解管理器
	class MultiobjectArchive final :public BestArchive
	{
	public:
		MultiobjectArchive()
		{
			_best_solution = new Solution[PFSIZE + 1];
			_best_size = 0;
		}

		~MultiobjectArchive()
		{
			delete[] _best_solution;
		}

		Solution* getElite()
		{
			return _best_solution + (rand() % _best_size);
		}

		bool updateBest(const Solution& new_best)
		{
			//遍历Pareto集
			Solution* temp = (Solution*)malloc(sizeof(Solution));
			for (int i = 0; i < _best_size; i++)
			{
				// 被现有解支配或与现有解相同，则不是最优
				if (_comparer_pointer->isBetter(_best_solution[i].fitness, new_best.fitness) || _best_solution[i] == new_best)
					return false;

				// 支配现有解，移除被支配解
				if (_comparer_pointer->isBetter(new_best.fitness, _best_solution[i].fitness))
				{
					*temp = std::move(_best_solution[i]);
					_best_solution[i] = std::move(_best_solution[_best_size - 1]);
					_best_solution[_best_size - 1] = std::move(*temp);

					i--;
					_best_size--;
				}
			}

			free(temp);

			//Pareto集已满
			if (_best_size == (PFSIZE - 1))
				return false;

			_best_solution[_best_size++].copy(new_best);
			return true;
		}

		void setProblem(ProblemHandle* handle)
		{
			// 单一目标时，日志记录

			_comparer_pointer = handle->getSolutionComparer();
		}
	};

	// 单目标多模态最优解管理器
	class MultimodalArchive final :public BestArchive
	{
	private:
		double _accuracy_object;
		double _accuracy_decision;
		int _worst_id;

		bool isEffect(const Solution& a, const Solution b)
		{
			// 存在明显差异或是相同峰
			return _comparer_pointer->diff_max_priori(a.fitness, b.fitness) > _accuracy_object || eu_distance(a.result, b.result) < _accuracy_decision;
		}

	public:
		MultimodalArchive(double accuracy_object = 0.01, double accuracy_decision = 1)
		{
			_best_solution = new Solution[PKSIZE + 1];
			_best_size = 0;
			_accuracy_object = accuracy_object;
			_accuracy_decision = accuracy_decision;
			_worst_id = -1;
		}

		~MultimodalArchive()
		{
			delete[] _best_solution;
		}

		Solution* getElite()
		{
			return _best_solution + (rand() % _best_size);
		}

		bool updateBest(const Solution& new_best)
		{
			Solution* temp = (Solution*)malloc(sizeof(Solution));
			// 遍历峰值集
			for (int i = 0; i < _best_size; i++)
			{
				// 当前解明显差于峰值解或差于同峰峰值解
				if (_comparer_pointer->isBetter(_best_solution[i].fitness, new_best.fitness))
				{
					if (isEffect(_best_solution[i], new_best))
						return false;
				}
				// 当前解明显好于峰值解或好于同峰峰值解
				else
				{
					if (isEffect(_best_solution[i], new_best))
					{
						*temp = std::move(_best_solution[i]);
						_best_solution[i] = std::move(_best_solution[_best_size - 1]);
						_best_solution[_best_size - 1] = std::move(*temp);

						i--;
						_best_size--;
					}
				}
			}
			free(temp);
			// 峰值集已满
			if (_best_size == (PKSIZE - 1))
				return false;

			_best_solution[_best_size++].copy(new_best);
			return true;
		}

		void setProblem(ProblemHandle* handle)
		{
			_comparer_pointer = handle->getSolutionComparer();
		}
	};
}