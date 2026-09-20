//------------------------Description------------------------
// 问题模板抽象基类 ProblemTemplate:统一全部 PT_* 模板的对外契约
//   (load 载入实例/描述文件、getProblem 产出 Problem),供 ProblemTemplateRegistry 按
//   TYPE 字符串多态构造。加此基类前各 PT_* 各自独立、loadProblem 靠硬编码枚举分派;
//   加基类 + 注册表后,新增家族只需"一文件 + 一行注册",不改 loadProblem。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include<string>

namespace EvoFlow
{
	class Problem;   // 前向声明:基类接口只用 Problem*,不需完整定义

	// 问题模板统一契约。各 PT_* 以 `: public ProblemTemplate` 挂载并 override 两个纯虚。
	class ProblemTemplate
	{
	public:
		// 经基类指针 delete 派生实例 → 必须虚析构(loadProblem 用 ProblemTemplate* 拥有并释放模板)。
		virtual ~ProblemTemplate() = default;

		// 从实例/描述文件载入(短名 → 自适应拼 _pdata 路径,见 pt-instance-path.h)。
		virtual void load(std::string name) = 0;

		// 产出 Problem(调用方拥有,delete 之)。返回的 Problem 与本模板对象生命周期无关,
		//   删除模板不影响已产出的 Problem(评估器等经 copy() 复制,语义同旧栈局部用法)。
		virtual Problem* getProblem() = 0;

		// 把当前用例写回描述/实例文件(overwrite=false 时不覆盖已存在文件)。
		virtual void save(bool overwrite = false) = 0;

		// 复位到默认(空)状态,便于同一模板对象复用/重载。
		virtual void clear() = 0;

		// 设置问题名。
		virtual void setName(std::string name) = 0;
	};
}
