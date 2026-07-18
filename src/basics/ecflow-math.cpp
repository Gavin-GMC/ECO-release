#include <stdexcept>
#include<string>

#include<doublelib.hpp>
#include<distance_calculator.hpp>

#include"ecflow-math.h"

namespace ECFlow
{

	double eu_distance(double a[], double b[], size_t size)
	{
		return distancelib::euclidean<double>(a, b, size);
	}

	double man_distance(double a[], double b[], size_t size)
	{
		return distancelib::manhattan<double>(a, b, size);
	}

	double che_distance(double a[], double b[], size_t size)
	{
		return distancelib::chebyshev<double>(a, b, size);
	}

	double hamming_distance(double a[], double b[], size_t size, double accuracy)
	{
		return distancelib::hamming<double>(a, b, size);
	}

	double cos_similarity(double a[], double b[], size_t size)
	{
		return distancelib::cosine<double>(a, b, size);
	}

	bool equal(double a, double b, double accuracy)
	{
		return doublelib::isEqual(a, b, accuracy);
	}

	bool large(double a, double b, double accuracy)
	{
		return doublelib::isGreater(a, b, accuracy);
	}

	bool less(double a, double b, double accuracy)
	{
		return doublelib::isLess(a, b, accuracy);
	}

	bool notlarge(double a, double b, double accuracy)
	{
		return doublelib::isLessOrEqual(a, b, accuracy);
	}

	bool notless(double a, double b, double accuracy)
	{
		return doublelib::isGreaterOrEqual(a, b, accuracy);
	}

	int intelliTrunc(double value)
	{
		double truncated_val = doublelib::trunc(value);

		// 校验截断后的值是否在int的取值范围内（避免溢出导致未定义行为）
		const int int_min = std::numeric_limits<int>::min();
		const int int_max = std::numeric_limits<int>::max();

		if (truncated_val < static_cast<double>(int_min) || truncated_val > static_cast<double>(int_max)) {
			throw std::out_of_range(
				"intelliTrunc: truncated value (" + std::to_string(truncated_val) +
				") out of int range [" + std::to_string(int_min) + ", " + std::to_string(int_max) + "]"
			);
		}

		return static_cast<int>(truncated_val);
	}
}