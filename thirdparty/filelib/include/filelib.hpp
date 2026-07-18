#pragma once
#include <string>
#include <algorithm>
#include <vector>

// 跨平台系统头文件适配
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#endif

namespace FileLib {

    /**
     * 辅助函数：统一路径分隔符为 '/'（内部处理用）
     * @param path 原始路径
     * @return 统一分隔符后的路径
     */
    inline std::string normalize_separator(const std::string& path) {
        std::string res = path;
        std::replace(res.begin(), res.end(), '\\', '/');
        return res;
    }

    /**
     * 辅助函数：将路径分隔符转换为系统默认分隔符
     * @param path 统一分隔符后的路径
     * @return 适配系统的路径
     */
    inline std::string convert_to_system_separator(const std::string& path) {
        std::string res = path;
#ifdef _WIN32
        std::replace(res.begin(), res.end(), '/', '\\');
#endif
        return res;
    }

    /**
     * 检查文件是否存在（仅普通文件，不含文件夹）
     * @param path 文件路径
     * @return 存在返回true，否则false
     */
    inline bool file_exist(const std::string& path) {
        if (path.empty()) return false;

#ifdef _WIN32
        DWORD attr = GetFileAttributesA(path.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES) return false;
        // 排除文件夹属性，仅保留普通文件
        return !(attr & FILE_ATTRIBUTE_DIRECTORY);
#else
        struct stat st;
        if (stat(path.c_str(), &st) != 0) return false;
        // S_ISREG 判断是否为普通文件
        return S_ISREG(st.st_mode);
#endif
    }

    /**
     * 检查文件夹是否存在
     * @param path 文件夹路径
     * @return 存在返回true，否则false
     */
    inline bool dir_exist(const std::string& path) {
        if (path.empty()) return false;

#ifdef _WIN32
        DWORD attr = GetFileAttributesA(path.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES) return false;
        // 检查文件夹属性
        return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
        struct stat st;
        if (stat(path.c_str(), &st) != 0) return false;
        // S_ISDIR 判断是否为目录
        return S_ISDIR(st.st_mode);
#endif
    }

    /**
     * 路径拼接（两个路径）
     * @param path1 路径1
     * @param path2 路径2
     * @return 拼接后的完整路径（适配系统分隔符）
     */
    inline std::string path_join(const std::string& path1, const std::string& path2) {
        if (path1.empty()) return path2;
        if (path2.empty()) return path1;

        std::string p1 = normalize_separator(path1);
        std::string p2 = normalize_separator(path2);

        // 移除路径1末尾的 '/'
        if (p1.back() == '/') p1.pop_back();
        // 移除路径2开头的 '/'
        if (p2.front() == '/') p2.erase(0, 1);

        std::string joined = p1 + "/" + p2;
        return convert_to_system_separator(joined);
    }

    /**
     * 路径拼接（多个路径，重载版本）
     * @param paths 路径列表
     * @return 拼接后的完整路径
     */
    inline std::string path_join(const std::vector<std::string>& paths) {
        if (paths.empty()) return "";

        std::string result = paths[0];
        for (size_t i = 1; i < paths.size(); ++i) {
            result = path_join(result, paths[i]);
        }
        return result;
    }

    /**
     * 获取路径的父路径
     * @param path 原始路径
     * @return 父路径（适配系统分隔符）
     */
    inline std::string get_parent_path(const std::string& path) {
        if (path.empty()) return "";

        std::string normalized = normalize_separator(path);
        // 移除末尾所有 '/'
        while (!normalized.empty() && normalized.back() == '/') {
            normalized.pop_back();
        }

        // 空路径（如输入 "//" 或 "/"），返回根目录
        if (normalized.empty()) {
#ifdef _WIN32
            return "\\";
#else
            return "/";
#endif
        }

        // 查找最后一个路径分隔符
        size_t last_sep = normalized.find_last_of('/');
        if (last_sep == std::string::npos) {
            // 无分隔符，返回当前目录 "."
            return ".";
        }

        // 根目录下的一级路径（如 "/a"），返回根目录
        if (last_sep == 0) {
#ifdef _WIN32
            return "\\";
#else
            return "/";
#endif
        }

        // 截取父路径并转换分隔符
        std::string parent = normalized.substr(0, last_sep);
        return convert_to_system_separator(parent);
    }

    /**
     * 创建文件夹
     * @param path 文件夹路径
     * @param recursive 是否递归创建（默认false）
     * @return 创建成功/已存在返回true，失败返回false
     */
    inline bool create_dir(const std::string& path, bool recursive=false) {
        if (path.empty()) return false;

        // 文件夹已存在，直接返回成功
        if (dir_exist(path)) return true;

        // 非递归创建
        if (!recursive) {
#ifdef _WIN32
            return CreateDirectoryA(path.c_str(), NULL) != 0;
#else
            // Linux下mkdir权限设为 0755（rwxr-xr-x）
            return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
        }

        // 递归创建：先创建父目录
        std::string parent = get_parent_path(path);
        if (!parent.empty() && !dir_exist(parent)) {
            if (!create_dir(parent, true)) return false;
        }

        // 创建当前目录
#ifdef _WIN32
        BOOL ret = CreateDirectoryA(path.c_str(), NULL);
        // 处理"已存在"错误（避免并发创建时的误判）
        return ret != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
#else
        int ret = mkdir(path.c_str(), 0755);
        return ret == 0 || errno == EEXIST;
#endif
    }

    /**
    * 获取当前工作目录
    * @return 当前工作目录路径（适配系统分隔符），失败返回空字符串
    */
    inline std::string get_current_dir() {
        // 定义足够大的缓冲区（4096字节适配绝大多数系统路径长度）
        constexpr size_t BUF_SIZE = 4096;
        char buf[BUF_SIZE] = { 0 };

#ifdef _WIN32
        // Windows：GetCurrentDirectoryA 返回实际复制的字符数，0表示失败
        DWORD len = GetCurrentDirectoryA(BUF_SIZE, buf);
        if (len == 0 || len >= BUF_SIZE) {
            return "";  // 缓冲区不足或调用失败
        }
#else
        // Linux：getcwd 返回NULL表示失败，成功返回缓冲区指针
        if (getcwd(buf.data(), BUF_SIZE) == nullptr) {
            return "";
        }
#endif

        // 转换为系统默认分隔符并返回
        return convert_to_system_separator(std::string(buf));
    }

} // namespace FileLib