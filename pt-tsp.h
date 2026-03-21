//------------------------Description------------------------
// This file defines the templates of travelling salesman problem for user,
//  which provides the interfaces to define specific problem.
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

namespace ECFC
{
	// template of Travelling Salesman Problem
	class PT_TSP
	{
	public:
		enum class DistanceType { Eula, Manhattan, Chebyshec};
		enum class RoundType { None, Ceil, Floor, Round };

	private:
		std::string _name;

		int _city_number;
		int _dimension;
		double* _position;
		double* _distance;
		DistanceType _dtype;
		RoundType _rtype;

		bool _symmetrical;
		bool _setted_distance;

		struct evaluateFunc : eccalcul_functor
		{
			int cityn;
			double* distance;

			evaluateFunc(int n, double* dist)
			{
				cityn = n;
				distance = new double[n * n + 1];
				
				memcpy(distance, dist, n * n * sizeof(double));
			}

			~evaluateFunc()
			{
				delete[] distance;
			}

			double operator()(double** a) const
			{
				double back = 0;
				int curr = a[0][0];
				int next;

				if (curr >= cityn || curr < 0)
					return 1e8;

				// 逐个访问
				for (int i = 1; i < cityn; i++)
				{					
					next = a[0][i];
					if (next >= cityn || next < 0)
						return 1e8;

					back += distance[curr * cityn + next];
					curr = next;
				}

				// 回到起始城市
				next = a[0][0];
				back += distance[curr * cityn + next];

				return back;
			}

			eccalcul_functor* copy()
			{
				return new evaluateFunc(cityn, distance);
			}
		};

		struct heuristicFunc : eccalcul_functor
		{
			int cityn;
			double* distance;

			heuristicFunc(int n, double* dist)
			{
				cityn = n;
				distance = new double[n * n + 1];

				memcpy(distance, dist, n * n * sizeof(double));
			}

			~heuristicFunc()
			{
				delete[] distance;
			}

			double operator()(double** a) const
			{
				int d_num = *(a[0]);
				if (d_num == 0) // 起始城市
				{
					return rand01();
				}

				int curr = a[2][d_num - 1];
				int next = int(*(a[1]));
				if (curr >= cityn || curr < 0)
					return -1;
				if (next >= cityn || next < 0)
					return -1;

				double dist = distance[curr * cityn + next];
				if (dist == 0)
					return ECFC_MAX;

				return 1 / dist;
			}

			eccalcul_functor* copy()
			{
				return new heuristicFunc(cityn, distance);
			}
		};

		evaluateFunc* _evaluate;
		heuristicFunc* _heuristic;

		void buildDistanceTable()
		{
			delete[] _distance;
			int table_size = _city_number * _city_number;
			_distance = new double[table_size + 1];

			switch (_dtype)
			{
			case DistanceType::Eula:
				for (int i = 0; i < _city_number; i++)
				{
					for (int j = 0; j < i; j++)
					{
						_distance[i * _city_number + j] = eu_distance(_position + i * _dimension, _position + j * _dimension, _dimension);
						_distance[j * _city_number + i] = _distance[i * _city_number + j];
					}
					_distance[i * _city_number + i] = ECFC_MAX;
				}
				break;
			case DistanceType::Manhattan:
				for (int i = 0; i < _city_number; i++)
				{
					for (int j = 0; j < i; j++)
					{
						_distance[i * _city_number + j] = man_distance(_position + i * _dimension, _position + j * _dimension, _dimension);
						_distance[j * _city_number + i] = _distance[i * _city_number + j];
					}
					_distance[i * _city_number + i] = ECFC_MAX;
				}
				break;
			default:
				break;
			}

			switch (_rtype)
			{
			case RoundType::None:
				break;
			case RoundType::Ceil:
				for (int i = 0; i < table_size; i++)
				{
					_distance[i] = std::ceil(_distance[i]);
				}
				break;
			case RoundType::Floor:
				for (int i = 0; i < table_size; i++)
				{
					_distance[i] = std::floor(_distance[i]);
				}
				break;
			case RoundType::Round:
				for (int i = 0; i < table_size; i++)
				{
					_distance[i] = std::round(_distance[i]);
				}
				break;
			default:
				break;
			}
		}

	public:
		PT_TSP()
		{
			_name = "tsp";

			_city_number = 0;
			_dimension = 0;
			_position = nullptr;
			_distance = nullptr;
			_dtype = DistanceType::Eula;
			_rtype = RoundType::None;
			_evaluate = nullptr;
			_heuristic = nullptr;

			_symmetrical = true;
			_setted_distance = false;
		}

		~PT_TSP()
		{
			delete[] _position;
			delete[] _distance;
			delete _evaluate;
			delete _heuristic;
		}

		void setName(std::string name)
		{
			_name = name;
		}

		void setCitys(double* city_position, int city_number, int map_dimension)
		{
			_city_number = city_number;
			_dimension = map_dimension;

			delete[] _position;
			_position = new double[_city_number * _dimension + 1];

			memcpy(_position, city_position, city_number * map_dimension * sizeof(double));
			_symmetrical = true;
			_setted_distance = false;
		}

