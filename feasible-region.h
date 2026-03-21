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
#include<algorithm>
#include"ecfc-constant.h"
#include"variable.h"
#include"interval.h"

namespace ECFC
{
	// 每个变量的可行域构成：
	// 初始可行域：由变量取值范围决定
	// 共用可行域：适用于应用于所有维度的约束（独一约束等）
	// 维度可行域：先拷贝共用可行域再应用针对维度的约束（容量约束，最小距约束等）

	// 由多个可行区间构成的完整可行域
	class FeasibleLine
	{
	private:
		Interval* _root; // 根节点，无实际意义和区间指示
		int _length;

		// 根据位置返回可行值
		double dereference(int position)
		{
			Interval* iter = _root->next;
			while (iter != nullptr)
			{
				if (position < iter->getLength())
					return iter->dereference(position);
				position -= iter->getLength();
				iter = iter->next;
			}

			return EMPTYVALUE;
		}

		void copy(Interval* intervals)
		{
			deleteSubsequentInterval(_root);

			if (intervals == nullptr)
				return;

			_root->next = new Interval(*intervals);

			Interval* iter = _root->next;
			_length = iter->getLength();

			while (intervals->next != nullptr)
			{
				intervals = intervals->next;
				iter->next = new Interval(*intervals);
				iter = iter->next;
				_length += iter->getLength();
			}
		}

		// 删去对象的下一个区间
		void deleteNextInterval(Interval* interval)
		{
			Interval* delete_buffer;
			delete_buffer = interval->next;
			interval->next = interval->next->next;
			_length -= delete_buffer->getLength();

			delete delete_buffer;
		}

		// 删去对象的后续区间
		void deleteSubsequentInterval(Interval* root)
		{
			if (root == nullptr || root->next == nullptr)
				return;

			Interval* delete_iter = root->next;
			root->next = nullptr;
			root = delete_iter;

			while (delete_iter != nullptr)
			{
				root = root->next;
				_length -= delete_iter->getLength();
				delete delete_iter;
				delete_iter = root;
			}
		}

		// 在目标区间后插入区间
		void insertInterval(Interval* interval, Interval* insert_interval)
		{
			insert_interval->next = interval->next;
			interval->next = insert_interval;
			_length += insert_interval->getLength();
		}

		// 移除区间（截止于右边界）
		Interval* regionReductionOut(Interval* root, double left, double right, bool left_open, bool right_open)
		{
			//参数不可以改为区间，因为会涉及到对齐和精度的问题
			Interval* iter = root;

			// 定位左边界至当前区间与后区间的中间或者当前区间的中间（通过切割实现定位至两区间之间）
			if (left_open)
			{
				while (iter->next != nullptr && notless(left, iter->next->getLeftBoundary()))
				{
					iter = iter->next;
				}
				if (less(left, iter->getRightBoundary()))
				{
					// 如果是开区间，则切割点不被移除，归前区间而被保留
					iter->split(left, true);
				}
			}
			else
			{
				while (iter->next != nullptr && large(left, iter->next->getLeftBoundary()))
				{
					iter = iter->next;
				}
				if(notlarge(left, iter->getRightBoundary()))
				{
					// 如果是闭区间，则切割点被移除，归后区间
					iter->split(left, false);
				}
			}

			// 定位右边界，并移除其间不可行区间
			if (right_open)
			{
				while (iter->next != nullptr && large(right, iter->next->getRightBoundary()))
				{
					deleteNextInterval(iter);
				}

				if (iter->next != nullptr && large(right, iter->next->getLeftBoundary()))
				{
					// 如果是开区间，则切割点不被移除，归后区间而被保留
					iter->next->split(right, false);
					deleteNextInterval(iter);
				}
			}
			else
			{
				while (iter->next != nullptr && notless(right, iter->next->getRightBoundary()))
				{
					deleteNextInterval(iter);
				}

				if (iter->next != nullptr && notless(right, iter->next->getLeftBoundary()))
				{
					// 如果是闭区间，则切割点被移除，归前区间
					iter->next->split(right, true);
					deleteNextInterval(iter);
				}
			}

			return iter;
		}

