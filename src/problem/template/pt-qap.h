//------------------------Description------------------------
// 二次分配 (Quadratic Assignment Problem) 问题模板。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "pt-instance-path.h"   // v1.4.9:load 路径自适应(短名/完整路径)
#include"problem.h"
#include<fstream>
#include<sstream>
#include<string>
#include<cstring>
#include"logger.hpp"
#include<iomanip>

namespace ECFlow
{
    // template of Quadratic Assignment Problem

    class PT_QAP
    {
    private:
        std::string _name;

        int _facilities_number;
        int _location_number;
        std::vector<std::vector<double>> _flow_matrix;
        std::vector<std::vector<double>> _distance_matrix;

        struct evaluateFunc : eccalcul_functor
        {
            int facilities_number;
            std::vector<std::vector<double>> flow_matrix;
            std::vector<std::vector<double>> distance_matrix;

            evaluateFunc(int n, std::vector<std::vector<double>> f, std::vector<std::vector<double>> d)
            {
                facilities_number = n;
                
                flow_matrix = f;
                distance_matrix = d;
            }

            ~evaluateFunc()
            {
               
            }

            double operator()(double** a) const
            {
                double back = 0;
                double* allo = a[0];

                for (int i = 0; i < facilities_number; i++)
                {
                    if (allo[i] < 0)
                        return ECFLOW_MAX;
                }

                for (int i = 0; i < facilities_number; i++)
                {
                    for (int j = 0; j < facilities_number; j++)
                    {
                        back += flow_matrix[i][j] * distance_matrix[int(allo[i])][int(allo[j])];
                    }
                }
                return back;
            }

            eccalcul_functor* copy()
            {
                return new evaluateFunc(facilities_number, flow_matrix, distance_matrix);
            }
        };

        struct heuristicFunc : eccalcul_functor
        {
            std::vector<double> total_flow;
            std::vector<double> total_distance;

            heuristicFunc(std::vector<std::vector<double>> flow_matrix, std::vector<std::vector<double>> distance_matrix)
            {
                double total;

                int flow_size = flow_matrix.size();
                for (int i = 0; i < flow_size; i++)
                {
                    total = 0;
                    for (int j = 0; j < flow_size; j++)
                    {
                        total += flow_matrix[i][j];
                        total += flow_matrix[j][i];
                    }
                    total_flow.push_back(total);
                }

                int distance_size = distance_matrix.size();
                for (int i = 0; i < distance_size; i++)
                {
                    total = 0;
                    for (int j = 0; j < distance_size; j++)
                    {
                        total += distance_matrix[i][j];
                    }
                    total_distance.push_back(total);
                }
            }

            heuristicFunc(std::vector<double> f, std::vector<double> d)
            {
                total_flow = f;
                total_distance = d;
            }

            ~heuristicFunc()
            {
                
            }

            double operator()(double** a) const
            {
                return total_flow[int(*a[0])] * total_distance[int(*a[1])];
            }

            eccalcul_functor* copy()
            {
                return new heuristicFunc(total_flow, total_distance);
            }
        };

        evaluateFunc* _evaluate;
        heuristicFunc* _heuristic;

    public:
        PT_QAP()
        {
            _name = "qap";
            _facilities_number = 0;
            _location_number = 0;
            _flow_matrix.clear();
            _distance_matrix.clear();

            _evaluate = nullptr;
            _heuristic = nullptr;
        }

        ~PT_QAP()
        {
            delete _evaluate;
            delete _heuristic;
        }

        void setName(std::string name)
        {
            _name = name;
        }

        void setFacilities(int facilities_number, double* flow_matrix)
        {
            // 验证设施数量是否有效
            if (facilities_number <= 0) {
                throw std::invalid_argument("facility count must be a positive integer");
            }

            // 释放原有内存（如果有）
            _facilities_number = facilities_number;
            _flow_matrix.clear();
            _flow_matrix.resize(facilities_number, std::vector<double>(facilities_number, 0.0));

            // 复制流量矩阵数据（假设输入为一维数组，按行优先存储）
            for (int i = 0; i < facilities_number; ++i) {
                for (int j = 0; j < facilities_number; ++j) {
                    int index = i * facilities_number + j;
                    _flow_matrix[i][j] = flow_matrix[index];
                }
            }
        }

