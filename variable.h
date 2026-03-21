//------------------------Description------------------------
// This file defines the element object in ECFC,
//  which consists of two parts: value and note 
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include<string>
#include"ecfc-constant.h"
#include"logger.h"

namespace ECFC
{

	// 对于离散优化问题，可大致分为三类，分配优化问题，序列优化问题和子集优化问题
	// 对于分配优化问题【如背包问题】，其选择取决于当前的维度
	// 对于序列优化问题【如旅行商问题】，其选择取决于之前选择
	// 对于子集(聚类)优化问题【如图着色问题】，上述两种情况都有可能出现
	//
	enum class VariableType
	{
		normal,
		allocation,
		sequence_direction,
		sequence_bidiagraph,
		sub_module
	};

	// 元素值对象，包含对象地址和形状信息
	class ECVariable
	{
	private:
		int _length;
		int size_h;
		int size_b;
	public:
		double* address;

		ECVariable()
		{
			address = nullptr;
			size_h = 0;
			size_b = 0;
			_length = 0;
		}

		ECVariable(int length, int height)
		{
			address = nullptr;
			size_h = height;
			size_b = length;
			_length = size_b * size_h;
		}

		~ECVariable()
		{

		}

		void setSize(int length, int height)
		{
			size_h = height;
			size_b = length;
			_length = size_b * size_h;
		}

		int getLength()
		{
			return _length;
		}

		int getHeight()
		{
			return size_h;
		}

		int getWidth()
		{
			return size_b;
		}

		void setAddress(double* pointer)
		{
			address = pointer;
		}

		void copy(const ECVariable& source)
		{
			address = source.address;
			size_h = source.size_h;
			size_b = source.size_b;
			_length = source._length;
		}

		double& operator[](const int index)
		{
			return address[index];
		}
	};

	// 元素注释对象，包含上下界，精确度等信息
	struct ElementNote
	{
	public:
		std::string _name = "none";
		double _upbound = EMPTYVALUE;
		double _lowbound = EMPTYVALUE;
		double _accuracy = 0;
		VariableType _type = VariableType::normal;
	};

	// ECFC中元素对象，用于储存并记录问题和优化过程中涉及的各种量
	class ECElement
	{
	private:
		//ElementNote _note;

	public:
		ElementNote _note;

		ECVariable variable;

		ECElement()
		{

		}

		ECElement(std::string name, int length, int height, double upbound, double lowbound, double accuracy, VariableType type = VariableType::normal)
		{
			_note._name = name;
			_note._upbound = upbound;
			_note._lowbound = lowbound;
			_note._accuracy = accuracy;
			_note._type = type;
			variable.setSize(length, height);
		}

		~ECElement()
		{

		}

		std::string getName()
		{
			return _note._name;
		}

		int getLength()
		{
			return variable.getLength();
		}

		double getUpbound()
		{
			return _note._upbound;
		}

		double getLowbound()
		{
			return _note._lowbound;
		}

		double getAccuracy()
		{
			return _note._accuracy;
		}

		VariableType getType()
		{
			return _note._type;
		}

		void copy(ECElement* source)
		{
			_note._name = source->_note._name;
			_note._upbound = source->_note._upbound;
			_note._lowbound = source->_note._lowbound;
			_note._accuracy = source->_note._accuracy;
			_note._type = source->_note._type;
			variable.copy(source->variable);
		}
	};
}