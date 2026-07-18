//------------------------Description------------------------
// 配置文件编解码 v3.2:FullConfig ⇄ JSON(机器友好)。手写最小 JSON DOM + 递归下降解析器(无第三方依赖)。
//-------------------------Reference-------------------------
// 全新设计(非迁移)。结构对应 configure-list.h;与 v3.1 文本(config-codec.h)语义等价、载体不同。
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
#include <cctype>
#include "configure-list.h"
#include "config-codec.h"   // FullConfig, codec::num/typeKeyword/keywordType

namespace ECFlow
{
    namespace codec
    {
        // ---- 最小 JSON DOM ----
        struct JsonValue
        {
            enum Type { Null, Bool, Num, Str, Arr, Obj } type = Null;
            bool        b = false;
            double      num = 0;
            std::string str;
            std::vector<JsonValue> arr;
            std::vector<std::pair<std::string, JsonValue>> obj;

            static const JsonValue& none() { static JsonValue n; return n; }
            const JsonValue& operator[](const std::string& key) const
            {
                for (auto& kv : obj) if (kv.first == key) return kv.second;
                return none();
            }
        };

        // ---- 递归下降解析(只支持 codec 产出子集) ----
        struct JsonParser
        {
            const std::string& s; size_t i = 0;
            JsonParser(const std::string& str) : s(str) {}
            void ws() { while (i < s.size() && std::isspace((unsigned char)s[i])) i++; }

            JsonValue parse() { ws(); return value(); }
            JsonValue value()
            {
                ws();
                char c = s[i];
                if (c == '{') return object();
                if (c == '[') return array();
                if (c == '"') { JsonValue v; v.type = JsonValue::Str; v.str = str(); return v; }
                if (c == 't' || c == 'f') { JsonValue v; v.type = JsonValue::Bool; v.b = (c == 't'); i += (c == 't' ? 4 : 5); return v; }
                if (c == 'n') return nullOrNan();
                return number();
            }
            std::string str()
            {
                std::string out; i++;   // 越过开 "
                while (i < s.size() && s[i] != '"')
                {
                    if (s[i] == '\\') { i++; out.push_back(s[i]); }   // 简单转义(\" \\ 等)
                    else out.push_back(s[i]);
                    i++;
                }
                i++;   // 越过闭 "
                return out;
            }
            // 'n' 开头的字面量 → 一律代表"未设置",读回 **EMPTYVALUE**(而非 0)。
            //   **必须按 token 实际长度前进**:null=4、nan=3。原实现一律 `i += 4`,撞上 3 字符的 nan 就**多吃一个字符**
            //   (把后随的 `]`/`,` 吞掉)→ 解析器错位 → 后续 `}` 被送进 number() → 截出空串 → std::stod("") 抛异常。
            //   nan 分支仅为**容错读取** 1.4.6.3 之前 encode 产出的非法 JSON;本版 encode 只印 null。
            JsonValue nullOrNan()
            {
                JsonValue v; v.type = JsonValue::Null; v.num = EMPTYVALUE;
                if      (s.compare(i, 4, "null") == 0) i += 4;
                else if (s.compare(i, 3, "nan")  == 0) i += 3;
                else                                   i += 4;   // 未知 n 开头 token:按 null 长度跳(保守,同原行为)
                return v;
            }
            JsonValue number()
            {
                size_t start = i;
                while (i < s.size() && (std::isdigit((unsigned char)s[i]) || s[i] == '-' || s[i] == '+' || s[i] == '.' || s[i] == 'e' || s[i] == 'E')) i++;
                JsonValue v; v.type = JsonValue::Num; v.num = std::stod(s.substr(start, i - start)); return v;
            }
            JsonValue array()
            {
                JsonValue v; v.type = JsonValue::Arr; i++;   // 越过 [
                ws();
                if (s[i] == ']') { i++; return v; }
                while (true)
                {
                    v.arr.push_back(value()); ws();
                    if (s[i] == ',') { i++; continue; }
                    if (s[i] == ']') { i++; break; }
                }
                return v;
            }
            JsonValue object()
            {
                JsonValue v; v.type = JsonValue::Obj; i++;   // 越过 {
                ws();
                if (s[i] == '}') { i++; return v; }
                while (true)
                {
                    ws(); std::string key = str(); ws();
                    i++;   // 越过 :
                    JsonValue val = value(); v.obj.emplace_back(key, val); ws();
                    if (s[i] == ',') { i++; continue; }
                    if (s[i] == '}') { i++; break; }
                }
                return v;
            }
        };

        inline void jsonReadTagPara(const JsonValue& j, std::string& tag, std::vector<double>& para)
        {
            tag = j["tag"].str; para.clear();
            for (auto& v : j["para"].arr) para.push_back(v.num);
        }
    }

