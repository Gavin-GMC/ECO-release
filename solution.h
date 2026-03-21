//------------------------Description------------------------
// This file defines the solution object. It contains two vectors,
// namely the result vector and the fitness vector.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include<algorithm>
#include<cstring>
#include"solution-decoder.h"

namespace ECFC
{
	class Solution
	{
	protected:
		int _size;
		int _object_number;

	public:
		SolutionDecoder* decoder_pointer;
		double* result;
		double* fitness;

		Solution()
		{
			_size = 0;
			_object_number = 0;
			decoder_pointer = nullptr;
			result = nullptr;
			fitness = nullptr;
		}

		~Solution()
		{
			delete[] result;
			delete[] fitness;
		}

		int getSolutionSize()
		{
			return _size;
		}

		int getObjectNumber()
		{
			return _object_number;
		}

		void setSize(int size, int object_number)
		{
			if (_size != size)
			{
				delete[] result;
				_size = size;
				result = new double[_size];
			}
			if (_object_number != object_number)
			{
				delete[] fitness;
				_object_number = object_number;
				fitness = new double[_object_number];
			}
		}

		void setSize(const Solution& source)
		{
			setSize(source._size, source._object_number);
		}

		void setDecoder(SolutionDecoder* pointer)
		{
			decoder_pointer = pointer;
		}

		void setDecoder(const Solution& source)
		{
			setDecoder(source.decoder_pointer);
		}

		void copy(const Solution& copy_source)
		{
			setSize(copy_source._size, copy_source._object_number);
			//对解决方案和适应度值进行深拷贝
			memcpy(result, copy_source.result, _size * sizeof(double));
			memcpy(fitness, copy_source.fitness, _object_number * sizeof(double));
			decoder_pointer = copy_source.decoder_pointer;
		}

		void copy(const double* source_result, const double* source_fitness)
		{
			//对解决方案和适应度值进行深拷贝
			memcpy(result, source_result, _size * sizeof(double));
			memcpy(fitness, source_fitness, _object_number * sizeof(double));
		}

		void shallowCopy(const Solution& copy_source)
		{
			_size = copy_source._size;
			_object_number = copy_source._object_number;
			result = copy_source.result;
			fitness = copy_source.fitness;
			decoder_pointer = copy_source.decoder_pointer;
		}

		void shallowClear()
		{
			_size = 0;
			_object_number = 0;
			result = nullptr;
			fitness = nullptr;
			decoder_pointer = nullptr;
		}

		void swap(Solution& copy_source)
		{
			std::swap(_size, copy_source._size);
			std::swap(_object_number, copy_source._object_number);
			std::swap(result, copy_source.result);
			std::swap(fitness, copy_source.fitness);
			std::swap(decoder_pointer, copy_source.decoder_pointer);
		}

		double& operator[](const int index)
		{
			return result[index];
		}

		bool operator==(const Solution& a)const
		{
			return memcmp(this->result, a.result, _size * sizeof(double)) == 0;
		}
	};
}
