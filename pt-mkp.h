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
#include<iomanip>

namespace ECFC
{
	// template of Multi-dimension and Multiple Knapsack Problem
	class PT_MKP
	{
	private:
		std::string _name;
		
		int _resource_number;
		// 背包集
		int _backpack_number;
		int _backpack_buffersize;
		double* _backpacks;
		// 物品集
		int _item_number;
		int _item_buffersize;
		double* _items_resource;
		int _profit_buffersize;
		double* _item_profits;

		bool _high_value_priori;

		struct evaluateFunc : eccalcul_functor
		{
			int item_number;
			double* profits;
			double none;

			evaluateFunc(int n, double* p, double none_tag = EMPTYVALUE)
			{
				item_number = n;
				profits = new double[n + 1];

				memcpy(profits, p, n * sizeof(double));
                none = none_tag;
			}

			~evaluateFunc()
			{
				delete[] profits;
			}

			double operator()(double** a) const
			{
				double back = 0;
				for (int i = 0; i < item_number; i++)
				{
					if (a[0][i] != none && a[0][i] > 0)
					{
						back += profits[i];
					}
				}

				return back;
			}

			eccalcul_functor* copy()
			{
				return new evaluateFunc(item_number, profits, none);
			}
		};

		struct heuristicFunc : eccalcul_functor
		{
			// 对于每个物品有两个侧重点一个是占用资源，一个是物品收益
			// 候选解的表达方式是0/none表示未分配，其余表示分配到的位置
			// 多维背包：具有多种约束，一个背包，启发式应作用于物品的放入顺序（单位价值高的物品先放）
			// 多背包：具有一种约束，多个背包，启发式应作用于背包的选择顺序（越合适的背包越先选择）
			// 
			// 事实上，在优化过程中，只有第二种方式是可接受的，因为问题编码中物品的置入顺序是固定的（这个只能体现在评估中）
			// 
			
			int item_number;
			int backpack_number;
			double* h_matrix;

			heuristicFunc(int resource_n, int backpack_n, double* backpacks, int item_n, double* items_r)
			{
				item_number = item_n;
				backpack_number = backpack_n + 1; // 还有背包0（不选择）
				h_matrix = new double[item_n * backpack_number + 1];

				double total_resource;
				double* backpack_resource_hold = new double[backpack_n * resource_n]; // 背包各资源在该资源总数中的占比
				for (int r = 0; r < resource_n; r++)
				{
					total_resource = 0;
					for (int i = 0; i < backpack_n; i++)
					{
						total_resource += backpacks[i * resource_n + r];
					}
					for (int i = 0; i < backpack_n; i++)
					{
						backpack_resource_hold[i * resource_n + r] = backpacks[i * resource_n + r] / total_resource;
					}
				}

				double* backpack_resource_rate = new double[backpack_n * resource_n]; // 背包中各资源的占比
				for (int i = 0; i < backpack_n; i++)
				{
					total_resource = 0;
					for (int r = 0; r < resource_n; r++)
					{
						total_resource += backpacks[i * resource_n + r];
					}
					for (int r = 0; r < resource_n; r++)
					{
						backpack_resource_rate[i * resource_n + r] = backpacks[i * resource_n + r] / total_resource;
					}
				}

				double* item_resource_rate = new double[item_n * resource_n]; // 物品中各资源的占比
				for (int i = 0; i < item_n; i++)
				{
					total_resource = 0;
					for (int r = 0; r < resource_n; r++)
					{
						total_resource += items_r[i * resource_n + r];
					}
					for (int r = 0; r < resource_n; r++)
					{
						item_resource_rate[i * resource_n + r] = items_r[i * resource_n + r] / total_resource;
					}
				}

				// 启发式矩阵计算
				int h_index;
				double min_h;
				for (int i = 0; i < item_number; i++)
				{
					h_matrix[i * backpack_number] = 0;

					for (int b = 0; b < backpack_n; b++)
					{
						h_index = i * backpack_number + b + 1;

						h_matrix[h_index] = 0;
						for (int r = 0; r < resource_n; r++)
						{
							h_matrix[h_index] += backpack_resource_hold[b * resource_n + r] * item_resource_rate[i * resource_n + r];
							h_matrix[h_index] -= abs(item_resource_rate[i * resource_n + r] - backpack_resource_rate[b * resource_n + r]);
						}
					}

					// 偏置以消除负值
					min_h = *std::min_element(h_matrix + i * backpack_number + 1, h_matrix + i * backpack_number + backpack_number);
					if (min_h < 0)
					{
						min_h *= 2;
						for (int b = 0; b < backpack_n; b++)
						{
							h_index = i * backpack_number + b + 1;
							h_matrix[h_index] -= min_h;
						}
					}
				}

				// 清理内存
				delete[] backpack_resource_hold;
				delete[] backpack_resource_rate;
				delete[] item_resource_rate;

			}

