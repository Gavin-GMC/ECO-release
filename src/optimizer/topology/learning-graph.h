//------------------------Description------------------------
// 学习图 LearningGraph:个体学习对象的拓扑(DAG)。每条边 = 一个起点个体 + 若干终点学习对象(解)。
//   终点为空指针表示"不学习、直接进入子代"。由 LearningTopology 产出,供 OffspringGenerator 消费。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <cstring>
#include "solution.h"
#include "individual.h"

namespace ECFlow
{
    // 终点为空指针表示不进行学习而是直接进入子代
    class LearningGraph
    {
    private:
        int _point_number;
        int _end_counter;
        int _end_point_size;
        int _graph_size;
        Individual** _start_point;
        Solution** _end_point;
    public:
        LearningGraph(size_t graph_size, size_t end_point_size)
        {
            _point_number = 0;
            _end_counter = 0;
            _graph_size = int(graph_size);
            _end_point_size = int(end_point_size);
            _start_point = new Individual * [graph_size];
            _end_point = new Solution * [graph_size * end_point_size];
        }

        ~LearningGraph()
        {
            delete[] _start_point;
            delete[] _end_point;
        }

        void addEdge(Individual* start_point, Solution** end_points)
        {
            // 越界检查
            //if (_point_number >= _graph_size)
            //	return;

            // 拷贝
            _start_point[_point_number] = start_point;
            memcpy(_end_point + _point_number * _end_point_size, end_points, _end_point_size * sizeof(Solution*));

            // 索引更新
            _point_number++;
        }

        void addStart(Individual* start_point)
        {
            _start_point[_point_number] = start_point;
            _end_counter = _point_number * _end_point_size;
            _point_number++;
        }

        void addEnd(Solution* end_point)
        {
            _end_point[_end_counter] = end_point;
            _end_counter++;
        }

        size_t getSize() { return _point_number; }

        size_t getEndSize() { return _end_point_size; }

        Individual* getStartPoint(int id) { return _start_point[id]; }

        Solution** getEndPoint(int id) { return _end_point + _end_point_size * id; }

        LearningGraph* copy()
        {
            // 1. 创建新的 LearningGraph 对象，初始化数组内存
            LearningGraph* new_graph = new LearningGraph(this->_graph_size, this->_end_point_size);

            // 2. 拷贝基本数据成员
            new_graph->_point_number = this->_point_number;
            new_graph->_end_counter = this->_end_counter;

            // 3. 拷贝 _start_point 数组的指针值（引用）
            memcpy(new_graph->_start_point, this->_start_point, this->_point_number * sizeof(Individual*));

            // 4. 拷贝 _end_point 数组的指针值（引用）
            memcpy(new_graph->_end_point, this->_end_point, this->_end_counter * sizeof(Solution*));

            return new_graph;
        }
    };
}
