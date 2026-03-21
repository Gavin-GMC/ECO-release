//------------------------Description------------------------
// This file defines the configure list of ECFC optimizer. 
// It contains the categories and the required parameters 
// of each component.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include<vector>
#include<string>
#include<fstream>
#include<iostream>
#include<iomanip>
#include"ecfc-constant.h"
#include"initializer-type.h"
#include"bestarchive-type.h"
#include"crossover-type.h"
#include"individual-type.h"
#include"distribution-type.h"
#include"learning-strategy-type.h"
#include"learning-topology-type.h"
#include"mutation-type.h"
#include"offspring-generator-type.h"
#include"selector-type.h"
#include"subswarm-constructer-type.h"
#include"subswarm-manager-type.h"
#include"subswarm-topology-type.h"
#include"repaire-type.h"

namespace ECFC
{
	struct ConfigureList
	{
		struct PopulationConfig
		{
			// 子种群标记
			std::string tag;

			// 子种群规模
			int size;

			// 种群终止条件
			int terminate_conditions[3] = { -1, -1, -1 }; // FES, Convergence, Time

			// 子种群初始化方式
			InitializerType ini_type;

			// 子代生成组件设置
			OffspringGeneratorType lgenerator_type;
			double l_para[2 * PARANUM + 3];
			LearningTopologyType ltopology_type;
			double* ltopology_para;
			LearningStrategyType lstrategy_type;
			double* lstrategy_para;

			// 种群更新设置
			SelectorType selector_type;
			double selector_para[PARANUM];

			// 解修复策略
			RepairType repair_type;

			// 种群最优档案类型
			BestArchiveType archive_type;

			PopulationConfig()
			{
				tag = "";
				size = 0;
				terminate_conditions[0] = terminate_conditions[1] = terminate_conditions[2] = -1;
				ini_type = InitializerType::F_default;
				
				lgenerator_type = OffspringGeneratorType::F_default;
				ltopology_type = LearningTopologyType::F_default;
				lstrategy_type = LearningStrategyType::F_default;
				repair_type = RepairType::F_default;
				for (int i = 0; i < PARANUM * 2 + 3; i++)
				{
					l_para[i] = 0;
				}

				ltopology_para = l_para + 1;
				lstrategy_para = l_para + 2 + PARANUM;

				selector_type = SelectorType::F_default;
				for (int i = 0; i < PARANUM; i++)
				{
					selector_para[i] = 0;
				}

				archive_type = BestArchiveType::F_default;
			}

			PopulationConfig(const PopulationConfig& source)
			{
				tag = source.tag;
				size = source.size;
				memcpy(terminate_conditions, source.terminate_conditions, 3 * sizeof(int));

				ini_type = source.ini_type;

				lgenerator_type = source.lgenerator_type;
				ltopology_type = source.ltopology_type;
				lstrategy_type = source.lstrategy_type;
				repair_type = source.repair_type;
				for (int i = 0; i < PARANUM * 2 + 3; i++)
				{
					l_para[i] = source.l_para[i];
				}

				ltopology_para = l_para + 1;
				lstrategy_para = l_para + 2 + PARANUM;

				selector_type = source.selector_type;
				for (int i = 0; i < PARANUM; i++)
				{
					selector_para[i] = source.selector_para[i];
				}

				archive_type = source.archive_type;
			}
		};

		// 优化器名称
		std::string name;

		// 优化器终止条件
		int terminate_conditions[3]; // FES, Convergence, Time

		// 个体类型，多种群共用
		IndividualType individual_type;

		// 子种群参数
		std::vector<PopulationConfig> subpopulations;

		// 多种群策略组件设置
		SubswarmManagerType smanager_type;
		double s_para[PARANUM * 2 + 2];
		SubswarmConstructerType sbuilder_type;
		double* sbuilder_para = s_para + 1;
		SubswarmTopologyType stopology_type;
		double* stopology_para = s_para + 2 + PARANUM;

		// 优化器最优档案类型
		BestArchiveType archive_type;

		// 日志器参数
		double logger_para[PARANUM];

		ConfigureList()
		{
			clear();
		}

		void clear()
		{
			name = "defualt";
			terminate_conditions[0] = terminate_conditions[1] = terminate_conditions[2] = -1;

			individual_type = IndividualType::F_default;

			smanager_type = SubswarmManagerType::F_default;
			sbuilder_type = SubswarmConstructerType::F_default;
			stopology_type = SubswarmTopologyType::F_default;

			for (int i = 0; i < PARANUM * 2 + 2; i++)
			{
				s_para[i] = 0;
			}

			archive_type = BestArchiveType::F_default;

			subpopulations.clear();

			for (int i = 0; i < PARANUM; i++)
			{
				logger_para[i] = 0;
			}
		}