			heuristicFunc(int item_n, int backpack_n, double* h)
			{
				item_number = item_n;
				backpack_number = backpack_n;
				int matrix_size = item_n * backpack_n;
				h_matrix = new double[matrix_size + 1];
				memcpy(h_matrix, h, matrix_size);
			}

			~heuristicFunc()
			{
				delete[] h_matrix;
			}

			double operator()(double** a) const
			{
				int index = (*a[0]) * backpack_number + (*a[1]);
				return h_matrix[index];
			}

			eccalcul_functor* copy()
			{
				return new heuristicFunc(item_number, backpack_number, h_matrix);
			}
		};

		evaluateFunc* _evaluate;
		heuristicFunc* _heuristic;

		void _value_sort()
		{
			sortHelper<int, double>* sortbuffer = new sortHelper<int, double>[_item_number + 1];

			double* total_resource = new double[_resource_number + 1]; // 背包各资源总数
			for (int r = 0; r < _resource_number; r++)
			{
				total_resource[r] = 0;
				for (int i = 0; i < _backpack_number; i++)
				{
					total_resource[r] += _backpacks[i * _resource_number + r];
				}
			}

			// 各物品单位价值计算
			double total_weight;
			double* weights;
			for (int i = 0; i < _item_number; i++)
			{
				total_weight = 0;
				weights = _items_resource + i * _resource_number;

				for (int r = 0; r < _resource_number; r++)
				{
					total_weight += weights[r] / total_resource[r];
				}

				sortbuffer[i] = sortHelper<int, double>(i, -1 * _item_profits[i] / total_weight);
			}
			
			// 排序
			std::sort(sortbuffer, sortbuffer + _item_number);

			// 记录重构
			double* new_items = new double[_item_buffersize];
			double* new_items_profits = new double[_profit_buffersize];
			for (int i = 0; i < _item_number; i++)
			{
				memcpy(new_items + i * _resource_number, _items_resource + sortbuffer[i].id * _resource_number, sizeof(double) * _resource_number);
				new_items_profits[i] = _item_profits[sortbuffer[i].id];
			}
			std::swap(new_items, _items_resource);
			std::swap(new_items_profits, _item_profits);

			// 内存释放
			delete[] sortbuffer;
			delete[] total_resource;
			delete[] new_items;
			delete[] new_items_profits;
		}

	public:
		PT_MKP()
		{
			_name = "mkp";

			_resource_number = 0;
			// 背包集
			_backpack_number = 0;
			_backpack_buffersize = 0;
			_backpacks = nullptr;
			// 物品集
			_item_number = 0;
			_item_buffersize = 0;
			_items_resource = nullptr;
			_profit_buffersize = 0;
			_item_profits = nullptr;

			_evaluate = nullptr;
			_heuristic = nullptr;
			_high_value_priori = false;
		}

		~PT_MKP()
		{
			delete[] _backpacks;
			delete[] _items_resource;
			delete[] _item_profits;
			delete _evaluate;
			delete _heuristic;
		}

		void setName(std::string name)
		{
			_name = name;
		}

		void setResourceNumber(int resource_number)
		{
			_resource_number = resource_number;

			_backpack_number = 0;
			_item_number = 0;
		}

		void addBackpacks(int backpack_number, double* resource_capcity)
		{
			// 空间申请
			int backpack_capacity = _backpack_buffersize / _resource_number;
			int buffer_left = backpack_capacity - _backpack_number;
			if (buffer_left < backpack_number)
			{
				int new_size = std::max(_backpack_number + backpack_number, 2 * backpack_capacity) * _resource_number;

				double* new_buffer = new double[new_size];
				// 空间置换
				memcpy(new_buffer, _backpacks, sizeof(double) * _resource_number * _backpack_number);
				delete[] _backpacks;
				_backpacks = new_buffer;
				_backpack_buffersize = new_size;
			}

			// 新背包添加
			memcpy(_backpacks + _resource_number * _backpack_number, resource_capcity, sizeof(double) * _resource_number * backpack_number);
			_backpack_number += backpack_number;
		}

