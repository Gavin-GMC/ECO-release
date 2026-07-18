//------------------------Description------------------------
// 问题实例路径的自适应解析 —— 供各问题模板 load() 复用。
//   模板 load 既要支持"短名 + 约定目录"(如 load("a280") → _pdata/tsp/a280.tsp,便于交互/测试),
//   又要支持"完整/相对路径"(如通用载入器 loadProblem 拿到的命令行实参 _pdata/tsp/a280.tsp、C:\x\a.tsp)。
//   由输入形态**自适应**判定:含 '/'、'\\' 或 '.' 之一 → 视为路径原样使用;否则视为短名拼约定路径。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <string>

namespace ECFlow
{
    // 输入含路径特征(目录分隔符 '/'、'\\' 或扩展名点 '.')→ 视为完整/相对路径,原样返回;
    //   否则视为短名 → 拼约定路径 "_pdata/<kind>/<in>.<ext>"(相对进程 CWD 的算例目录)。
    inline std::string resolveInstancePath(const std::string& in, const std::string& kind, const std::string& ext)
    {
        if (in.find('/')  != std::string::npos ||
            in.find('\\') != std::string::npos ||
            in.find('.')  != std::string::npos)
            return in;
        return "_pdata/" + kind + "/" + in + "." + ext;
    }

    // 从路径或短名取一个干净的问题名:去目录前缀 + 去扩展名。
    //   (path="_pdata/tsp/a280.tsp" → "a280";短名 "a280" → "a280")
    inline std::string instanceName(const std::string& in)
    {
        std::string s = in;
        size_t slash = s.find_last_of("/\\");
        if (slash != std::string::npos) s = s.substr(slash + 1);
        size_t dot = s.find_last_of('.');
        if (dot != std::string::npos) s = s.substr(0, dot);
        return s;
    }
}