		void setMap(double* distance, int city_number, bool symmetric)
		{
			_city_number = city_number;

			delete[] _distance;
			_distance = new double[_city_number * _city_number + 1];

			memcpy(_distance, distance, city_number * city_number * sizeof(double));
			_symmetrical = symmetric;
			_setted_distance = true;
		}

		void setEdgeWeightType(DistanceType dtype, RoundType rtype = RoundType::None)
		{
			_dtype = dtype;
			_rtype = rtype;
		}

		int getProblemSize()
		{
			return _city_number;
		}

		Problem* getProblem()
		{
			if (_city_number == 0)
				return nullptr;

			// 问题定义
			Problem* back = new Problem(_name);

			// 添加变量
			if (_symmetrical)
				back->addVariable("x", 0, _city_number - 1, 1, _city_number, 1, ECFC::VariableType::sequence_bidiagraph); // 对称图
			else
				back->addVariable("x", 0, _city_number - 1, 1, _city_number, 1, ECFC::VariableType::sequence_direction); // 有向图
			
			// 距离矩阵生成
			if(!_setted_distance)
				buildDistanceTable();

			// 添加目标函数，总距离最小化
			delete _evaluate;
			_evaluate = new evaluateFunc(_city_number, _distance);
			back->addObjective("length", 1, true, "x", _evaluate);

			// 添加启发函数，基于上一个选择以及距离的倒数
			delete _heuristic;
			_heuristic = new heuristicFunc(_city_number, _distance);
			back->addInspirationFunc("x", "x", _heuristic);
			
			// 添加约束
			back->addConstrainUnique("x", 1e6, "length"); // 每个城市限定访问一次
			back->addConstrainRange("x", ECFC::EMPTYVALUE, ECFC::EMPTYVALUE, 1e6, "length"); // 只能访问区域内的城市

			return back;
		}

		void save()
		{

		}