		void addItems(int item_number, double* profits, double* resource_occupy)
		{
			// 空间申请
			int item_capacity = _item_buffersize / _resource_number;
			int buffer_left = item_capacity - _item_number;
			if (buffer_left < item_number)
			{
				int new_size = std::max(_item_number + item_number, 2 * item_capacity) * _resource_number;

				double* new_buffer = new double[new_size];
				// 空间置换
				memcpy(new_buffer, _items_resource, sizeof(double) * _resource_number * _item_number);
				delete[] _items_resource;
				_items_resource = new_buffer;
				_item_buffersize = new_size;
			}
			buffer_left = _profit_buffersize - _item_number;
			if (buffer_left < item_number)
			{
				int new_size = std::max(_item_number + item_number, 2 * _profit_buffersize);

				double* new_buffer = new double[new_size];
				// 空间置换
				memcpy(new_buffer, _item_profits, sizeof(double) * _item_number);
				delete[] _item_profits;
				_item_profits = new_buffer;
				_profit_buffersize = new_size;
			}

			// 新物品添加
			memcpy(_item_profits + _item_number, profits, sizeof(double) * item_number);
			memcpy(_items_resource + _item_number * _resource_number, resource_occupy, sizeof(double) * _resource_number * item_number);
			_item_number += item_number;
		}

		void setItemValueSort(bool sorted = true)
		{
			_high_value_priori = sorted;
		}

		Problem* getProblem()
		{
			// 问题定义
			Problem* back = new Problem(_name);

			// 添加变量
			back->addVariable("x", 0, _backpack_number, 1, _item_number);

			// 物品贪心
			if (_high_value_priori)
				_value_sort();

			// 数据提取与结构化
			double* backpack_resource;
			int total_backpack = _backpack_number + 1;
			backpack_resource = new double[_resource_number * total_backpack];
			double* backpack_pointer = backpack_resource;
			for (int i = 0; i < _resource_number; i++)
			{
				backpack_pointer[0] = ECFC_MAX; // 默认缺省背包
				backpack_pointer += 1;
				for (int j = 0; j < _backpack_number; j++)
				{
					backpack_pointer[j] = _backpacks[j * _resource_number + i];
				}
				backpack_pointer += _backpack_number;
			}

			double* item_occupy = new double[_item_number * _resource_number];
			double* item_pointer = item_occupy;
			for (int i = 0; i < _resource_number; i++)
			{
				for (int j = 0; j < _item_number; j++)
				{
					item_pointer[j] = _items_resource[j * _resource_number + i];
				}
				item_pointer += _item_number;
			}

			// 添加目标函数，总价值最大化
			delete _evaluate;
			_evaluate = new evaluateFunc(_item_number, _item_profits);
			back->addObjective("profits", 1, false, "x", _evaluate);

			// 添加启发函数，基于上一个选择以及距离的倒数
			delete _heuristic;
			_heuristic = new heuristicFunc(_resource_number, _backpack_number, _backpacks, _item_number, _items_resource);
			back->addInspirationFunc("x", "x", _heuristic);

			// 添加约束
			backpack_pointer = backpack_resource;
			item_pointer = item_occupy;
			for (int i = 0; i < _resource_number; i++) // 资源约束
			{
				back->addConstrainCapacity("x", backpack_pointer, total_backpack, item_pointer, _item_number, 1, "profits");
				backpack_pointer += total_backpack;
				item_pointer += _item_number;
			}
			back->addConstrainRange("x", ECFC::EMPTYVALUE, ECFC::EMPTYVALUE, 1e6, "profits"); // 只能分配到指定的背包

			// 清理内存
			delete[] item_occupy;
			delete[] backpack_resource;

			return back;
		}

		void save(bool overwrite = false)
		{
			std::string file_name = "_pdata/mkp/" + _name + ".mkp";

			if (!overwrite && file_exist(file_name))
			{
				sys_logger.error("Save failed! The mkp problem file already exists and overwriting is not allowed.");
				return;
			}

			std::fstream mkp_file;
			mkp_file.open(file_name, std::ios::out);

			mkp_file << "NAME: " << _name << std::endl;
			mkp_file << "TYPE: MKP" << std::endl;
			mkp_file << "ITEMS: " << _item_number << std::endl;
			mkp_file << "BACKPACKS: " << _backpack_number << std::endl;
			mkp_file << "RESOURCES: " << _resource_number << std::endl;

			int weight;
			weight = std::to_string(_item_number).length();
			mkp_file << "PROFIT_SECTION" << std::endl;
			for (int i = 0; i < _item_number; i++)
			{
				mkp_file << std::setw(weight) << std::right << (i + 1) << " " << _item_profits[i] << std::endl;
			}

			mkp_file << "RESOURCE_SECTION" << std::endl;
			for (int i = 0; i < _item_number; i++)
			{
				mkp_file << std::setw(weight) << std::right << (i + 1);
				for (int r = 0; r < _resource_number; r++)
				{
					mkp_file << " " << _items_resource[i * _resource_number + r];
				}
				mkp_file << std::endl;
			}
			
			weight = std::to_string(_backpack_number).length();
			mkp_file << "CAPACITY_SECTION" << std::endl;
			for (int i = 0; i < _backpack_number; i++)
			{
				mkp_file << std::setw(weight) << std::right << (i + 1);
				for (int r = 0; r < _resource_number; r++)
				{
					mkp_file << " " << _backpacks[i * _resource_number + r];
				}
				mkp_file << std::endl;
			}

			mkp_file << "EOF";
		}

