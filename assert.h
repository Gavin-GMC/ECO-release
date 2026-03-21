//------------------------Description------------------------
// This file is consist of class "Assert", which define the basic
// data format of assert system, and class "AssertList", which 
// provide a container for the assertion object
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
#include<vector>
#include"module-type.h"

namespace ECFC
{
	//
	// 断言检测框架
	// 每个组件有一个断言获取的方法，返回组件包含的断言
	// （组件内的断言以继承的形式组织，用一个向量储存，并由类按照继承进行添加）
	// 根据配置表构建断言矩阵，有以下两种情况
	// 1.配置表已指定组件类别：根据其断言向目标组件进行匹配
	// 2.配置表未指定组件类别：将所有预匹配的断言添加至空向量中，然后向工厂类中获取最小匹配组件
	// 
	// 断言检测只针对配置表进行
	// 后续基于配置表进行优化器生成
	// 
	// 每个工厂类提供
	// 1.基于类别返回对象指针
	// 2.基于类别返回对象断言
	// 
	// 断言匹配器提供
	// 1.对于设置返回断言匹配情况
	// 2.对于自适应设置基于断言返回最小匹配类型和参数
	//
	// 断言匹配共发生两次：
	// 1.优化器配置表内的断言匹配
	// 2.优化器与问题的断言匹配（一般不做拒绝or失败，只做提示）
	//

	enum class MatchType
	{
		notLess,
		notLessButNotice,
		equal,
		anyButNotice,
		postAssert
	};

	class AssertList;

	class Assert
	{
	public:
		enum class MatchResult {
			unsatisfied,
			satisfied,
			notfullysatisfied,
			failsatisfied
		};

		friend AssertList;

	private:
		int _number;
		std::string _item;
		ModuleType _module_type;
		MatchResult(*match_func)(const Assert& target, const Assert& source);

		static MatchResult matchNotLess(const Assert& target, const Assert& source)
		{
			if(target._item != source._item) //条目不匹配
				return MatchResult::unsatisfied;
			if (source._number < target._number)
				return MatchResult::failsatisfied; // 匹配失败
			return MatchResult::satisfied;
		}
		static MatchResult matchNotLessButNotice(const Assert& target, const Assert& source)
		{
			if (target._item != source._item) //条目不匹配
				return MatchResult::unsatisfied;
			if (source._number < target._number) // 匹配失败
				return MatchResult::failsatisfied; 
			if (source._number > target._number) // 不准确匹配
				return MatchResult::notfullysatisfied;
			return MatchResult::satisfied;
		}
		static MatchResult matchEqual(const Assert& target, const Assert& source)
		{
			if (target._item != source._item) //条目不匹配
				return MatchResult::unsatisfied;
			if (source._number != target._number) // 匹配失败
				return MatchResult::failsatisfied;
			return MatchResult::satisfied;
		}
		static MatchResult matchAnyButNotice(const Assert& target, const Assert& source)
		{
			if (target._item != source._item) //条目不匹配
				return MatchResult::unsatisfied;
			if (source._number < target._number) // 不准确匹配
				return MatchResult::notfullysatisfied;
			if (source._number > target._number) // 不准确匹配
				return MatchResult::notfullysatisfied;
			return MatchResult::satisfied;
		}

	public:
		Assert(ModuleType module_type, std::string item, int number = -1, MatchType match_type = MatchType::notLessButNotice)
		{
			_module_type = module_type;
			_item = item;
			_number = number;
			switch (match_type)
			{
			case ECFC::MatchType::notLess:
				match_func = matchNotLess;
				break;
			case ECFC::MatchType::notLessButNotice:
				match_func = matchNotLessButNotice;
				break;
			case ECFC::MatchType::equal:
				match_func = matchEqual;
				break;
			case ECFC::MatchType::anyButNotice:
				match_func = matchAnyButNotice;
				break;
			case ECFC::MatchType::postAssert:
				match_func = nullptr;
				break;
			default:
				match_func = matchNotLessButNotice;
				break;
			}
		}

		Assert(const Assert& target)
		{
			_module_type = target._module_type;
			_item = target._item;
			_number = target._number;
			match_func = target.match_func;
		}

		~Assert()
		{

		}

		bool itemMatch(const Assert& target)
		{
			return _item == target._item;
		}

		ModuleType getModuleType()
		{
			return _module_type;
		}

		std::string getitem()
		{
			return _item;
		}

		int getNumber()
		{
			return _number;
		}

		Assert* copy()
		{
			return new Assert(*this);
		}

		MatchResult match(const Assert& target)
		{
			return match_func(target, *this);
		}
	};

	class AssertList
	{
	private:
		std::vector<Assert*> _list;
	public:
		AssertList()
		{

		}

		~AssertList()
		{
			for (int i = 0; i < _list.size(); i++)
			{
				delete _list[i];
			}
			_list.clear();
		}

		size_t getSize()
		{
			return _list.size();
		}

		void add(Assert* item, bool fusion_merge = false)
		{
			if (fusion_merge)
			{
				bool added = false;
				for (int i = 0; i < _list.size(); i++)
				{
					if (_list[i]->itemMatch(*item))
					{
						_list[i]->_number = std::max(_list[i]->_number, item->_number);
						added = true;
						delete item;
						break;
					}
				}

				if (!added)
				{
					_list.push_back(item);
				}
			}
			else
			{
				_list.push_back(item);
			}
		}

		Assert& operator[](const int index)
		{
			return *_list[index];
		}
	};
}