		// 交集域（截止于右边界）
		Interval* regionReductionTo(Interval* root, double left, double right, bool left_open, bool right_open)
		{
			Interval* iter = root;

			// 定位左边界
			if (left_open)
			{
				while (iter->next != nullptr && notless(left, iter->next->getRightBoundary()))
				{
					deleteNextInterval(iter);
				}

				if (iter->next == nullptr)
				{
					return iter;
				}

				if (notless(left, iter->getLeftBoundary()))
				{
					// 如果是开区间，则切割点被移除，归前区间
					iter->next->split(left, true);

					deleteNextInterval(iter);
				}
			}
			else
			{
				while (iter->next != nullptr && large(left, iter->next->getRightBoundary()))
				{
					deleteNextInterval(iter);
				}

				if (iter->next == nullptr)
				{
					return iter;
				}
				if (large(left, iter->next->getLeftBoundary()))
				{
					// 如果是闭区间，则切割点被保留，归后区间
					iter->next->split(left, false);

					deleteNextInterval(iter);
				}
			}

			// 定位右边界
			if (right_open)
			{
				while (iter->next != nullptr && less(iter->next->getLeftBoundary(), right))
				{
					iter = iter->next;
				}
				// if (right <= iter->getRightBoundary())
				if (notless(right, iter->getRightBoundary()))
				{
					// 如果是开区间则切割点归后区间，从而被移除
					iter->split(right, false);
				}
			}
			else
			{
				while (iter->next != nullptr && notlarge(iter->next->getLeftBoundary(), right))
				{
					iter = iter->next;
				}
				// if (right < iter->getRightBoundary())
				if (less(right, iter->getRightBoundary()))
				{
					// 如果是闭区间则切割点归前区间，从而被保留
					iter->split(right, true);
				}
				deleteSubsequentInterval(iter->next);
			}

			return iter;
		}

		//合并域（截止于右边界）
		Interval* regionConnectWith(Interval* root, double left, double right, bool left_open, bool right_open, double accuary)
		{
			Interval* iter = root;

			// 定位左边界
			if (left_open)
			{
				while (iter->next != nullptr && notless(left, iter->next->getLeftBoundary()))
				{
					iter = iter->next;
				}
				if (less(left, iter->getRightBoundary()))
				{
					left = iter->getRightBoundary();
				}
				else
				{
					// 将左边界调整至合适的位置（只缩不放）
					double left_buffer;
					if (iter->getRightBoundary() != ECFC_MIN) // 插入位置不为根节点后
					{
						left_buffer = iter->getRightBoundary() + ceil((left - iter->getRightBoundary()) / accuary) * accuary;

						if (left_buffer != left) // 区间边界已经缩小了，需要改为闭区间
						{
							left = left_buffer;
							left_open = false;
						}
					}
					else // 插入位置为根节点后，需要参考后续区间
					{
						if (iter->next != nullptr) // 有后续区间
						{
							left_buffer = iter->next->getLeftBoundary() - floor((iter->next->getLeftBoundary() - left) / accuary) * accuary;

							if (left_buffer != left) // 区间边界已经缩小了，需要改为闭区间
							{
								left = left_buffer;
								left_open = false;
							}
						}
					}
				}
			}
			else
			{
				// while (iter->next != nullptr && left > iter->next->getLeftBoundary())
				while (iter->next != nullptr && large(left, iter->next->getLeftBoundary()))
				{
					iter = iter->next;
				}
				// if (left <= iter->getRightBoundary())
				if (notlarge(left, iter->getRightBoundary()))
				{
					// 将左边界调整至合适的位置（只缩不放）
					left = iter->getRightBoundary();

					// 需要调整到开区间，不然会有重复
					left_open = true;
				}
				else
				{
					// 将左边界调整至合适的位置（只缩不放）
					if (iter->getRightBoundary() != ECFC_MIN) // 插入位置不为根节点后
					{
						left = iter->getRightBoundary() + ceil((left - iter->getRightBoundary()) / accuary) * accuary;
					}
					else
					{
						if (iter->next != nullptr) // 有后续区间
						{
							left = iter->next->getLeftBoundary() - floor((iter->next->getLeftBoundary() - left) / accuary) * accuary;
						}
					}
				}
			}

			if (left > right || (left == right && (left_open || right_open))) // 该区间已被现区间包含
			{
				return iter;
			}

			// 插入区间
			insertInterval(iter, new Interval(left, right, accuary, left_open, right_open));
			iter = iter->next;

			// 定位右边界，只有闭的情况，并移除其间重复区间
			// while (iter->next != nullptr && iter->getRightBoundary() >= iter->next->getRightBoundary())
			while (iter->next != nullptr && notless(iter->getRightBoundary(), iter->next->getRightBoundary()))
			{
				deleteNextInterval(iter);
			}

			// if (iter->next != nullptr && iter->getRightBoundary() >= iter->next->getLeftBoundary())
			if (iter->next != nullptr && notless(iter->getRightBoundary(), iter->next->getLeftBoundary()))
			{
				// 如果是闭区间，则切割点移除，归前区间
				iter->next->split(iter->getRightBoundary(), true);
				deleteNextInterval(iter);
			}

			return iter;
		}

