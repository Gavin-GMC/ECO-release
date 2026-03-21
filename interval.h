//------------------------Description------------------------
// This file defines the feasible intervel and region of elements.
// The computation and reduction of intervals are also included.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include"basicfunc.h"
#include"ecfc-constant.h"
#include"variable.h"
#include<algorithm>

namespace ECFC
{
	// 区间对象，以单向链表形式构建域
	class Interval
	{
	protected:
		int _length;
		double _accuracy;
		double _begin;
		double _end;

	public:
		Interval* next;

		Interval(double begin, double end, double accuracy, bool left_open, bool right_open)
		{
			_accuracy = accuracy;

			_begin = begin;
			if (left_open)
				_begin += accuracy;

			// 这里预设了右边界与左边界的距离是精度的整数倍，因此需要对右边界进行对齐
			_length = intelliTrunc((end - _begin) / accuracy);
			_end = _begin + _length * _accuracy;
			if (right_open && equal(_end, end))
			{
				_end -= accuracy;
			}
			else
			{
				_length += 1;
			}		

			next = nullptr;
		}

		Interval(Interval& source)
		{
			_length = source._length;
			_accuracy = source._accuracy;
			_begin = source._begin;
			_end = source._end;
			next = nullptr;
		}

		~Interval()
		{
		}

		double discretized(double value)
		{
			int length = intelliTrunc((value - _begin) / _accuracy);
			return _begin + length * _accuracy;
		}

		// 将区间中的位置转化为实际值
		double dereference(int distance)//不对外提供接口，预设距离合法
		{
#if defined(DEBUGMODE) || defined(FEASIBLEDEBUG)
			sys_logger.debug("Interval:\tResult:\t" + std::to_string(_begin + distance * _accuracy) + "\tAccuracy:\t" + std::to_string(_accuracy) + "\tDistance:\t" + std::to_string(distance) + "\tBegin:\t" + std::to_string(_begin));
#endif
			return _begin + distance * _accuracy;
		}

		double getAccuracy()
		{
			return _accuracy;
		}

		double getLeftBoundary()
		{
			return _begin;
		}

		double getRightBoundary()
		{
			return _end;
		}

		// 获得所有可行值的列表(不对外提供接口，预设数组长度是足够的)
		void getFeasibleList(double* list_buffer)
		{
			list_buffer[0] = _begin;
			for (int i = 1; i < _length; i++)
				list_buffer[i] = list_buffer[i - 1] + _accuracy;
		}

		// 获得所有可行值的列表(不对外提供接口，预设数组长度是足够的)
		void getFeasibleList(double* list_buffer, int size)
		{
			list_buffer[0] = _begin;
			size = std::min(size, _length);
			for (int i = 1; i < size; i++)
				list_buffer[i] = list_buffer[i - 1] + _accuracy;
		}

		int getLength()
		{
			return _length;
		}

		// 将区间的左侧裁去一部分
		int reductionLeft(double boundary, bool is_open)
		{
			if (boundary < _begin)
				return 0;

			int back = intelliTrunc((boundary - _begin) / _accuracy);//返回值，缩减长度

			_begin += (back * _accuracy);

			if (_begin < boundary || is_open)
			{
				back += 1;
				_begin += _accuracy;
			}

			_length -= back;

			return back;
		}

		// 将区间的右侧裁去一部分
		int reductionRight(double boundary, bool is_open)
		{
			if (boundary > _end)
				return 0;

			int back = intelliTrunc((_end - boundary) / _accuracy);

			_end -= (back * _accuracy);

			if (_end > boundary || is_open)
			{
				back += 1;
				_end -= _accuracy;
			}

			_length -= back;

			return back;
		}

		// 将区间分割为相连的两个
		void split(double cut_position, bool belong_front) // 需要对切割点的位置进行检查，确保是合适的位置
		{
			if (less(cut_position, _begin) || large(cut_position, _end))
				return;

			//分割区间，切割点一闭一开避免重复值
			Interval* buffer;
			if (belong_front)
			{
				if (equal(cut_position, _end))
					return;

				// 获得切割点右侧最近点位
				cut_position = _begin + ceil((cut_position - EQACCURACY - _begin) / _accuracy) * _accuracy;

				// 分割
				buffer = new Interval(cut_position, _end, _accuracy, true, false);
				reductionRight(cut_position, false);
			}
			else
			{
				if (equal(cut_position, _begin))
					return;

				// 获得切割点左侧最近点位
				cut_position = _begin + floor((cut_position + EQACCURACY - _begin) / _accuracy) * _accuracy;

				// 分割
				buffer = new Interval(cut_position, _end, _accuracy, false, false);
				reductionRight(cut_position, true);
			}

			//插入链表
			buffer->next = next;
			next = buffer;
		}

		// 判断目标值是否在当前区间中
		bool isIn(double value)
		{
			return notless(value, _begin) && notlarge(value, _end);
		}
	};
}