        void setFacilities(int facilities_number, std::vector<std::vector<double>> flow_matrix)
        {
            // 验证设施数量是否有效
            if (facilities_number <= 0) {
                throw std::invalid_argument("facility count must be a positive integer");
            }

            // 验证流量矩阵维度是否正确
            if (flow_matrix.size() != static_cast<size_t>(facilities_number)) {
                throw std::invalid_argument("flow matrix row count does not match facility count");
            }

            for (const auto& row : flow_matrix) {
                if (row.size() != static_cast<size_t>(facilities_number)) {
                    throw std::invalid_argument("flow matrix column count does not match facility count");
                }
            }

            // 保存设施数量和流量矩阵
            _facilities_number = facilities_number;
            _flow_matrix = flow_matrix;
        }

        void setLocations(int location_number, double* distance_matrix)
        {
            // 验证位置数量是否有效
            if (location_number <= 0) {
                throw std::invalid_argument("location count must be a positive integer");
            }

            // 释放原有内存（如果有）
            _location_number = location_number;
            _distance_matrix.clear();
            _distance_matrix.resize(location_number, std::vector<double>(location_number, 0.0));

            // 复制距离矩阵数据（假设输入为一维数组，按行优先存储）
            for (int i = 0; i < location_number; ++i) {
                for (int j = 0; j < location_number; ++j) {
                    int index = i * location_number + j;
                    _distance_matrix[i][j] = distance_matrix[index];
                }
            }
        }

        void setLocations(int location_number, std::vector<std::vector<double>> distance_matrix)
        {
            // 验证位置数量是否有效
            if (location_number <= 0) {
                throw std::invalid_argument("location count must be a positive integer");
            }

            // 验证距离矩阵维度是否正确
            if (distance_matrix.size() != static_cast<size_t>(location_number)) {
                throw std::invalid_argument("distance matrix row count does not match location count");
            }

            for (const auto& row : distance_matrix) {
                if (row.size() != static_cast<size_t>(location_number)) {
                    throw std::invalid_argument("distance matrix column count does not match location count");
                }
            }

            // 保存位置数量和距离矩阵
            _location_number = location_number;
            _distance_matrix = distance_matrix;
        }

        Problem* getProblem()
        {
            // 问题定义
            Problem* back = new Problem(_name);

            // 添加变量
            back->addVariable("allocation", 0, _location_number-1, 1, _facilities_number);

            // 添加目标函数，总成本最小化
            delete _evaluate;
            _evaluate = new evaluateFunc(_facilities_number, _flow_matrix, _distance_matrix);
            back->addObjective("cost", 1, true, "allocation", _evaluate);

            // 添加启发函数，基于总流和总距离
            delete _heuristic;
            _heuristic = new heuristicFunc(_flow_matrix, _distance_matrix);
            back->addInspirationFunc("allocation", "allocation", _heuristic);

            // 添加约束
            back->addConstrainUnique("allocation", 1e6, "cost"); // 每个位置只能分配一次
            back->addConstrainRange("allocation", ECFlow::EMPTYVALUE, ECFlow::EMPTYVALUE, 1e6, "cost"); // 只能分配到指定的位置

            return back;
        }

