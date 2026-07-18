//------------------------Description------------------------
// 组件自注册表:每个组件基类一张 Registry<Base>(分类)+ ECFLOW_REGISTER 自注册宏。
//   取代原 *-type 枚举 / *-factory / setter 的散点维护:新增组件 = 一个文件 + 一行注册。
//-------------------------Reference-------------------------
// 设计与原型见 docs/组件注册表方案.md;原型 registry-prototype/ 的工程化(proto→ECFlow)。
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
#include <map>
#include <cstdio>
#include "module-type.h"
#include "parameter-template.h"
#include "ecflow-assert.h"

namespace ECFlow
{
    inline const char* moduleTypeName(ModuleType m)
    {
        switch (m)
        {
        case ModuleType::T_individual:         return "Individual";
        case ModuleType::T_feature:            return "Feature";
        case ModuleType::T_learnstrategy:      return "LearningStrategy";
        case ModuleType::T_learntopology:      return "LearningTopology";
        case ModuleType::T_offspringgenerator: return "OffspringGenerator";
        case ModuleType::T_selector:           return "Selector";
        case ModuleType::T_evaluator:          return "Evaluator";
        case ModuleType::T_Repair:             return "Repair";
        case ModuleType::T_subswarbuilder:     return "SubswarmBuilder";
        case ModuleType::T_subswarmtopology:   return "SubswarmTopology";
        case ModuleType::T_subswarmmanager:    return "SubswarmManager";
        case ModuleType::T_bestarchive:        return "BestArchive";
        default:                               return "?";
        }
    }

    template <class Base>
    class Registry
    {
    public:
        struct Entry
        {
            std::string        name;
            ModuleType         category;
            // 参数模板:**自包含**(含渐进披露的 next,见 parameter-template.h)。
            //   模板自包含后,`tail()` 返回的模板才能带着**它自己的**下一级(多级披露天然成立),
            //   且 Registry 不必再为此转发。
            ParameterTemplate  params;
            size_t             size;                                 // typeSize
            Base*            (*create)(const double*);               // produce
            void             (*pre)(AssertList&, const double*);     // preAssert(填入传入列表)
            void             (*post)(AssertList&, const double*);    // postAssert(填入传入列表)

            size_t paraNum() const { return params.count(); }        // paraNum(**仅第一级**;总数依各级取值而定)
        };

        static Registry& instance() { static Registry r; return r; }   // 首次调用惰性建表

        // 注册 + 重名检查
        bool add(const Entry& e)
        {
            if (_t.find(e.name) != _t.end())
            {
                std::fprintf(stderr, "[Registry:%s] duplicate registration rejected: \"%s\" already exists\n",
                             moduleTypeName(e.category), e.name.c_str());
                return false;
            }
            _t.emplace(e.name, e);
            return true;
        }

        const Entry* find(const std::string& n) const
        {
            auto it = _t.find(n);
            return it == _t.end() ? nullptr : &it->second;
        }

        Base*  create(const std::string& n, const double* p) const
        {
            auto e = find(n);
            return (e && e->create) ? e->create(p) : nullptr;
        }
        // createArray:造 length 个 Base*(逐个 create),调用方拥有并释放。镜像原 XXFactory::newXxxArray:
        //   未注册 → 返回 nullptr;已注册 → new Base*[length](length 可为 0 → 非空空数组)。
        Base** createArray(const std::string& n, int length, const double* p = nullptr) const
        {
            if (!find(n)) return nullptr;
            Base** arr = new Base*[length];
            for (int i = 0; i < length; ++i) arr[i] = create(n, p);
            return arr;
        }
        size_t sizeOf(const std::string& n) const { auto e = find(n); return e ? e->size : 0; }
        // 参数模板(自包含:渐进披露经 ParameterTemplate::next / nextLevel 逐级取,不再由 Registry 转发)
        const ParameterTemplate* params(const std::string& n) const { auto e = find(n); return e ? &e->params : nullptr; }

        // 断言:填入调用方持有的 AssertList(避免 AssertList 按值拷贝→双重释放)
        void preAssert (const std::string& n, AssertList& out, const double* p) const { auto e = find(n); if (e && e->pre)  e->pre(out, p); }
        void postAssert(const std::string& n, AssertList& out, const double* p) const { auto e = find(n); if (e && e->post) e->post(out, p); }

        std::vector<std::string> names() const
        {
            std::vector<std::string> v;
            for (auto& kv : _t) v.push_back(kv.first);
            return v;
        }

    private:
        std::map<std::string, Entry> _t;
    };

    #define ECFLOW_CAT_(a, b) a##b
    #define ECFLOW_CAT(a, b)  ECFLOW_CAT_(a, b)
    // 自注册宏:inline 变量(C++17)启动前注册;显式 TAG 保证跨 TU 名字稳定(不用 __COUNTER__);
    //   变参接住 Entry 表达式(如 xxxEntry())内部的逗号。
    #define ECFLOW_REGISTER(TAG, Base, ...) \
        inline const bool ECFLOW_CAT(_ecflow_reg_, TAG) = ::ECFlow::Registry<Base>::instance().add(__VA_ARGS__)
}
