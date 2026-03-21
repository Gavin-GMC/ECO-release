#include <algorithm>
#include"hungarian.h"
#include <cstring>
#include<climits>

namespace alglib {
    bool _hungarian_find(int a, int size, bool** adjacency_matrix, bool* vistited, int* matched)
    {
        for (int i = 0; i < size; i++)
        {
            if (adjacency_matrix[a][i] == 1 && !vistited[i]) //如果节点i与a相邻并且未被查找过
            {
                vistited[i] = true; //标记i为已查找过

                if (matched[i] == -1 //如果i未在前一个匹配M中
                    || _hungarian_find(matched[i], size, adjacency_matrix, vistited, matched)) //i在匹配M中，但是从与i相邻的节点出发可以有增广路
                {
                    matched[i] = a; //记录查找成功记录

                    return true;//返回查找成功
                }
            }
        }

        return false;
    }

    // 返回二分图的最优匹配
    int hungarian(bool** adjacency_matrix, int left_size, int right_size, std::vector<int>* assignment)
    {
        bool* visited = new bool[right_size];
        int* matched = new int[right_size];
        for (int i = 0; i < right_size; i++)
            matched[i] = -1;

        int result = 0;
        for (int i = 0; i < left_size; i++)
        {
            memset(visited, 0, right_size * sizeof(bool));
            if (_hungarian_find(i, right_size, adjacency_matrix, visited, matched))
                result++;
        }

        if (assignment != nullptr)
        {
            assignment->resize(left_size, -1);
            for (int i = 0; i < right_size; i++)
            {
                if (matched[i] == -1)
                    continue;
                (*assignment)[matched[i]] = i;
            }
        }

        delete[] visited;
        delete[] matched;
        return result;
    }

    template<typename T>
    bool _kuhnmunkres_find(int u, int size, T** cost_matrix, bool* visited_left, bool* visited_right, T* mark_left, T* mark_right, int* matched, T* slack)
    {
        visited_left[u] = 1;//标记进入增广路
        for (int v = 0; v < size; v++)
        {
            if (cost_matrix[u][v] >= 0 && !visited_right[v])//如果Y部的点还没进入增广路,并且存在路径
            {
                T t = mark_left[u] + mark_right[v] - cost_matrix[u][v];
                if (t == 0)//t为0说明是相等子图
                {
                    visited_right[v] = 1;//加入增广路

                    //如果Y部的点还未进行匹配,或者已经进行了匹配，可以从原来的匹配反向找到增广路，那就可以进行匹配
                    if (matched[v] == -1 || _kuhnmunkres_find(matched[v], size, cost_matrix, visited_left, visited_right, mark_left, mark_right, matched, slack))
                    {
                        matched[v] = u;//进行匹配
                        return true;
                    }
                }
                else if (t > 0)//此处t一定是大于0，因为顶标之和一定>=边权
                {
                    slack[v] = std::min(slack[v], t); //slack[v]存的是Y部的点需要变成相等子图顶标值最小增加多少
                }
            }
        }
        return false;
    }

