// SPDX-License-Identifier: BSD-3-Clause

#include <stdexcept>
#include<string>

#include<doublelib.hpp>
#include<distance_calculator.hpp>

#include"evoflow-math.h"

namespace EvoFlow
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

	// 地理坐标(度.分)→弧度,TSPLIB GEO 约定(用 TSPLIB 惯例常量 3.141592,与其参考最优一致)。
	static double geoToRad(double coord)
	{
		const double TSPLIB_PI = 3.141592;
		double deg = (double)(int)coord;            // 整数部分=度
		double min = coord - deg;                   // 小数部分=分(如 .47=47 分)
		return TSPLIB_PI * (deg + 5.0 * min / 3.0) / 180.0;
	}

	double geo_distance(double a[], double b[])
	{
		const double RRR = 6378.388;
		double lat_i = geoToRad(a[0]), long_i = geoToRad(a[1]);
		double lat_j = geoToRad(b[0]), long_j = geoToRad(b[1]);
		double q1 = std::cos(long_i - long_j);
		double q2 = std::cos(lat_i - lat_j);
		double q3 = std::cos(lat_i + lat_j);
		return (double)(int)(RRR * std::acos(0.5 * ((1.0 + q1) * q2 - (1.0 - q1) * q3)) + 1.0);   // TSPLIB 内建取整
	}

	double metricDistance(DistanceType type, double a[], double b[], size_t size)
	{
		switch (type)
		{
		case DistanceType::Manhattan: return man_distance(a, b, size);
		case DistanceType::Chebyshev: return che_distance(a, b, size);
		case DistanceType::Geo:       return geo_distance(a, b);
		default:                      return eu_distance(a, b, size);   // Euclidean
		}
	}

	double applyRound(RoundType type, double d)
	{
		switch (type)
		{
		case RoundType::Round: return std::round(d);
		case RoundType::Ceil:  return std::ceil(d);
		case RoundType::Floor: return std::floor(d);
		default:               return d;   // None
		}
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
