#include"alg-rand.h"
#include<algorithm>
#include<random>

namespace alglib {
    std::mt19937 gen;  // 使用 Mersenne Twister 生成器

    void setseed(unsigned int seed)
    {
        gen.seed(seed);
        srand(seed);
    }

    //return a value between 0-1
    double rand01()
    {
        return double(rand()) / RAND_MAX;
    }

    //return a value between 0-1 but not be 0 or 1
    double rand01_()
    {
        return double(rand() + 1) / (double(RAND_MAX) + 2);
    }

    int wide_rand()
    {
        // 直接生成随机数
        return gen() % 2147483648;  // 取模限制范围
    }

    template<class T>
    void shuffle(T* arr, size_t n)
    {
        // 使用 std::shuffle 打乱数组
        std::shuffle(arr, arr + n, gen);
    }
}