		void load(std::string file_name)
		{
			std::ifstream file;
			file.open(file_name, std::ios::in);
			
			// 加载失败
			if (!file)
			{
				sys_logger.error("An error occurred loading file " + file_name);
				return;
			}

			std::string in_buffer;
			std::string format;
			file >> in_buffer >> format;
			format.pop_back();

			if (format == "txt")
			{
				int type_buffer;

				file >> in_buffer >> name;

				file >> in_buffer >>
					in_buffer >> terminate_conditions[0] >>
					in_buffer >> terminate_conditions[1] >>
					in_buffer >> terminate_conditions[2];

				file >> in_buffer >> type_buffer;
				individual_type = IndividualType(type_buffer);

				file >> in_buffer >> type_buffer;
				smanager_type = SubswarmManagerType(type_buffer);
				
				file >> in_buffer >> type_buffer >> in_buffer;
				s_para[0] = type_buffer;
				sbuilder_type = SubswarmConstructerType(s_para[0]);
				for (int i = 0; i < PARANUM; i++)
				{
                    file >> in_buffer;
                    if (in_buffer == "e")
                        s_para[i + 1] = EMPTYVALUE;
                    else
                        s_para[i + 1] = std::stod(in_buffer);
				}

				file >> in_buffer >> type_buffer >> in_buffer;
				s_para[1 + PARANUM] = type_buffer;
				stopology_type = SubswarmTopologyType(s_para[1 + PARANUM]);
				for (int i = 0; i < PARANUM; i++)
				{
                    file >> in_buffer;
                    if (in_buffer == "e")
                        s_para[i + 2 + PARANUM] = EMPTYVALUE;
                    else
                        s_para[i + 2 + PARANUM] = std::stod(in_buffer);
				}

				file >> in_buffer >> type_buffer;
				archive_type = BestArchiveType(type_buffer);

				int swarm_number;
				file >> in_buffer >> swarm_number;
				file >> in_buffer;

				for (int i = 0; i < swarm_number; i++)
				{
					subpopulations.push_back(PopulationConfig());
					
					file >> in_buffer >> subpopulations[i].tag;
					file >> in_buffer >> subpopulations[i].size;

					file >> in_buffer >>
						in_buffer >> subpopulations[i].terminate_conditions[0] >>
						in_buffer >> subpopulations[i].terminate_conditions[1] >>
						in_buffer >> subpopulations[i].terminate_conditions[2];

					file >> in_buffer >> type_buffer;
					subpopulations[i].ini_type = InitializerType(type_buffer);

					file >> in_buffer >> type_buffer;
					subpopulations[i].lgenerator_type = OffspringGeneratorType(type_buffer);

					file >> in_buffer >> type_buffer >> in_buffer;
					subpopulations[i].l_para[0] = type_buffer;
					subpopulations[i].ltopology_type = LearningTopologyType(subpopulations[i].l_para[0]);
					for (int j = 0; j < PARANUM; j++)
					{
                        file >> in_buffer;
                        if (in_buffer == "e")
                            subpopulations[i].l_para[j + 1] = EMPTYVALUE;
                        else
                            subpopulations[i].l_para[j + 1] = std::stod(in_buffer);
					}

					file >> in_buffer >> type_buffer >> in_buffer;
					subpopulations[i].l_para[1 + PARANUM] = type_buffer;
					subpopulations[i].lstrategy_type = LearningStrategyType(subpopulations[i].l_para[PARANUM + 1]);
					for (int j = 0; j < PARANUM; j++)
					{
                        file >> in_buffer;
                        if (in_buffer == "e")
                            subpopulations[i].l_para[j + 2 + PARANUM] = EMPTYVALUE;
                        else
                            subpopulations[i].l_para[j + 2 + PARANUM] = std::stod(in_buffer);
					}

					file >> in_buffer >> type_buffer;
					subpopulations[i].l_para[2 + 2 * PARANUM] = type_buffer;
					subpopulations[i].repair_type = RepairType(type_buffer);

					file >> in_buffer >> type_buffer >> in_buffer;
					subpopulations[i].selector_type = SelectorType(type_buffer);
					for (int j = 0; j < PARANUM; j++)
					{
                        file >> in_buffer;
                        if (in_buffer == "e")
                            subpopulations[i].selector_para[j] = EMPTYVALUE;
                        else
                            subpopulations[i].selector_para[j] = std::stod(in_buffer);
					}

					file >> in_buffer >> type_buffer;
					subpopulations[i].archive_type = BestArchiveType(type_buffer);

					file >> in_buffer;
				}

				file >> in_buffer;
				for (int i = 0; i < PARANUM; i++)
				{
                    file >> in_buffer;
                    if (in_buffer == "e")
                        logger_para[i] = EMPTYVALUE;
                    else
                        logger_para[i] = std::stod(in_buffer);
				}
			}
		}

