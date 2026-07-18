//------------------------Description------------------------
// 配置文件编解码。FullConfig(workflows + OptimizerConfig)⇄ 文本。
//-------------------------Reference-------------------------
// 全新设计(非迁移);格式对应 configure-list.h 的结构。
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
#include <sstream>
#include <iomanip>
#include <cmath>
#include "configure-list.h"
#include "ecflow-constant.h"   // EMPTYVALUE / is_empty:未设置参数需跨越序列化边界

namespace ECFlow
{
    // 序列化单元:一份完整配置 = 若干 workflow + 一个 optimizer(含子群/协作/全局档案)
    struct FullConfig
    {
        std::vector<WorkflowConfig> workflows;
        OptimizerConfig             optimizer;
    };

    namespace codec
    {
        // ModuleType ↔ 组件关键字
        inline std::string typeKeyword(ModuleType t)
        {
            switch (t)
            {
            case ModuleType::T_learntopology:      return "topology";
            case ModuleType::T_learnstrategy:      return "strategy";
            case ModuleType::T_offspringgenerator: return "generator";
            case ModuleType::T_selector:           return "selector";
            case ModuleType::T_evaluator:          return "evaluator";
            case ModuleType::T_Repair:             return "repair";
            default:                               return "unknown";
            }
        }
        inline ModuleType keywordType(const std::string& k)
        {
            if (k == "topology")  return ModuleType::T_learntopology;
            if (k == "strategy")  return ModuleType::T_learnstrategy;
            if (k == "generator") return ModuleType::T_offspringgenerator;
            if (k == "selector")  return ModuleType::T_selector;
            if (k == "evaluator") return ModuleType::T_evaluator;
            if (k == "repair")    return ModuleType::T_Repair;
            return ModuleType::T_default;
        }

        // 数值文本化:整值印成整数,否则全精度(保证 round-trip 精确)。
        // ⚠ EMPTYVALUE(=quiet_NaN)走**下面这条**分支印成 "nan":`std::floor(NaN)==NaN` 为**假**(NaN 自比较恒假),
        //   恰好短路掉 `(long long)NaN` —— 那是 **UB**。该短路是"碰巧正确"而非设计,**不得**把上面的判断
        //   "优化"成任何会让 NaN 落进整数分支的形式(如先判 fabs、或改用 std::trunc/位运算等)。
        inline std::string num(double v)
        {
            if (std::floor(v) == v && std::fabs(v) < 1e15)
                return std::to_string((long long)v);
            std::ostringstream s; s << std::setprecision(17) << v; return s.str();
        }
        inline std::vector<std::string> split(const std::string& line)
        {
            std::vector<std::string> t; std::istringstream s(line); std::string w;
            while (s >> w) t.push_back(w);
            return t;
        }
        inline void appendPara(std::ostringstream& o, const std::vector<double>& para)
        {
            for (double p : para) o << " " << num(p);
        }
        // tokens[from..] → tag + para
        inline void readTagPara(const std::vector<std::string>& tk, size_t from, std::string& tag, std::vector<double>& para)
        {
            tag = (from < tk.size()) ? tk[from] : "";
            para.clear();
            for (size_t i = from + 1; i < tk.size(); i++) para.push_back(std::stod(tk[i]));
        }
    }

    // ======================= v3.1 文本编解码 =======================
    class ConfigCodecV31
    {
    public:
        static std::string encode(const FullConfig& fc)
        {
            std::ostringstream o;
            o << "<ver= v3.1>\n";

            for (const WorkflowConfig& wf : fc.workflows)
            {
                o << "[workflow] " << wf.tag << "\n";
                o << "initializer " << wf.ini_tag; codec::appendPara(o, wf.ini_para); o << "\n";
                for (const ComponentConfig& c : wf.components)
                {
                    o << "component " << codec::typeKeyword(c.c_type) << " " << c.tag;
                    codec::appendPara(o, c.para); o << "\n";
                }
            }

            const OptimizerConfig& op = fc.optimizer;
            o << "[optimizer]\n";
            o << "name " << op.name << "\n";
            o << "tag " << op.tag << "\n";
            o << "terminator " << op.terminate_conditions[0] << " " << op.terminate_conditions[1] << " " << op.terminate_conditions[2] << "\n";
            o << "logging " << (op.logger_full_result ? 1 : 0) << " " << (op.logger_process ? 1 : 0) << " " << (op.logger_full_process ? 1 : 0) << "\n";
            o << "manager " << op.cooperation.manager_tag; codec::appendPara(o, op.cooperation.manager_para); o << "\n";
            o << "constructer " << op.cooperation.constructer_tag; codec::appendPara(o, op.cooperation.constructer_para); o << "\n";
            o << "ctopology " << op.cooperation.topology_tag; codec::appendPara(o, op.cooperation.topology_para); o << "\n";
            o << "garchive " << op.g_archive_tag; codec::appendPara(o, op.g_archive_para); o << "\n";

            for (const SubpopulationConfig& sp : op.subpopulations)
            {
                o << "[subpopulation]\n";
                o << "tag " << sp.tag << "\n";
                o << "size " << sp.size << "\n";
                o << "workflow " << sp.workflow_tag << "\n";
                o << "terminator " << sp.terminate_conditions[0] << " " << sp.terminate_conditions[1] << " " << sp.terminate_conditions[2] << "\n";
                o << "archive " << sp.archive_tag; codec::appendPara(o, sp.archive_para); o << "\n";
            }
            return o.str();
        }

