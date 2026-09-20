//------------------------Description------------------------
// This file defines the templates of classic problem for user,
//  which provides the functions to define specific problem.
// Templates including benchmark optimization, multi-knapsack problem,
// travelling salesman problem, target coverage problem, etc.

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <fstream>
#include <string>
#include <stdexcept>
#include"pt-tsp.h"
#include"pt-mkp.h"
#include"pt-qap.h"
#include"pt-cflp.h"
#include"pt-cpmp.h"
#include"pt-cvrp.h"
#include"pt-fjsp.h"
#include"pt-mds.h"
#include"pt-mis.h"
#include"pt-sr.h"
#include"pt-stp.h"
#include"pt-wfs.h"
#include"problem-template-registry.h"   // TYPE→模板 注册表(各 pt-*.h 末尾自注册),loadProblem 查表分派

namespace EvoFlow
{
    // —— 通用问题载入器(T-LOAD,v1.4.9 收尾#4)————————————————————————————————
    //   输入问题文件路径 → peek 头部 `TYPE:` 字段识别问题家族 → 分派到对应模板 load + getProblem → 返回 Problem*。
    //   所有 EvoFlow 标准算例文件头部均含 `TYPE: <家族名>`(TSP/MKP/QAP/CFLP/CVRP… 已验证一致),故按该字段分派。
    //   **v2.0.2(PROBLEM-TEMPLATE-REGISTRY):枚举 if-else 分派 → 注册表查表**——各 PT_* 挂 `ProblemTemplate`
    //   基类并在自身文件末 `EVOFLOW_REGISTER_PROBLEM(tag,"TYPE",PT_XXX)` 自注册;加新家族只需"一文件 + 一行注册",
    //   本函数无需改动。注册表为问题层专用轻量版(不复用优化器 Registry,避免问题层反依赖优化器层)。
    //   返回的 Problem* 由**调用方拥有**(delete 之)。

    // 从文件头 peek `TYPE:` 的值(轻量扫描,不消费模板自身的完整解析)。找不到返回空串。
    inline std::string peekProblemType(const std::string& path)
    {
        std::ifstream f(path);
        if (!f) throw std::runtime_error("[loadProblem] cannot open problem file: " + path);
        std::string tok;
        while (f >> tok)
        {
            if (tok == "TYPE" || tok == "TYPE:")
            {
                std::string val;
                if (!(f >> val)) break;
                if (val == ":") { if (!(f >> val)) break; }   // 容错 "TYPE : XXX" 写法
                return val;
            }
        }
        return "";
    }

    inline Problem* loadProblem(const std::string& path)
    {
        std::string type = peekProblemType(path);
        if (type.empty())
            throw std::runtime_error("[loadProblem] no 'TYPE:' field in problem file: " + path);

        // —— 注册表查表分派:按 TYPE 造模板 → load → getProblem(加新家族无需改此处)——
        ProblemTemplate* t = ProblemTemplateRegistry::instance().create(type);
        if (!t)
            throw std::runtime_error("[loadProblem] unknown/unsupported problem TYPE '" + type + "' in " + path);

        t->load(path);
        Problem* p = t->getProblem();   // 产出的 Problem 独立于模板(评估器经 copy 复制)
        delete t;                       // 语义同旧栈局部用法:getProblem 后即析构模板
        return p;
    }
}
