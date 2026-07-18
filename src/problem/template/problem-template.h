//------------------------Description------------------------
// This file defines the templates of classic problem for user,
//  which provides the functions to define specific problem.
// Templates including benchmark optimization, multi-knapsack problem,
// travelling salesman problem, target coverage problem, etc.
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <fstream>
#include <string>
#include <stdexcept>
#include"pt-numerical.h"
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

namespace ECFlow
{
    // —— 通用问题载入器(T-LOAD,v1.4.9 收尾#4)————————————————————————————————
    //   输入问题文件路径 → peek 头部 `TYPE:` 字段识别问题家族 → 分派到对应模板 load + getProblem → 返回 Problem*。
    //   所有 ECFlow 标准算例文件头部均含 `TYPE: <家族名>`(TSP/MKP/QAP/CFLP/CVRP… 已验证一致),故按该字段分派。
    //   **本轮按枚举查询当前类型分派(用户定):暂不引入 ProblemTemplate 基类 + Registry**——
    //   否则 13 个各自独立的模板都要挂基类/注册,超出收尾#4 的最小闭环。基类+注册器见 [未来规划 PROBLEM-TEMPLATE-REGISTRY]。
    //   增量 1 仅接 TSP 一家打通端到端;其余家族随增量 2 逐个加入 include + 分派分支。
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

        // —— 枚举分派(增量 2:13 家族 + 增量2补 NUMERICAL 描述文件)——
        if (type == "NUMERICAL") { PT_Numerical t; t.load(path); return t.getProblem(); }
        if (type == "TSP"  || type == "ATSP") { PT_TSP  t; t.load(path); return t.getProblem(); }
        if (type == "MKP")  { PT_MKP  t; t.load(path); return t.getProblem(); }
        if (type == "QAP")  { PT_QAP  t; t.load(path); return t.getProblem(); }
        if (type == "CFLP") { PT_CFLP t; t.load(path); return t.getProblem(); }
        if (type == "CPMP") { PT_CPMP t; t.load(path); return t.getProblem(); }
        if (type == "CVRP") { PT_CVRP t; t.load(path); return t.getProblem(); }
        if (type == "FJSP") { PT_FJSP t; t.load(path); return t.getProblem(); }
        if (type == "MDS")  { PT_MDS  t; t.load(path); return t.getProblem(); }
        if (type == "MIS")  { PT_MIS  t; t.load(path); return t.getProblem(); }
        if (type == "SR")   { PT_SR   t; t.load(path); return t.getProblem(); }
        if (type == "STP")  { PT_STP  t; t.load(path); return t.getProblem(); }
        if (type == "WFS")  { PT_WFS  t; t.load(path); return t.getProblem(); }

        throw std::runtime_error("[loadProblem] unknown/unsupported problem TYPE '" + type + "' in " + path);
    }
}