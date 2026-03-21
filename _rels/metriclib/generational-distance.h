#pragma once
#include<cmath>
#include"funcs.h"

namespace MetricLib
{
	double gd(double* input, double* refer_point, int size, int refer_size, int object_number)
	{
		double back = 0;

		double distance_min;
		double distance_buffer;
		for (int i = 0; i < size; i++)
		{
			distance_min = eu_distance(input + i * object_number, refer_point, object_number);
			for (int j = 1; j < refer_size; j++)
			{
				distance_buffer = eu_distance(input + i * object_number, refer_point + j * object_number, object_number);
				if (distance_buffer < distance_min)
				{
					distance_buffer = distance_min;
				}
			}
			back += distance_min;
		}
		back /= size;

		return back;
	}

	double igd(double* input, double* refer_point, int size, int refer_size, int object_number)
	{
		double back = 0;

		double distance_min;
		double distance_buffer;
		for (int i = 0; i < refer_size; i++)
		{
			distance_min = eu_distance(refer_point + i * object_number,input , object_number);
			for (int j = 1; j < size; j++)
			{
				distance_buffer = eu_distance(refer_point + i * object_number, input + j * object_number, object_number);
				if (distance_buffer < distance_min)
				{
					distance_buffer = distance_min;
				}
			}
			back += distance_min;
		}
		back /= refer_size;;

		return back;
	}
}