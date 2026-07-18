//------------------------Description------------------------
// 断言系统的基本数据结构:Assert(单条断言的数据格式 + 匹配)与 AssertList(断言容器)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include "module-type.h"

namespace ECFlow
{
    // 匹配方式:决定一条断言如何比较期望值与实际值
    enum class MatchType
    {
        notLess,            // 实际 ≥ 期望;否则失败
        notLessButNotice,   // 实际 ≥ 期望;超出仅提示(不精确)
        equal,              // 实际 == 期望
        anyButNotice,       // 任意;不等仅提示
        postAssert          // 后置断言(无匹配函数)
    };

    class AssertList;

    class Assert
    {
    public:
        enum class MatchResult {
            unsatisfied,        // 项不匹配(item 不同)
            satisfied,          // 满足
            notfullysatisfied,  // 满足但不精确(可提示)
            failsatisfied       // 违反(失败)
        };

        friend AssertList;

    private:
        int         _number;
        std::string _item;
        ModuleType  _module_type;
        MatchType   _match_type;   // 保存匹配语义(供 AssertMatcher 按语义直接比对,避开 postAssert 的 nullptr match_func)
        MatchResult (*match_func)(const Assert& target, const Assert& source);

        static MatchResult matchNotLess(const Assert& target, const Assert& source)
        {
            if (target._item != source._item)     return MatchResult::unsatisfied;    // 项不匹配
            if (source._number < target._number)  return MatchResult::failsatisfied;  // 匹配失败
            return MatchResult::satisfied;
        }
        static MatchResult matchNotLessButNotice(const Assert& target, const Assert& source)
        {
            if (target._item != source._item)     return MatchResult::unsatisfied;       // 项不匹配
            if (source._number < target._number)  return MatchResult::failsatisfied;     // 匹配失败
            if (source._number > target._number)  return MatchResult::notfullysatisfied; // 满足但不精确
            return MatchResult::satisfied;
        }
        static MatchResult matchEqual(const Assert& target, const Assert& source)
        {
            if (target._item != source._item)     return MatchResult::unsatisfied;   // 项不匹配
            if (source._number != target._number) return MatchResult::failsatisfied; // 匹配失败
            return MatchResult::satisfied;
        }
        static MatchResult matchAnyButNotice(const Assert& target, const Assert& source)
        {
            if (target._item != source._item)     return MatchResult::unsatisfied;       // 项不匹配
            if (source._number < target._number)  return MatchResult::notfullysatisfied; // 不精确
            if (source._number > target._number)  return MatchResult::notfullysatisfied; // 不精确
            return MatchResult::satisfied;
        }

    public:
        Assert(ModuleType module_type, std::string item, int number = -1, MatchType match_type = MatchType::notLessButNotice)
        {
            _module_type = module_type;
            _item = item;
            _number = number;
            _match_type = match_type;
            switch (match_type)
            {
            case MatchType::notLess:          match_func = matchNotLess;          break;
            case MatchType::notLessButNotice: match_func = matchNotLessButNotice; break;
            case MatchType::equal:            match_func = matchEqual;            break;
            case MatchType::anyButNotice:     match_func = matchAnyButNotice;     break;
            case MatchType::postAssert:       match_func = nullptr;               break;
            default:                          match_func = matchNotLessButNotice; break;
            }
        }

        Assert(const Assert& target)
        {
            _module_type = target._module_type;
            _item = target._item;
            _number = target._number;
            _match_type = target._match_type;
            match_func = target.match_func;
        }

        ~Assert() {}

        bool        itemMatch(const Assert& target) { return _item == target._item; }
        ModuleType  getModuleType() { return _module_type; }
        std::string getitem()       { return _item; }
        int         getNumber()     { return _number; }
        MatchType   getMatchType()  { return _match_type; }
        Assert*     copy()          { return new Assert(*this); }
        MatchResult match(const Assert& target) { return match_func(target, *this); }
    };

    class AssertList
    {
    private:
        std::vector<Assert*> _list;
    public:
        AssertList() {}
        ~AssertList()
        {
            for (size_t i = 0; i < _list.size(); i++) delete _list[i];
            _list.clear();
        }

        size_t getSize() const { return _list.size(); }

        void add(Assert* item, bool fusion_merge = false)
        {
            if (fusion_merge)
            {
                bool added = false;
                for (size_t i = 0; i < _list.size(); i++)
                {
                    if (_list[i]->itemMatch(*item))
                    {
                        _list[i]->_number = std::max(_list[i]->_number, item->_number);
                        added = true;
                        delete item;
                        break;
                    }
                }
                if (!added) _list.push_back(item);
            }
            else
            {
                _list.push_back(item);
            }
        }

        Assert& operator[](const int index) { return *_list[index]; }
    };
}