    // ======================= v3.2 JSON 编解码 =======================
    class ConfigCodecV32
    {
    public:
        // 未设置参数(EMPTYVALUE)→ JSON **null**。理由:裸 `nan` **不是合法 JSON 字面量**(JSON 数字文法不含它),
        //   而 null 既合法、语义又恰好是"无值"。encode 全仓仅此一处产出 null,故 decode 可无歧义地把 null 读回 EMPTYVALUE。
        static std::string jsonNum(double v) { return is_empty(v) ? std::string("null") : codec::num(v); }
    private:
        static void tagPara(std::ostringstream& o, const std::string& key, const std::string& tag, const std::vector<double>& para)
        {
            o << "\"" << key << "\":{\"tag\":\"" << tag << "\",\"para\":[";
            for (size_t i = 0; i < para.size(); i++) { if (i) o << ","; o << jsonNum(para[i]); }
            o << "]}";
        }
    public:
        static std::string encode(const FullConfig& fc)
        {
            std::ostringstream o;
            o << "{\"version\":\"v3.2\",\"workflows\":[";
            for (size_t w = 0; w < fc.workflows.size(); w++)
            {
                const WorkflowConfig& wf = fc.workflows[w];
                if (w) o << ",";
                o << "{\"tag\":\"" << wf.tag << "\",";
                tagPara(o, "initializer", wf.ini_tag, wf.ini_para);
                o << ",\"components\":[";
                for (size_t c = 0; c < wf.components.size(); c++)
                {
                    const ComponentConfig& cc = wf.components[c];
                    if (c) o << ",";
                    o << "{\"type\":\"" << codec::typeKeyword(cc.c_type) << "\",\"tag\":\"" << cc.tag << "\",\"para\":[";
                    for (size_t k = 0; k < cc.para.size(); k++) { if (k) o << ","; o << jsonNum(cc.para[k]); }
                    o << "]}";
                }
                o << "]}";
            }
            o << "],\"optimizer\":{";
            const OptimizerConfig& op = fc.optimizer;
            o << "\"name\":\"" << op.name << "\",\"tag\":\"" << op.tag << "\",";
            o << "\"terminator\":[" << op.terminate_conditions[0] << "," << op.terminate_conditions[1] << "," << op.terminate_conditions[2] << "],";
            o << "\"logging\":[" << (op.logger_full_result ? "true" : "false") << "," << (op.logger_process ? "true" : "false") << "," << (op.logger_full_process ? "true" : "false") << "],";
            tagPara(o, "manager", op.cooperation.manager_tag, op.cooperation.manager_para); o << ",";
            tagPara(o, "constructer", op.cooperation.constructer_tag, op.cooperation.constructer_para); o << ",";
            tagPara(o, "ctopology", op.cooperation.topology_tag, op.cooperation.topology_para); o << ",";
            tagPara(o, "g_archive", op.g_archive_tag, op.g_archive_para); o << ",";
            o << "\"subpopulations\":[";
            for (size_t s = 0; s < op.subpopulations.size(); s++)
            {
                const SubpopulationConfig& sp = op.subpopulations[s];
                if (s) o << ",";
                o << "{\"tag\":\"" << sp.tag << "\",\"size\":" << sp.size
                  << ",\"workflow\":\"" << sp.workflow_tag << "\",\"terminator\":[" << sp.terminate_conditions[0] << "," << sp.terminate_conditions[1] << "," << sp.terminate_conditions[2] << "],";
                tagPara(o, "archive", sp.archive_tag, sp.archive_para);
                o << "}";
            }
            o << "]}}";
            return o.str();
        }

        static FullConfig decode(const std::string& json)
        {
            FullConfig fc;
            codec::JsonParser parser(json);
            codec::JsonValue root = parser.parse();

            for (const auto& jw : root["workflows"].arr)
            {
                WorkflowConfig wf; wf.tag = jw["tag"].str;
                codec::jsonReadTagPara(jw["initializer"], wf.ini_tag, wf.ini_para);
                for (const auto& jc : jw["components"].arr)
                {
                    ComponentConfig c; c.c_type = codec::keywordType(jc["type"].str); c.tag = jc["tag"].str;
                    for (const auto& v : jc["para"].arr) c.para.push_back(v.num);
                    wf.components.push_back(std::move(c));
                }
                fc.workflows.push_back(std::move(wf));
            }

            const codec::JsonValue& jo = root["optimizer"];
            OptimizerConfig& op = fc.optimizer;
            op.name = jo["name"].str; op.tag = jo["tag"].str;
            op.terminate_conditions[0] = (int)jo["terminator"].arr[0].num;
            op.terminate_conditions[1] = (int)jo["terminator"].arr[1].num;
            op.terminate_conditions[2] = (int)jo["terminator"].arr[2].num;
            op.logger_full_result = jo["logging"].arr[0].b;
            op.logger_process = jo["logging"].arr[1].b;
            op.logger_full_process = jo["logging"].arr[2].b;
            codec::jsonReadTagPara(jo["manager"], op.cooperation.manager_tag, op.cooperation.manager_para);
            codec::jsonReadTagPara(jo["constructer"], op.cooperation.constructer_tag, op.cooperation.constructer_para);
            codec::jsonReadTagPara(jo["ctopology"], op.cooperation.topology_tag, op.cooperation.topology_para);
            codec::jsonReadTagPara(jo["g_archive"], op.g_archive_tag, op.g_archive_para);

            for (const auto& js : jo["subpopulations"].arr)
            {
                SubpopulationConfig sp;
                sp.tag = js["tag"].str; sp.size = (int)js["size"].num;
                sp.workflow_tag = js["workflow"].str;
                sp.terminate_conditions[0] = (int)js["terminator"].arr[0].num;
                sp.terminate_conditions[1] = (int)js["terminator"].arr[1].num;
                sp.terminate_conditions[2] = (int)js["terminator"].arr[2].num;
                codec::jsonReadTagPara(js["archive"], sp.archive_tag, sp.archive_para);
                op.subpopulations.push_back(std::move(sp));
            }
            return fc;
        }
    };
}
