// SPDX-License-Identifier: BSD-3-Clause

#include<time.h>
#include<cmath>   // v2.5.4.1:无状态 Box-Muller(sqrt/log/cos)

#include<randlib.hpp>

#include"evoflow-rand.h"


namespace EvoFlow
{
	static std::mt19937 random_engine_;   // 全局默认引擎(子群上下文外使用)
	static unsigned int _global_seed;

	// 本线程"当前活跃引擎"指针:nullptr → 用全局默认引擎 random_engine_。
	// thread_local:并行时每线程各一份,EngineScope 只影响本线程,互不干扰。
	static thread_local std::mt19937* current_engine_ = nullptr;

	// 取当前该用的引擎:有 current_engine_ 用它,否则用全局默认。
	static inline std::mt19937& active_engine()
	{
		return current_engine_ ? *current_engine_ : random_engine_;
	}

	void set_current_engine(std::mt19937* e) { current_engine_ = e; }
	std::mt19937* get_current_engine() { return current_engine_; }

	void seed_engine(std::mt19937& engine, unsigned int seed)
	{
		randomlib::set_seed(seed, engine);
	}

	// 通过lambda触发初始化，程序启动时自动执行（主函数前）
	static bool __evoflow_random_inited = []() {
		set_seed(time(NULL));
		return true;
	}();

	void set_seed(unsigned int seed)
	{
		// 作用于全局默认引擎:程序入口一次性播基种子。
		// per-子群引擎由 SubpopulationManager 用 derive_subpop_seed 单独播,不走这里。
		_global_seed = seed;
		randomlib::set_seed(seed, random_engine_);
	}

	double get_rand_real(double low, double high)
	{
		return randomlib::get_real(low, high, active_engine());
	}

	double rand01()
	{
		return get_rand_real(0, 1);
	}

	int get_int(int low, int high)
	{
		return randomlib::get_int(low, high, active_engine());
	}

	int wide_rand()
	{
		// v2.5.4.1:局部分布对象(去 static)——`uniform_int_distribution` 无跨调用状态,
		//   局部构造与静态复用结果逐位一致,但消除并行下对同一 static 对象并发 operator() 的隐患。
		// 取值范围 [0, INT_MAX]，恒非负（wide_rand() % N 被广泛用作数组下标，负值会越界）
		std::uniform_int_distribution<int> dist(
			0,                                // 下界 0，保证非负（与原始 basicfunc.h 一致）
			std::numeric_limits<int>::max()   // int类型最大值（如2147483647）
		);

		// 调用当前活跃引擎生成随机数
		return dist(active_engine());
	}

	double get_normal(double mean, double stddev)
	{
		// v2.5.4.1:无状态 Box-Muller —— 由两个均匀数直接算一个标准正态,只依赖当前活跃引擎、零缓存/零静态。
		//   替换 randlib 的 static std::normal_distribution(其 Box-Muller 配对缓存跨引擎/子群/线程共享 →
		//   泄漏 + 并行竞态)。因 get_rand_real/rand01_ 已走 active_engine() → 天然 per-子群独立 + 线程安全 + 串/并一致。
		//   u1 用 rand01_() 开区间 (0,1) 保 ln(u1) 有限;丢弃 Box-Muller 的第二个值(cos 支)。
		const double two_pi = 6.283185307179586476925286766559;
		double u1 = rand01_();
		double u2 = rand01();
		double z = std::sqrt(-2.0 * std::log(u1)) * std::cos(two_pi * u2);
		return mean + stddev * z;
	}

	unsigned int get_random_seed()
	{
		return _global_seed;
	}

	std::mt19937& get_random_engine()
	{
		// 返回当前活跃引擎:使 shuffle() 等直接取引擎的调用也遵循 per-子群切换。
		return active_engine();
	}
}