	public:
		FeasibleLine()
		{
			_root = new Interval(ECFC_MIN, ECFC_MIN, 1, false, true);

			_length = 0;
		}

		FeasibleLine(const double* source, const int size)
		{
			_root = new Interval(ECFC_MIN, ECFC_MIN, 1, false, true);
			Interval* iter = _root;

			double* buffer = new double[size];
			memcpy(buffer, source, size * sizeof(double));
			std::sort(buffer, buffer + size);// 升序排序

			// 插入值
			for (int i = 0; i < size; i++)
			{
				// 去重！！！！
				iter->next = new Interval(buffer[i], buffer[i], 1, false, false);
				iter = iter->next;
			}
			_length = size;

			delete[] buffer;
		}

		FeasibleLine(FeasibleLine& source)
		{
			copy(source._root);
		}

		FeasibleLine(ElementNote* variable_note)
		{
			_root = new Interval(ECFC_MIN, ECFC_MIN, 1, false, true);
			_root->next = new Interval(variable_note->_lowbound, variable_note->_upbound, variable_note->_accuracy, false, false);
			_length = _root->next->getLength();
		}

		~FeasibleLine()
		{
			deleteSubsequentInterval(_root);
			delete _root;
		}

		double getDiscretized(double value)
		{
			Interval* iter = _root->next;
			while (iter != nullptr)
			{
				if (iter->isIn(value))
					return iter->discretized(value);
				iter = iter->next;
			}
			return EMPTYVALUE;
		}

		// 获取随机可行值
		double getRandomDecision()
		{
			if (_length == 0)
				return ECFC::EMPTYVALUE;
			return dereference(wide_rand() % _length);
		}

		// 获取边界值
		double getBoundaryDecision(bool get_left = true)
		{
			// 域为空
			if (_length == 0)
			{
				return EMPTYVALUE;
			}

			if (get_left)
				return _root->next->getLeftBoundary();

			Interval* iter = _root->next;
			while (iter->next != nullptr)
				iter = iter->next;
			return iter->getRightBoundary();
		}

		// 获取目标值的最邻近可行值
		double getClosestFeasibleDecision(double value)
		{
			//定位离其最近的两个区间
			Interval* iter = _root;
			while (iter->next != nullptr && value >= iter->next->getLeftBoundary())
			{
				iter = iter->next;
			}

			//本身是可行解
			if (value <= iter->getRightBoundary())
				return value;

			//无后续区间
			if (iter->next == nullptr)
				return iter->getRightBoundary();

			//计算距离
			double distance = value - iter->getRightBoundary();
			if (distance <= (iter->next->getLeftBoundary() - value))
				return iter->getRightBoundary();
			else
				return iter->next->getLeftBoundary();
		}