		void load(std::string name)
		{
			int city_number = 0;
			int demension = 0;
			double* city_map = nullptr;
			bool set_edge = false;
			bool symytric = true;

			setName(name);

			std::string s_buffer;

			std::fstream tsp_file;
			tsp_file.open("_pdata/tsp/" + name + ".tsp");

			while (tsp_file >> s_buffer)
			{
				if (s_buffer == "NAME" || s_buffer == "NAME:")
				{
					std::getline(tsp_file, s_buffer);
					continue;
				}
				if (s_buffer == "COMMENT" || s_buffer == "COMMENT:")
				{
					std::getline(tsp_file, s_buffer);
					continue;
				}


				if (s_buffer == "DIMENSION" || s_buffer == "DIMENSION:")
				{
					if (s_buffer.find(':') == std::string::npos)
						tsp_file >> s_buffer;

					tsp_file >> city_number;
					continue;
				}

				if (s_buffer == "TYPE" || s_buffer == "TYPE:")
				{
					if (s_buffer.find(':') == std::string::npos)
						tsp_file >> s_buffer;

					tsp_file >> s_buffer;
					if (s_buffer == "TSP")
					{
						symytric = true;
					}
					else if (s_buffer == "ATSP")
					{
						symytric = false;
					}
					else
					{
						sys_logger.error("Wrong data file!");
						return;
					}
					continue;
				}

				if (s_buffer == "EDGE_WEIGHT_TYPE" || s_buffer == "EDGE_WEIGHT_TYPE:")
				{
					if (s_buffer.find(':') == std::string::npos)
						tsp_file >> s_buffer;

					tsp_file >> s_buffer;
					if (s_buffer == "EXPLICIT") // 设置边
					{
						bool full = false;
						bool upper = false;
						bool diag = false;

						while (s_buffer != "EDGE_WEIGHT_SECTION")
						{
							tsp_file >> s_buffer;
							if (s_buffer == "EDGE_WEIGHT_FORMAT" || s_buffer == "EDGE_WEIGHT_FORMAT:")
							{
								if (s_buffer.find(':') == std::string::npos)
									tsp_file >> s_buffer;

								tsp_file >> s_buffer;

								if (s_buffer == "FULL_MATRIX")
								{
									full = true;
								}
								else if (s_buffer == "UPPER_ROW" || s_buffer == "LOWER_COL")
								{
									upper = true;
									diag = false;
								}
								else if (s_buffer == "LOWER_ROW" || s_buffer == "UPPER_COL")
								{
									upper = false;
									diag = false;
								}
								else if (s_buffer == "UPPER_DIAG_ROW" || s_buffer == "LOWER_DIAG_COL")
								{
									upper = true;
									diag = true;
								}
								else if (s_buffer == "LOWER_DIAG_ROW" || s_buffer == "UPPER_DIAG_COL")
								{
									upper = false;
									diag = true;
								}
								continue;
							}

							if (s_buffer == "DISPLAY_DATA_TYPE" || s_buffer == "DISPLAY_DATA_TYPE:")
							{
								std::getline(tsp_file, s_buffer);
								continue;
							}

							if (s_buffer == "NODE_COORD_TYPE" || s_buffer == "NODE_COORD_TYPE:")
							{
								std::getline(tsp_file, s_buffer);
								continue;
							}

						}


						city_map = new double[city_number * city_number];

						if (full)
						{
							int index = 0;
							for (int i = 0; i < city_number; i++)
							{
								for (int j = 0; j < city_number; j++)
								{
									tsp_file >> city_map[index++];
								}
							}
						}
						else
						{
							if (upper)
							{
								if (diag)
								{
									for (int i = 0; i < city_number; i++)
									{
										for (int j = i; j < city_number; j++)
										{
											tsp_file >> city_map[i * city_number + j];
											city_map[j * city_number + i] = city_map[i * city_number + j];
										}
									}
								}
								else
								{
									for (int i = 0; i < city_number; i++)
									{
										for (int j = i + 1; j < city_number; j++)
										{
											tsp_file >> city_map[i * city_number + j];
											city_map[j * city_number + i] = city_map[i * city_number + j];
										}
									}
								}
							}
							else
							{
								if (diag)
								{
									for (int i = 0; i < city_number; i++)
									{
										for (int j = 0; j <= i; j++)
										{
											tsp_file >> city_map[i * city_number + j];
											city_map[j * city_number + i] = city_map[i * city_number + j];
										}
									}
								}
								else
								{
									for (int i = 0; i < city_number; i++)
									{
										for (int j = 0; j < i; j++)
										{
											tsp_file >> city_map[i * city_number + j];
											city_map[j * city_number + i] = city_map[i * city_number + j];
										}
									}
								}
							}
						}

						for (int i = 0; i < city_number; i++)
						{
							city_map[i * city_number + i] = ECFC::EMPTYVALUE;
						}

						setMap(city_map, city_number, symytric);
						delete[] city_map;
						return;

					}
					else // 设置点
					{
						if (s_buffer == "EUC_2D" || s_buffer == "ATT")
						{
							demension = 2;
							setEdgeWeightType(ECFC::PT_TSP::DistanceType::Eula, ECFC::PT_TSP::RoundType::Round);
						}
						else if (s_buffer == "EUC_3D")
						{
							demension = 3;
							setEdgeWeightType(ECFC::PT_TSP::DistanceType::Eula, ECFC::PT_TSP::RoundType::Round);
						}
						else if (s_buffer == "MAX_2D")
						{
							demension = 2;
							setEdgeWeightType(ECFC::PT_TSP::DistanceType::Chebyshec, ECFC::PT_TSP::RoundType::Round);
						}
						else if (s_buffer == "MAX_3D")
						{
							demension = 3;
							setEdgeWeightType(ECFC::PT_TSP::DistanceType::Chebyshec, ECFC::PT_TSP::RoundType::Round);
						}
						else if (s_buffer == "MAN_2D")
						{
							demension = 2;
							setEdgeWeightType(ECFC::PT_TSP::DistanceType::Manhattan, ECFC::PT_TSP::RoundType::Round);
						}
						else if (s_buffer == "MAN_3D")
						{
							demension = 3;
							setEdgeWeightType(ECFC::PT_TSP::DistanceType::Manhattan, ECFC::PT_TSP::RoundType::Round);
						}
						else if (s_buffer == "CEIL_2D")
						{
							demension = 2;
							setEdgeWeightType(ECFC::PT_TSP::DistanceType::Eula, ECFC::PT_TSP::RoundType::Ceil);
						}
						else if (s_buffer == "GEO")
						{
							demension = 2;
							setEdgeWeightType(ECFC::PT_TSP::DistanceType::Eula, ECFC::PT_TSP::RoundType::None);
						}
						else
						{
                            sys_logger.error("Error in distance type during loading tsp problem");
                            return;
						}

						while (s_buffer != "NODE_COORD_SECTION")
						{
							tsp_file >> s_buffer;

							if (s_buffer == "DISPLAY_DATA_TYPE" || s_buffer == "DISPLAY_DATA_TYPE:")
							{
								std::getline(tsp_file, s_buffer);
								continue;
							}

							if (s_buffer == "EDGE_WEIGHT_FORMAT" || s_buffer == "EDGE_WEIGHT_FORMAT:")
							{
								std::getline(tsp_file, s_buffer);
								continue;
							}

							if (s_buffer == "FIXED_EDGES_SECTION")
							{
								while (s_buffer != "-1")
								{
									tsp_file >> s_buffer;
								}
								continue;
							}
						}

						city_map = new double[city_number * demension];
						int map_index = 0;
						for (int i = 0; i < city_number; i++)
						{
							tsp_file >> s_buffer;
							for (int j = 0; j < demension; j++)
							{
								tsp_file >> city_map[map_index];
								map_index++;
							}
						}

						setCitys(city_map, city_number, demension);

						delete[] city_map;
						return;
					}
				}
			}
		}

		void clear()
		{
			_name = "tsp_default";
			_city_number = 0;
			_dimension = 0;

			delete[] _position;
			_position = nullptr;
			delete[] _distance;
			_distance = nullptr;
			delete _evaluate;
			_evaluate = nullptr;
			delete _heuristic;
			_heuristic = nullptr;

			_dtype = DistanceType::Eula;
			_rtype = RoundType::None;
		}
	};
}
