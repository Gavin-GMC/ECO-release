#ifndef RANDOMLIB_H
#define RANDOMLIB_H

#include <random>
#include <chrono>
#include <stdexcept>
#include <type_traits>
#include <cstdint>
#include <vector>

// 所有随机数相关功能置于randomlib命名空间
namespace randomlib {
    // 纯标准库安全种子初始化：支持任意符合UniformRandomBitGenerator的生成器
    template <typename Gen>
    inline void seed_engine_safely(Gen& engine) {
        static_assert(std::is_default_constructible_v<Gen>, "Generator must support default construction!");
        static_assert(std::is_same_v<decltype(engine()), typename Gen::result_type>,
            "Generator must satisfy UniformRandomBitGenerator concept!");

        const auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
        const auto ns_count = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();

        std::vector<typename Gen::result_type> seed_data{
            static_cast<typename Gen::result_type>(ns_count & 0xFFFFFFFFFFFFFFFF),
            static_cast<typename Gen::result_type>(ns_count >> 64),
            static_cast<typename Gen::result_type>(std::chrono::system_clock::now().time_since_epoch().count())
        };

        std::seed_seq seed_seq(seed_data.begin(), seed_data.end());
        engine.seed(seed_seq);
    }

    // 匿名命名空间：确保引擎初始化函数仅当前编译单元可见
    namespace {
        // 初始化全局默认引擎（仅执行一次）
        std::mt19937 init_default_engine() {
            std::mt19937 eng;
            seed_engine_safely(eng);
            return eng;
        }
    }

    // 声明全局静态随机数生成器（默认类型std::mt19937）
    static std::mt19937 engine_ = init_default_engine();

    // 手动设置种子：支持自定义生成器（默认操作全局引擎）
    template <typename Gen = std::mt19937>
    inline void set_seed(uint64_t manual_seed, Gen& engine = engine_) {
        engine.seed(static_cast<typename Gen::result_type>(manual_seed));
    }

    // 重新执行安全种子初始化：默认操作全局引擎
    template <typename Gen = std::mt19937>
    inline void reseed(Gen& engine = engine_) {
        seed_engine_safely(engine);
    }

    // 生成指定范围整数（闭区间 [low, high]）
    // 生成器参数默认使用全局静态engine_
    template <typename IntT = int, typename Gen = std::mt19937>
    inline IntT get_int(IntT low, IntT high, Gen& engine = engine_) {
        static_assert(std::is_integral_v<IntT>, "get_int only supports integral types!");
        if (low > high) {
            throw std::invalid_argument("get_int: low must be <= high");
        }
        std::uniform_int_distribution<IntT> dist(low, high);
        return dist(engine);
    }

    // 生成指定范围浮点数（左闭右开 [low, high)）
    template <typename RealT = double, typename Gen = std::mt19937>
    inline RealT get_real(RealT low = 0.0, RealT high = 1.0, Gen& engine = engine_) {
        static_assert(std::is_floating_point_v<RealT>, "get_real only supports floating types!");
        if (low >= high) {
            throw std::invalid_argument("get_real: low must be < high");
        }
        std::uniform_real_distribution<RealT> dist(low, high);
        return dist(engine);
    }

    // 生成正态分布浮点数（默认标准正态分布）
    template <typename RealT = double, typename Gen = std::mt19937>
    inline RealT get_normal(RealT mean = 0.0, RealT stddev = 1.0, Gen& engine = engine_) {
        static_assert(std::is_floating_point_v<RealT>, "get_normal only supports floating types!");
        if (stddev < 0) {
            throw std::invalid_argument("get_normal: stddev must be non-negative");
        }
        static std::normal_distribution<RealT> dist;
        dist.param(typename std::normal_distribution<RealT>::param_type(mean, stddev));
        return dist(engine);
    }

    // 对数组进行随机打乱：默认使用全局引擎
    template<class U, typename Gen = std::mt19937>
    inline void shuffle(U* arr, size_t n, Gen& engine = engine_) {
        if (n == 0) return;
        if (arr == nullptr) {
            throw std::invalid_argument("shuffle: array pointer cannot be nullptr when n > 0");
        }
        std::shuffle(arr, arr + n, engine);
    }

} // namespace randomlib

#endif // RANDOMLIB_H