		// 获取所有可行值构成的列表
		double* getFeasibleList(int& list_size)
		{
			list_size = _length;

			double* back = new double[_length];
			Interval* iter = _root;

			int offset = 0;
			while (iter != nullptr)
			{
				// 获取区间段的可行值
				iter->getFeasibleList(back + offset);
				// 将容器空白指针进行偏置
				offset += iter->getLength();
				iter = iter->next;
			}

			return back;
		}

		void getFeasibleList(double*& list_buffer, int& list_size)
		{
			if (list_size == EMPTYVALUE)
			{
				list_buffer = getFeasibleList(list_size);
			}
			else
			{
				Interval* iter = _root;

				int offset = 0;
				int left_size = list_size;
				while (iter != nullptr && left_size > 0)
				{
					// 获取区间段的可行值
					iter->getFeasibleList(list_buffer + offset, left_size);
					// 将容器空白指针进行偏置
					offset += iter->getLength();
					left_size -= iter->getLength();
					iter = iter->next;
				}

				if (left_size < 0)
					left_size = 0;
				list_size -= left_size;
			}
		}

		int getLength()
		{
			return _length;
		}

		// 并集
		void merge(double left, double right, bool left_open, bool right_open, double accuary)
		{
			regionConnectWith(_root, left, right, left_open, right_open, accuary);
		}

		// 并集
		void merge(Interval* target)
		{
			regionConnectWith(_root, target->getLeftBoundary(), target->getRightBoundary(), false, false, target->getAccuracy());
		}

		// 并集
		void merge(FeasibleLine* target)
		{
			Interval* iter = target->_root->next;
			Interval* root = _root;
			while (iter != nullptr)
			{
				root = regionConnectWith(root, iter->getLeftBoundary(), iter->getRightBoundary(), false, false, iter->getAccuracy());
				iter = iter->next;
			}
		}

		// 交集
		void unite(double left, double right, bool left_open, bool right_open)
		{
			regionReductionTo(_root, left, right, left_open, right_open);
			// Interval* root = regionReductionTo(_root, left, right, left_open, right_open);
			// deleteSubsequentInterval(root);
		}

		// 交集
		void unite(Interval* target)
		{
			Interval* root = regionReductionTo(_root, target->getLeftBoundary(), target->getRightBoundary(), false, false);
			deleteSubsequentInterval(root);
		}

		// 交集
		void unite(FeasibleLine* target)
		{
			Interval* iter = target->_root->next;
			Interval* root = _root;
			while (iter != nullptr)
			{
				root = regionReductionTo(root, iter->getLeftBoundary(), iter->getRightBoundary(), false, false);
				iter = iter->next;
			}

			deleteSubsequentInterval(root);
		}

		// 差集
		void differ(double left, double right, bool left_open, bool right_open)
		{
			regionReductionOut(_root, left, right, left_open, right_open);
		}

		// 差集
		void differ(Interval* target)
		{
			regionReductionOut(_root, target->getLeftBoundary(), target->getRightBoundary(), false, false);
		}

		// 差集
		void differ(FeasibleLine* target)
		{
			Interval* iter = target->_root->next;
			Interval* root = _root;
			while (iter != nullptr)
			{
				root = regionReductionOut(root, iter->getLeftBoundary(), iter->getRightBoundary(), false, false);
				iter = iter->next;
			}
		}

		void copy(FeasibleLine* source)
		{
			copy(source->_root->next);
		}

		bool isEmpty()
		{
			return _length == 0;
		}

		bool isin(double value)
		{
			Interval* iter = _root->next;
			while (iter != nullptr)
			{
				if (iter->isIn(value))
					return true;

				iter = iter->next;
			}

			return false;
		}

		void clear()
		{
			deleteSubsequentInterval(_root);
		}
	};
}
