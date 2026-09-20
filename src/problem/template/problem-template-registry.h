//------------------------Description------------------------
// 问题模板自注册表:TYPE 字符串 → 工厂(()->ProblemTemplate*)。
//   镜像 optimizer 侧 Registry<组件> 的**自注册模式**(Meyers 单例 + 启动前 inline 变量注册),
//   但**轻量、fit-for-purpose**:问题模板无参数模板/断言/ModuleType 语义,故不复用 optimizer 的
//   Registry<Base>(那会让问题层反向依赖优化器层——层次倒置)。本表只需 TYPE→工厂。
//   各 PT_* 文件末 `EVOFLOW_REGISTER_PROBLEM(tag,"TYPE",PT_XXX)` 自注册,loadProblem 查表构造。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include<string>
#include<map>
#include<vector>
#include<cstdio>
#include"problem-template-base.h"

namespace EvoFlow
{
	// TYPE 字符串 → 问题模板工厂。首次调用惰性建表(Meyers 单例,规避静态初始化顺序问题)。
	class ProblemTemplateRegistry
	{
	public:
		using Factory = ProblemTemplate * (*)();

		static ProblemTemplateRegistry& instance() { static ProblemTemplateRegistry r; return r; }

		// 注册 + 重名检查(重复 TYPE 拒绝并告警,返回 false)。
		bool add(const std::string& type, Factory f)
		{
			if (_t.find(type) != _t.end())
			{
				std::fprintf(stderr, "[ProblemTemplateRegistry] duplicate TYPE rejected: \"%s\" already registered\n", type.c_str());
				return false;
			}
			_t.emplace(type, f);
			return true;
		}

		// 按 TYPE 造模板实例(调用方拥有并 delete);未注册 → nullptr。
		ProblemTemplate* create(const std::string& type) const
		{
			auto it = _t.find(type);
			return it == _t.end() ? nullptr : it->second();
		}

		std::vector<std::string> types() const
		{
			std::vector<std::string> v;
			for (auto& kv : _t) v.push_back(kv.first);
			return v;
		}

	private:
		std::map<std::string, Factory> _t;
	};

	#define EVOFLOW_PT_CAT_(a, b) a##b
	#define EVOFLOW_PT_CAT(a, b)  EVOFLOW_PT_CAT_(a, b)
	// 自注册宏:inline 变量(C++17)在 main 前注册 TYPE→工厂;TAG 保证跨 TU 名字稳定 → 只注册一次。
	//   用法(文件末,全局作用域):EVOFLOW_REGISTER_PROBLEM(tsp, "TSP", EvoFlow::PT_TSP);
	#define EVOFLOW_REGISTER_PROBLEM(TAG, TYPESTR, CLS) \
		inline const bool EVOFLOW_PT_CAT(_evoflow_pt_reg_, TAG) = \
			::EvoFlow::ProblemTemplateRegistry::instance().add(TYPESTR, []() -> ::EvoFlow::ProblemTemplate* { return new CLS(); })
}
