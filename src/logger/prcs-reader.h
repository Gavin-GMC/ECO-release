//------------------------Description------------------------
// `.prcs`(过程日志)读取器。
// 与写入端(logger.hpp 落文件头 / subpopulation.h 落记录)【同目录】——格式的写与读必须同步演进,
// 放一起时漂移一眼可见;且 CLI/UI 侧要用它时不必拖进整个 analyzer。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

namespace EvoFlow
{
    class PrcsReader
    {
    public:
        struct Header
        {
            std::string version, optimizer, tag, problem;
            int  run = 0;
            bool full_process = false;
        };
        struct Record
        {
            long long fes = 0;
            std::string swarm;
            int nbest = 0, npop = 0;
            std::vector<std::vector<double>> best;   // [i][objective]
            std::vector<std::vector<double>> pop;    // [i][objective];未开整群时为空
        };

        // 逐记录回调。onRecord 形如 void(const Record&)。
        // 返回 false 并置 err(不在此处写日志——保持对 CLI/UI 可复用)。
        template <class Fn>
        static bool read(const std::string& path, Header& head, Fn onRecord, std::string& err)
        {
            err.clear();
            std::ifstream f(path);
            if (!f.is_open()) { err = "cannot open " + path; return false; }

            std::string line;
            if (!std::getline(f, line)) { err = "empty file " + path; return false; }
            _chomp(line);

            // ---- 版本探测:只认 v1.1 ----
            if (line.rfind("<ver=", 0) != 0)
            {
                err = "unversioned .prcs (pre-v2.2.2.2 bare line stream) is not supported: " + path
                    + " -- it carries no FES column; re-run with a current build to get a v1.1 trace";
                return false;
            }
            head.version = _versionToken(line);
            if (head.version != "1.1" && head.version != "1.2")   // v2.5.3:两级化升 1.2,读取端加收
            {
                err = "unsupported .prcs version '" + head.version + "' in " + path
                    + " -- only 1.1 / 1.2 are supported";
                return false;
            }

            // ---- 元信息行 ----
            if (!std::getline(f, line)) { err = "truncated header in " + path; return false; }
            _chomp(line);
            {
                std::vector<std::string> t = _split(line);
                for (size_t i = 0; i + 1 < t.size(); i += 2)
                {
                    if      (t[i] == "Optimizer:")    head.optimizer = t[i + 1];
                    else if (t[i] == "Tag:")          head.tag = t[i + 1];
                    else if (t[i] == "Problem:")      head.problem = t[i + 1];
                    else if (t[i] == "Run:")          head.run = std::atoi(t[i + 1].c_str());
                    else if (t[i] == "Full_Process:") head.full_process = (t[i + 1] != "0");
                }
            }
            if (!std::getline(f, line)) { err = "truncated header in " + path; return false; }  // 分隔线

            // ---- 记录 ----
            Record cur; bool open = false, inPop = false; long recs = 0;
            while (std::getline(f, line))
            {
                _chomp(line);
                if (line.empty()) continue;

                if (line.rfind("#rec", 0) == 0)
                {
                    if (open) { onRecord(cur); recs++; }
                    cur = Record(); inPop = false;
                    if (!_parseRec(line, cur)) { err = "malformed #rec line in " + path + ": " + line; return false; }
                    open = true;
                }
                else if (line.rfind("#---", 0) == 0)
                {
                    if (!open) { err = "separator before any #rec in " + path; return false; }
                    inPop = true;
                }
                else if (line[0] == '#')   // v2.5.3(v1.2):跳过其它元行(如 #epoch 边界),不参与记录解析
                {
                    continue;
                }
                else if (open && !line.empty() && (line[0] >= '0' && line[0] <= '9'))
                {
                    std::vector<double> obj = _numbers(line);
                    if (obj.empty()) { err = "data line without a numeric value in " + path + ": " + line; return false; }
                    (inPop ? cur.pop : cur.best).push_back(obj);
                    if (head.full_process && !_skipBlock(f))
                    { err = "full-process block not terminated by EOS in " + path; return false; }
                }
                else
                {
                    err = "unrecognised line in " + path + ": " + line;
                    return false;
                }
            }
            if (open) { onRecord(cur); recs++; }
            if (recs == 0) { err = "no records in " + path; return false; }
            return true;
        }

    private:
        static void _chomp(std::string& s)
        { while (!s.empty() && (s.back() == '\r' || s.back() == '\n')) s.pop_back(); }

        static std::vector<std::string> _split(const std::string& s)
        {
            std::vector<std::string> v; std::istringstream is(s); std::string t;
            while (is >> t) v.push_back(t);
            return v;
        }

        static std::string _versionToken(const std::string& line)
        {
            std::vector<std::string> t = _split(line);      // "<ver=" "1.1" ">"
            return (t.size() >= 2) ? t[1] : "";
        }

        // 与 ec-analyzer::loadResult 同规则:丢弃以 ':' / '-' 结尾的标签 token,其余按数解析
        static std::vector<double> _numbers(const std::string& line)
        {
            std::vector<double> v;
            std::vector<std::string> t = _split(line);
            for (size_t i = 1; i < t.size(); i++)           // t[0] 是块内序号 "N:"
            {
                if (t[i].empty() || t[i].back() == ':' || t[i].back() == '-') continue;
                try { v.push_back(std::stod(t[i])); } catch (...) { }
            }
            return v;
        }

        // full_process 下一条数据是【多行块】(目标值行 + 逐变量行 + EOS),同 .rslt。
        // 轨迹只需目标值 → 吞掉续行至 EOS。
        static bool _skipBlock(std::ifstream& f)
        {
            std::string l;
            while (std::getline(f, l))
            {
                _chomp(l);
                if (l == "EOS") return true;
            }
            return false;
        }

        static bool _parseRec(const std::string& line, Record& r)
        {
            std::vector<std::string> t = _split(line);      // #rec fes N swarm ID nbest M npop Q
            if (t.size() != 9) return false;
            if (t[1] != "fes" || t[3] != "swarm" || t[5] != "nbest" || t[7] != "npop") return false;
            r.fes   = std::atoll(t[2].c_str());
            r.swarm = t[4];
            r.nbest = std::atoi(t[6].c_str());
            r.npop  = std::atoi(t[8].c_str());
            return true;
        }
    };
}
