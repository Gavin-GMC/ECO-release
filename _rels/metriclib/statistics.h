#pragma once
#include<cmath>
#include <unordered_map>
namespace MetricLib
{
	// 计算均值
	double mean(double* data, int length) {
		double sum = 0.0;
		for (int i = 0; i < length; ++i) {
			sum += data[i];
		}
		return sum / length;
	}

	// 计算方差
	double variance(double* data, int length) {
		double meanValue = mean(data, length);
		double sum = 0.0;
		for (int i = 0; i < length; ++i) {
			sum += (data[i] - meanValue) * (data[i] - meanValue);
		}
		return sum / length;
	}

	// 计算标准差
	double standardDeviation(double* data, int length) {
		return sqrt(variance(data, length));
	}

	// 计算中位数
	double median(double* data, int length) {
		std::vector<double> sortedData(data, data + length);
		std::sort(sortedData.begin(), sortedData.end());
		if (length % 2 == 0) {
			return (sortedData[length / 2 - 1] + sortedData[length / 2]) / 2.0;
		}
		else {
			return sortedData[length / 2];
		}
	}

	// 计算众数
	double mode(double* data, int length) {
		std::unordered_map<double, int> countMap;
		for (int i = 0; i < length; ++i) {
			countMap[data[i]]++;
		}

		double modeValue = std::numeric_limits<double>::quiet_NaN();
		int maxCount = 0;
		for (const auto& pair : countMap) {
			if (pair.second > maxCount) {
				maxCount = pair.second;
				modeValue = pair.first;
			}
		}

		return modeValue;
	}

	double largest(double* input, int size)
	{
		double back;
		back = input[0];

		for (int i = 1; i < size; i++)
		{
			if (input[i] > back)
			{
				back = input[i];
			}
		}

		return back;
	}

	double smallest(double* input, int size)
	{
		double back;
		back = input[0];

		for (int i = 1; i < size; i++)
		{
			if (input[i] < back)
			{
				back = input[i];
			}
		}

		return back;
	}

	// 计算平均排名
	double* rank_statistic(double** input, int size, bool min_is_better = true)
	{
		int total_size = 0;
		for (int i = 0; i < size; i++)
			total_size += input[i][0];

		// 插入数据
		sortHelper<int, double>* sortbuffer = new sortHelper<int, double>[total_size];
		int buffer_index = 0;
		for (int i = 0; i < size; i++)
			for (int j = 0; j < input[i][0]; j++)
				sortbuffer[buffer_index++] = sortHelper<int, double>(i, input[i][j + 1]);

		// 最小最大转化
		if (!min_is_better) 
			for (int i = 0; i < total_size; i++)
				sortbuffer[i].value *= -1;

		std::sort(sortbuffer, sortbuffer + total_size);

		double* back = new double[size + 1];
		for (int i = 0; i < size; i++)
			back[i] = 0;
		for (int i = 0; i < total_size; i++)
			back[sortbuffer[i].id] += i;
		for (int i = 0; i < size; i++)
			back[i] /= input[i][0];

		delete[] sortbuffer;

		return back;
	}
}