		void save(std::string file_name, std::string format = "txt")
		{
			std::ofstream file;
			file.open(file_name, std::ios::out);

			file <<"<ver= " << format << ">\n";

			if (format == "txt")
			{
				file <<"optimizer_name:\t" << name << std::endl;

				file << "terminator:\t" <<
					"FES:\t" << std::defaultfloat << std::setprecision(15) << terminate_conditions[0] << "\t" <<
					"Convergence:\t" << std::defaultfloat << std::setprecision(15) << terminate_conditions[1] << "\t" <<
					"Time: \t" << std::defaultfloat << std::setprecision(15) << terminate_conditions[2] << std::endl;

				file << "individual_type:\t" << int(individual_type) << std::endl;

				file << "subpopulation_manager:\t" << int(smanager_type) << std::endl;
				
				file << "subpopulation_formation:\t" << std::defaultfloat << std::setprecision(15)<< s_para[0] << "\t" <<
					"parameters:\t";
				for (int i = 0; i < PARANUM; i++)
				{
                    if (s_para[i + 1] == EMPTYVALUE)
                        file << "e\t";
                    else
                        file << std::defaultfloat << std::setprecision(15) << s_para[i + 1] << "\t";
				}
				file << std::endl;
				
				file << "communication_model:\t" << std::defaultfloat << std::setprecision(15) << s_para[1 + PARANUM] << "\t" <<
					"parameters:\t";
				for (int i = 0; i < PARANUM; i++)
				{
                    if (s_para[i + 2 + PARANUM] == EMPTYVALUE)
                        file << "e\t";
                    else
                        file << std::defaultfloat << std::setprecision(15) << s_para[i + 2 + PARANUM] << "\t";
				}
				file << std::endl;

				file << "best_archive:\t" << int(archive_type) << std::endl;
				
				file << "\n";
				file << "subpopulation_number:\t" << subpopulations.size() << std::endl;
				file << "------------------------------\n";
				for (int i = 0; i < subpopulations.size(); i++)
				{
					file << "subpopulation_tag:\t" << subpopulations[i].tag << std::endl;

					file << "subpopulation_size:\t" << subpopulations[i].size << std::endl;

					file << "terminator:\t" <<
						"FES:\t" << std::defaultfloat << std::setprecision(15) << subpopulations[i].terminate_conditions[0] << "\t" <<
						"Convergence:\t" << std::defaultfloat << std::setprecision(15) << subpopulations[i].terminate_conditions[1] << "\t" <<
						"Time: \t" << std::defaultfloat << std::setprecision(15) << subpopulations[i].terminate_conditions[2] << std::endl;

					file << "initialization_type:\t" << int(subpopulations[i].ini_type) << std::endl;

					file << "producing_frame:\t" << int(subpopulations[i].lgenerator_type) << std::endl;

					file << "learning_topology:\t" << subpopulations[i].l_para[0] << "\t" <<
						"parameters:\t";
					for (int j = 0; j < PARANUM; j++)
					{
                        if (subpopulations[i].l_para[j + 1] == EMPTYVALUE)
                            file << "e\t";
                        else
                            file << std::defaultfloat << std::setprecision(15) << subpopulations[i].l_para[j + 1] << "\t";
					}
					file << std::endl;

					file << "learning_strategy:\t" << subpopulations[i].l_para[1 + PARANUM] << "\t" <<
						"parameters:\t";
					for (int j = 0; j < PARANUM; j++)
					{
                        if (subpopulations[i].l_para[j + 2 + PARANUM] == EMPTYVALUE)
                            file << "e\t";
                        else
                            file << std::defaultfloat << std::setprecision(15) << subpopulations[i].l_para[j + 2 + PARANUM] << "\t";
					}
					file << std::endl;

					file << "repair_method:\t" << int(subpopulations[i].repair_type) << std::endl;

					file << "environment_selector:\t" << int(subpopulations[i].selector_type) << "\t" <<
						"parameters:\t";
					for (int j = 0; j < PARANUM; j++)
					{
                        if (subpopulations[i].selector_para[j] == EMPTYVALUE)
                            file << "e\t";
                        else
                            file << std::defaultfloat << std::setprecision(15) << subpopulations[i].selector_para[j] << "\t";
					}
					file << std::endl;
					
					file << "best_archive:\t" << int(subpopulations[i].archive_type) << std::endl;

					file << "------------------------------\n";
				}
				
				file << "logger_paras:\t";
				for (int i = 0; i < PARANUM; i++)
				{
                    if (logger_para[i] == EMPTYVALUE)
                        file << "e\t";
                    else
                        file << logger_para[i] << "\t";
				}
				file << std::endl;
			}
		}
	};
}