    // 返回最大分配结果(cost_matrix的值不小于0为不可达)（后续考虑拓展至非方阵以及不可完全分配的情况）
    int kuhnmunkres(int** cost_matrix, int size, std::vector<int>* assignment)
    {
        // 空间申请
        int* mark_left = new int[size];
        int* mark_right = new int[size];
        int* matched = new int[size]; // 每个右值关联的左值
        bool* visited_left = new bool[size];
        bool* visited_right = new bool[size];
        int* slack = new int[size]; // 边权和顶标最小的差值

        // 数据初始值分配
        memset(matched, -1, size * sizeof(int));
        memset(mark_left, 0, size * sizeof(int)); // 左集的顶标为该点连接的边的最大权值
        memset(mark_right, 0, size * sizeof(int)); // 右集的顶标为0
        for (int i = 0; i < size; i++) // 预处理出顶标值
        {
            for (int j = 0; j < size; j++)
            {
                if (cost_matrix[i][j] < 0) // 不可分配
                    continue;
                mark_left[i] = std::max(mark_left[i], cost_matrix[i][j]);
            }
        }

        for (int i = 0; i < size; i++)//枚举X部的点
        {
            for (int s = 0; s < size; s++)
                slack[s] = INT_MAX;
            while (1)
            {
                memset(visited_left, 0, size * sizeof(bool));
                memset(visited_right, 0, size * sizeof(bool));
                if (_kuhnmunkres_find(i, size, cost_matrix, visited_left, visited_right, mark_left, mark_right, matched, slack))
                    break;//已经匹配正确

                // 找出还没经过的点中，需要变成相等子图的最小额外增加的顶标值
                int minz = INT_MAX;
                for (int j = 0; j < size; j++)
                    if (!visited_right[j] && minz > slack[j])
                        minz = slack[j];


                // 还未匹配，将X部的顶标减去minz，Y部的顶标加上minz
                for (int j = 0; j < size; j++)
                    if (visited_left[j])
                        mark_left[j] -= minz;
                for (int j = 0; j < size; j++)
                    if (visited_right[j])
                        mark_right[j] += minz;
                    else
                        slack[j] -= minz; // 修改顶标后，要把所有不在交错树中的Y顶点的slack值都减去minz
            }
        }

        if (assignment != nullptr)
        {
            assignment->resize(size);
            for (int i = 0; i < size; i++)
            {
                if (matched[i] == -1)
                    continue;
                (*assignment)[matched[i]] = i;
            }
        }

        double back = 0;//二分图最优匹配权值
        for (int i = 0; i < size; i++)
        {
            if (matched[i] == -1)
                continue;
            back += cost_matrix[matched[i]][i];
        }

        // 清理内存
        delete[] mark_left;
        delete[] mark_right;
        delete[] matched;
        delete[] visited_left;
        delete[] visited_right;
        delete[] slack;

        return back;
    };

    // 返回最大分配结果(cost_matrix的值不小于0为不可达)（后续考虑拓展至非方阵以及不可完全分配的情况）
    double kuhnmunkres(double** cost_matrix, int size, std::vector<int>* assignment)
    {
        // 空间申请
        double* mark_left = new double[size];
        double* mark_right = new double[size];
        int* matched = new int[size]; // 每个右值关联的左值
        bool* visited_left = new bool[size];
        bool* visited_right = new bool[size];
        double* slack = new double[size]; // 边权和顶标最小的差值

        // 数据初始值分配
        memset(matched, -1, size * sizeof(int));
        memset(mark_left, 0, size * sizeof(double)); // 左集的顶标为该点连接的边的最大权值
        memset(mark_right, 0, size * sizeof(double)); // 右集的顶标为0
        for (int i = 0; i < size; i++) // 预处理出顶标值
        {
            for (int j = 0; j < size; j++)
            {
                if (cost_matrix[i][j] < 0) // 不可分配
                    continue;
                mark_left[i] = std::max(mark_left[i], cost_matrix[i][j]);
            }
        }

        for (int i = 0; i < size; i++)//枚举X部的点
        {
            for (int s = 0; s < size; s++)
                slack[s] = INT_MAX;
            while (1)
            {
                memset(visited_left, 0, size * sizeof(bool));
                memset(visited_right, 0, size * sizeof(bool));
                if (_kuhnmunkres_find(i, size, cost_matrix, visited_left, visited_right, mark_left, mark_right, matched, slack))
                    break;//已经匹配正确

                // 找出还没经过的点中，需要变成相等子图的最小额外增加的顶标值
                int minz = INT_MAX;
                for (int j = 0; j < size; j++)
                    if (!visited_right[j] && minz > slack[j])
                        minz = slack[j];


                // 还未匹配，将X部的顶标减去minz，Y部的顶标加上minz
                for (int j = 0; j < size; j++)
                    if (visited_left[j])
                        mark_left[j] -= minz;
                for (int j = 0; j < size; j++)
                    if (visited_right[j])
                        mark_right[j] += minz;
                    else
                        slack[j] -= minz; // 修改顶标后，要把所有不在交错树中的Y顶点的slack值都减去minz
            }
        }

        if (assignment != nullptr)
        {
            assignment->resize(size);
            for (int i = 0; i < size; i++)
            {
                if (matched[i] == -1)
                    continue;
                (*assignment)[matched[i]] = i;
            }
        }

        double back = 0;//二分图最优匹配权值
        for (int i = 0; i < size; i++)
        {
            if (matched[i] == -1)
                continue;
            back += cost_matrix[matched[i]][i];
        }

        // 清理内存
        delete[] mark_left;
        delete[] mark_right;
        delete[] matched;
        delete[] visited_left;
        delete[] visited_right;
        delete[] slack;

        return back;
    };
}