#include<time.h>

#include<randlib.hpp>

#include"ecflow-rand.h"


namespace ECFlow
{
	static std::mt19937 random_engine_;
	static unsigned int _global_seed;

	// 通过lambda触发初始化，程序启动时自动执行（主函数前）
	static bool __ecflow_random_inited = []() {
		set_seed(time(NULL));
		return true;
	}();

	void set_seed(unsigned int seed)
	{
		_global_seed = seed;
		randomlib::set_seed(seed, random_engine_);
	}

	double get_rand_real(double low, double high)
	{
		return randomlib::get_real(low, high, random_engine_);
	}

	double rand01()
	{
		return get_rand_real(0, 1);
	}

	int get_int(int low, int high)
	{
		return randomlib::get_int(low, high, random_engine_);
	}

	int wide_rand()
	{
		// 静态分布对象：仅构造一次，后续复用，避免重复开销
		// 取值范围 [0, INT_MAX]，恒非负（wide_rand() % N 被广泛用作数组下标，负值会越界）
		static std::uniform_int_distribution<int> dist(
			0,                                // 下界 0，保证非负（与原始 basicfunc.h 一致）
			std::numeric_limits<int>::max()   // int类型最大值（如2147483647）
		);

		// 调用全局生成器生成随机数
		return dist(random_engine_);
	}

	double get_normal(double mean, double stddev)
	{
		return randomlib::get_normal(mean, stddev, random_engine_);
	}

	unsigned int get_random_seed()
	{
		return _global_seed;
	}

	std::mt19937& get_random_engine()
	{
		return random_engine_;
	}
}