		void load(std::string name)
		{
			clear();

			setName(name);

			_high_value_priori = true;

			std::string s_buffer;

			std::fstream mkp_file;
			mkp_file.open("_pdata/mkp/" + name + ".mkp");

			while (mkp_file >> s_buffer)
			{
				if (s_buffer == "NAME" || s_buffer == "NAME:")
				{
					std::getline(mkp_file, s_buffer);
					continue;
				}
				if (s_buffer == "COMMENT" || s_buffer == "COMMENT:")
				{
					std::getline(mkp_file, s_buffer);
					continue;
				}
				if (s_buffer == "TYPE" || s_buffer == "TYPE:")
				{
					if (s_buffer.find(':') == std::string::npos)
						mkp_file >> s_buffer;

					mkp_file >> s_buffer;
					if (s_buffer != "MKP")
					{
						sys_logger.error("Wrong data file!");
						return;
					}
					continue;
				}
				if (s_buffer == "ITEMS" || s_buffer == "ITEMS:")
				{
					if (s_buffer.find(':') == std::string::npos)
						mkp_file >> s_buffer;

					mkp_file >> _item_number;
					continue;
				}
				if (s_buffer == "BACKPACKS" || s_buffer == "BACKPACKS:")
				{
					if (s_buffer.find(':') == std::string::npos)
						mkp_file >> s_buffer;

					mkp_file >> _backpack_number;
					continue;
				}
				if (s_buffer == "RESOURCES" || s_buffer == "RESOURCES:")
				{
					if (s_buffer.find(':') == std::string::npos)
						mkp_file >> s_buffer;

					mkp_file >> _resource_number;
					continue;
				}
				if (s_buffer == "OPTIMAL" || s_buffer == "OPTIMAL:")
				{
					std::getline(mkp_file, s_buffer);
					continue;
				}
				if (s_buffer == "PROFIT_SECTION")
				{
					if (_profit_buffersize < _item_number)
					{
						delete[] _item_profits;
						_item_profits = new double[_item_number];
						_profit_buffersize = _item_number;
					}

					for (int i = 0; i < _item_number; i++)
					{
						mkp_file >> s_buffer >> _item_profits[i];
					}
				}
				if (s_buffer == "RESOURCE_SECTION")
				{
					if (_item_buffersize < _item_number * _resource_number)
					{
						delete[] _items_resource;
						_items_resource = new double[_item_number * _resource_number];
						_item_buffersize = _item_number * _resource_number;
					}

					double* rp;
					for (int i = 0; i < _item_number; i++)
					{
						mkp_file >> s_buffer;
						rp = _items_resource + i * _resource_number;
						for (int r = 0; r < _resource_number; r++)
						{
							mkp_file >> rp[r];
						}
					}
				}
				if (s_buffer == "CAPACITY_SECTION")
				{
					if (_backpack_buffersize < _backpack_number * _resource_number)
					{
						delete[] _backpacks;
						_backpacks = new double[_backpack_number * _resource_number];
						_backpack_buffersize = _backpack_number * _resource_number;
					}

					double* rp;
					for (int b = 0; b < _backpack_number; b++)
					{
						mkp_file >> s_buffer;
						rp = _backpacks + b * _resource_number;
						for (int r = 0; r < _resource_number; r++)
						{
							mkp_file >> rp[r];
						}
					}
				}
				if (s_buffer == "EOF")
				{
					return;
				}
			}
		}

		void clear()
		{
			_name = "mkp";
			_resource_number = 0;
			_backpack_number = 0;
			_item_number = 0;
			delete _evaluate;
			_evaluate = nullptr;
			delete _heuristic;
			_heuristic = nullptr;

			_high_value_priori = false;
		}
	};
}
