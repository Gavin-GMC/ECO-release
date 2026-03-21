//------------------------Description------------------------
// This file defines the solution decoder, which provides a
//  correspondence between the result vector and the variables.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <iostream>
#include <sstream>
#include <iomanip>
#include"variable.h"
namespace ECFC
{
	struct SolutionDecoder
	{
	private:
		int _variable_number;
		int _object_number;
	public:
		ElementNote* notes;
		int* variable_sizes;
		std::string* object_name;

		SolutionDecoder(int variable_number, int object_number)
		{
			_variable_number = variable_number;
			_object_number = object_number;
			notes = new ElementNote[variable_number];
			variable_sizes = new int[variable_number];
			object_name = new std::string[object_number];
		}

		~SolutionDecoder()
		{
			delete[] notes;
			delete[] variable_sizes;
			delete[] object_name;
		}

		int getVariableNumber()
		{
			return _variable_number;
		}

		int getObjectNumber()
		{
			return _object_number;
		}

		std::string toString(double* result, double* fitness, bool full_print = false)
		{
			std::ostringstream oss;

			if (full_print)
			{
				for (int j = 0; j < getObjectNumber(); j++)
				{
					oss << object_name[j] + ":\t";
					oss << std::to_string(fitness[j]) + "\t";
				}
				oss << "\n";

				int index = 0;
				// 逐变量打印
				for (int vid = 0; vid < getVariableNumber(); vid++)
				{
					oss << "v" + std::to_string(vid + 1) + "-\t" + notes[vid]._name + ":";
					for (int did = 0; did < variable_sizes[vid]; did++)
					{
						oss << "\t" << std::defaultfloat << std::setprecision(15) << result[index];
						index++;
					}
					oss << "\n";
				}
				oss << "EOS";
			}
			else
			{
				for (int j = 0; j < getObjectNumber(); j++)
				{
					oss << object_name[j] + ":\t";
					oss << std::to_string(fitness[j]) + "\t";
				}
			}

			return oss.str();
		}
	};
}