        void save(bool overwrite = false)
        {
            std::string file_name = "_pdata/qap/" + _name + ".qap";

            if (!overwrite && file_exist(file_name))
            {
                sys_logger.error("Save failed! The qap problem file already exists and overwriting is not allowed.");
                return;
            }

            std::ofstream file(file_name);

            // 输出文件头部信息
            file << "NAME: " << _name << std::endl;
            file << "TYPE: QAP" << std::endl;
            file << "FACILITIES: " << _facilities_number << std::endl;
            file << "LOCATIONS: " << _location_number << std::endl;

            int width;
            width = std::to_string(_facilities_number).length();
            // 输出流量矩阵
            file << "FLOW_SECTION" << std::endl;
            for (int i = 0; i < _facilities_number; ++i) {
                file << std::setw(width) << (i + 1) << "  ";  // 行号右对齐
                for (int j = 0; j < _facilities_number; ++j) {
                    file << _flow_matrix[i][j];
                    if (j < _facilities_number - 1) {
                        file << " ";  // 数值间用空格分隔
                    }
                }
                file << std::endl;
            }

            width = std::to_string(_location_number).length();
            // 输出距离矩阵
            file << "Distance_SECTION" << std::endl;
            for (int i = 0; i < _location_number; ++i) {
                file << std::setw(width) << (i + 1) << "  ";  // 行号右对齐
                for (int j = 0; j < _location_number; ++j) {
                    file << _distance_matrix[i][j];
                    if (j < _location_number - 1) {
                        file << " ";  // 数值间用空格分隔
                    }
                }
                file << std::endl;
            }

            // 文件结束标记
            file << "EOF" << std::endl;

            file.close();

            sys_logger.info("Saved QAP data to " + file_name);
        }

        void load(std::string name)
        {
            clear();

            setName(instanceName(name));

            std::ifstream file(resolveInstancePath(name, "qap", "qap"));
            if (!file.is_open()) {
                sys_logger.error("Error: Could not open question data file!");
                return;
            }

            std::string s_buffer;

            // 读取文件的每一行
            while (file >> s_buffer) {
                if (s_buffer == "NAME" || s_buffer == "NAME:")
                {
                    std::getline(file, s_buffer);
                    continue;
                }
                if (s_buffer == "COMMENT" || s_buffer == "COMMENT:")
                {
                    std::getline(file, s_buffer);
                    continue;
                }
                if (s_buffer == "TYPE" || s_buffer == "TYPE:")
                {
                    if (s_buffer.find(':') == std::string::npos)
                        file >> s_buffer;

                    file >> s_buffer;
                    if (s_buffer != "QAP")
                    {
                        sys_logger.error("Wrong data file!");
                        return;
                    }
                    continue;
                }
                if (s_buffer == "FACILITIES" || s_buffer == "FACILITIES:")
                {
                    if (s_buffer.find(':') == std::string::npos)
                        file >> s_buffer;

                    file >> _facilities_number;
                    continue;

                }
                if (s_buffer == "LOCATIONS" || s_buffer == "LOCATIONS:")
                {
                    if (s_buffer.find(':') == std::string::npos)
                        file >> s_buffer;

                    file >> _location_number;
                    continue;
                }
                if (s_buffer == "EOF")
                {
                    break; // 文件结束
                }

                if (s_buffer == "FLOW_SECTION")
                {
                    _flow_matrix.resize(_facilities_number);
                    for (int i = 0; i < _facilities_number; i++)
                    {
                        _flow_matrix[i].resize(_facilities_number);
                        file >> s_buffer; // 行号

                        for (int j = 0; j < _facilities_number; j++)
                        {
                            file >> _flow_matrix[i][j];
                        }
                    }
                    continue;
                }
                if (s_buffer == "Distance_SECTION")
                {
                    _distance_matrix.resize(_location_number);
                    for (int i = 0; i < _location_number; i++)
                    {
                        _distance_matrix[i].resize(_location_number);
                        file >> s_buffer; // 行号

                        for (int j = 0; j < _location_number; j++)
                        {
                            file >> _distance_matrix[i][j];
                        }
                    }
                    continue;
                }
            }

            file.close();

            // 验证数据完整性
            if (_flow_matrix.size() != _facilities_number ||
                _distance_matrix.size() != _location_number ||
                _flow_matrix[0].size() != _facilities_number ||
                _distance_matrix[0].size() != _location_number) {
                sys_logger.error("Error: Incomplete or inconsistent data in file!");
                // 清空已读取的数据
                clear();
            }
        }

        void clear()
        {
            _name = "qap";
            _facilities_number = 0;
            _location_number = 0;
            _flow_matrix.clear();
            _distance_matrix.clear();

            delete _evaluate;
            _evaluate = nullptr;
            delete _heuristic;
            _heuristic = nullptr;
        }
    };
}
