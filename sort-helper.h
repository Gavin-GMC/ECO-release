//------------------------Description------------------------
// This file provide the provides support for sorting, 
// including a sorting template function and the class 
// "sortHelper", which provides a container for keeping
//  object ids and values bound while sorting.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once

namespace ECFC
{
	//sort the list between given address
	template<class T>
	void sort(T* left, T* right, bool is_ascending = true)
	{
		T* p_buffer = (T*)malloc(sizeof(T));
		if (!p_buffer)
			return;

		size_t ssize = right - left;

		//insert sort
		for (int i = 0; i < ssize; i++)
		{
			for (int j = 0; j < i; j++)
			{
				//insert individual
				if (left[i] < left[j])
				{
					*p_buffer = left[i];
					for (int j1 = i; j1 > j; j1--)
						left[j1] = left[j1 - 1];
					left[j] = *p_buffer;
					break;
				}
			}
		}

		free(p_buffer);
	}

	// compare the objectives the pointer point to
	template<class T_pointer>
	bool pointerLess(T_pointer o1, T_pointer o2)
	{
		return *o1 < *o2;
	}

	// compare the objectives the pointer point to
	template<class T_pointer>
	bool pointerLarger(T_pointer o1, T_pointer o2)
	{
		return *o1 > *o2;
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

	// a structure that helps sort the associated variables based on its pointer
	template<class idT, class vlT>
	struct sortPointerHelper {
		idT id;
		vlT* value;

		sortPointerHelper() {}

		sortPointerHelper(idT id, vlT* value)
		{
			this->id = id;
			this->value = value;
		}

		bool operator<(const sortPointerHelper& a)const
		{
			return *value < *a.value;
		}

		bool operator>(const sortPointerHelper& a)const
		{
			return *value > *a.value;
		}

	};
}