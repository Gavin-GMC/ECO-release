#include <vector>
#include <algorithm>

namespace alglib {
    // 返回二分图的最优匹配
    int hungarian(bool** adjacency_matrix, int left_size, int right_size, std::vector<int>* assignment = nullptr);

    // 返回最大分配结果(cost_matrix的值不小于0为不可达)（后续考虑拓展至非方阵以及不可完全分配的情况）
    int kuhnmunkres(int** cost_matrix, int size, std::vector<int>* assignment = nullptr);

    // 返回最大分配结果(cost_matrix的值不小于0为不可达)（后续考虑拓展至非方阵以及不可完全分配的情况）
    double kuhnmunkres(double** cost_matrix, int size, std::vector<int>* assignment = nullptr);
}