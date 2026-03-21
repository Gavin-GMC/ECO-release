#pragma once

namespace MetricLib
{
	//return a value between 0-1
	double rand01()
	{
		return double(rand()) / RAND_MAX;
	}

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

	// a structure that helps sort the associated variables
	template<class idT, class vlT>
	struct sortHelper {
		idT id;
		vlT value;

		sortHelper() {}

		sortHelper(idT id, vlT value)
		{
			this->id = id;
			this->value = value;
		}

		bool operator<(const sortHelper& a)const
		{
			return value < a.value;
		}

		bool operator>(const sortHelper& a)const
		{
			return value > a.value;
		}

	};
}