        static FullConfig decode(const std::string& text)
        {
            FullConfig fc;
            std::istringstream in(text);
            std::string line;
            int block = 0;   // 0 none, 1 workflow, 2 optimizer, 3 subpopulation

            while (std::getline(in, line))
            {
                std::vector<std::string> tk = codec::split(line);
                if (tk.empty()) continue;
                const std::string& key = tk[0];

                if (key == "<ver=") continue;                         // 版本头
                if (key == "[workflow]")
                {
                    block = 1; fc.workflows.emplace_back();
                    fc.workflows.back().tag = (tk.size() > 1) ? tk[1] : "";
                    continue;
                }
                if (key == "[optimizer]")      { block = 2; continue; }
                if (key == "[subpopulation]")  { block = 3; fc.optimizer.subpopulations.emplace_back(); continue; }

                if (block == 1)
                {
                    WorkflowConfig& wf = fc.workflows.back();
                    if (key == "initializer")     codec::readTagPara(tk, 1, wf.ini_tag, wf.ini_para);
                    else if (key == "component")
                    {
                        ComponentConfig c; c.c_type = codec::keywordType(tk[1]);
                        codec::readTagPara(tk, 2, c.tag, c.para);
                        wf.components.push_back(std::move(c));
                    }
                }
                else if (block == 2)
                {
                    OptimizerConfig& op = fc.optimizer;
                    if (key == "name")            op.name = (tk.size() > 1) ? tk[1] : "";
                    else if (key == "tag")        op.tag  = (tk.size() > 1) ? tk[1] : "";
                    else if (key == "terminator") { op.terminate_conditions[0] = std::stoi(tk[1]); op.terminate_conditions[1] = std::stoi(tk[2]); op.terminate_conditions[2] = std::stoi(tk[3]); }
                    else if (key == "logging")    { op.logger_full_result = tk[1] != "0"; op.logger_process = tk[2] != "0"; op.logger_full_process = tk[3] != "0"; }
                    else if (key == "manager")    codec::readTagPara(tk, 1, op.cooperation.manager_tag, op.cooperation.manager_para);
                    else if (key == "constructer")codec::readTagPara(tk, 1, op.cooperation.constructer_tag, op.cooperation.constructer_para);
                    else if (key == "ctopology")  codec::readTagPara(tk, 1, op.cooperation.topology_tag, op.cooperation.topology_para);
                    else if (key == "garchive")   codec::readTagPara(tk, 1, op.g_archive_tag, op.g_archive_para);
                }
                else if (block == 3)
                {
                    SubpopulationConfig& sp = fc.optimizer.subpopulations.back();
                    if (key == "tag")             sp.tag = (tk.size() > 1) ? tk[1] : "";
                    else if (key == "individual") { /* v1.4.8 已删该项;容忍旧配置表的遗留行,读到即忽略 */ }
                    else if (key == "size")       sp.size = std::stoi(tk[1]);
                    else if (key == "workflow")   sp.workflow_tag = (tk.size() > 1) ? tk[1] : "";
                    else if (key == "terminator") { sp.terminate_conditions[0] = std::stoi(tk[1]); sp.terminate_conditions[1] = std::stoi(tk[2]); sp.terminate_conditions[2] = std::stoi(tk[3]); }
                    else if (key == "archive")    codec::readTagPara(tk, 1, sp.archive_tag, sp.archive_para);
                }
            }
            return fc;
        }
    };
}
