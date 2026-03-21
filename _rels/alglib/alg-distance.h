#ifndef ALG_DISTANCE_H
#define ALG_DISTANCE_H
#include<algorithm>
#include<cmath>
#include<stdexcept>
#include<unordered_set>
#include<unordered_map>
#include"hungarian.h"
#include"double_comp.h"

namespace alglib {
    //return the eula distance between two points
    double eu_distance(double a[], double b[], size_t size = 2)
    {
        double back = 0;
        for (int i = 0; i < size; i++)
        {
            back += pow(a[i] - b[i], 2);
        }

        return sqrt(back);
    }

    //return the manhattan distance between two points
    double man_distance(double a[], double b[], size_t size = 2)
    {
        double back = 0;
        for (int i = 0; i < size; i++)
        {
            back += fabs(a[i] - b[i]);
        }
        return back;
    }

    //return the chebyshec distance between two points
    double che_distance(double a[], double b[], size_t size = 2)
    {
        double back = 0;
        double bu;
        for (int i = 0; i < size; i++)
        {
            bu = fabs(a[i] - b[i]);
            if (bu > back)
                back = bu;
        }
        return back;
    }

    // return the hamming distance between two points
    double hamming_distance(double a[], double b[], size_t size = 2, double accuracy = EQACCURACY)
    {
        double back = 0;
        for (int i = 0; i < size; i++)
        {
            if (std::abs(a[i] - b[i]) > accuracy)
                back++;
        }
        return back;
    }

    // return the cosine similarity between two points
    double cos_similarity(double a[], double b[], size_t size = 2)
    {
        double dot_product = 0.0;  // 点积
        double norm_a = 0.0;       // 向量 a 的模长平方
        double norm_b = 0.0;       // 向量 b 的模长平方

        for (size_t i = 0; i < size; ++i) {
            dot_product += a[i] * b[i];
            norm_a += a[i] * a[i];
            norm_b += b[i] * b[i];
        }

        if (norm_a == 0.0 || norm_b == 0.0) {
            throw std::invalid_argument("Vectors must have non-zero norms");
        }

        return dot_product / (std::sqrt(norm_a) * std::sqrt(norm_b));
    }

    // return the partition distance between set
    double partition_distance(int a[], int b[], size_t size = 2)
    {
        // 确定簇的数目
        std::unordered_set<int> uniqueValues(a, a + size);
        int a_partition = uniqueValues.size();
        std::unordered_map<int, int> a_id;
        int id = 0;
        for (int value : uniqueValues) {
            a_id[value] = id++;
        }

        uniqueValues.clear();
        uniqueValues.insert(b, b + size);
        int b_partition = uniqueValues.size();
        std::unordered_map<int, int> b_id;
        id = 0;
        for (int value : uniqueValues) {
            b_id[value] = id++;
        }
        // 构建分配矩阵
        int* allo_matrix = new int[a_partition * b_partition + 1];
        for (int i = 0; i < size; i++)
            for (int j = 0; j < size; j++)
                allo_matrix[i * size + j] = 0;

        for (int i = 0; i < size; i++)
            for (int j = 0; j < size; j++)
                allo_matrix[a_id[a[i]] * size + b_id[b[j]]] ++;

        // 求解分配问题
        int** cost_matrix = new int* [size];
        for (int i = 0; i < size; i++)
            cost_matrix[i] = allo_matrix + i * size;
        int best_match = alglib::kuhnmunkres(cost_matrix, size);
        delete[] cost_matrix;
        delete[] allo_matrix;

        // 返回分区距离
        return size - best_match;
    }


    // return the partition distance between set
    double partition_distance(double a[], double b[], size_t size = 2, double accuracy = EQACCURACY)
    {
        int* _a = new int[size];
        int* _b = new int[size];
        for (int i = 0; i < size; i++)
        {
            _a[i] = a[i] / accuracy;
            _b[i] = b[i] / accuracy;
        }
        int back = partition_distance(_a, _b, size);

        delete[] _a;
        delete[] _b;
        return back;
    }
}

#endif // !ALG_DISTANCE_H



