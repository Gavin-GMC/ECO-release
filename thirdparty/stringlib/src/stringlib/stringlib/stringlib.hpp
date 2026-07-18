#ifndef STRINGLIB_HPP
#define STRINGLIB_HPP

#include <string>
#include <vector>
#include <cctype>
#include <algorithm>
#include <functional>
#include <stdexcept>

// 命名空间隔离，避免命名冲突
namespace stringlib {

    /**
     * @brief 按指定字符分割字符串
     * @param str 待分割的源字符串
     * @param split 分割符（单个字符）
     * @param res 输出参数，存储分割后的字符串列表
     * @note 保留连续分割符产生的空字符串（如 "a,,b" 分割后为 ["a", "", "b"]）
     * @note 空字符串输入会返回空的vector
     */
    inline void stringSplit(const std::string& str, const char split, std::vector<std::string>& res) {
        res.clear(); // 清空结果容器，避免残留数据
        if (str.empty()) {
            return;
        }

        std::string temp;
        for (char c : str) {
            if (c == split) {
                res.push_back(temp);
                temp.clear();
            }
            else {
                temp += c;
            }
        }
        // 处理最后一段字符串（分割符不在末尾的情况）
        res.push_back(temp);
    }

    /**
     * @brief 生成字符串的哈希值（BKDRHash算法，低冲突、高效率）
     * @param str 待计算哈希的字符串
     * @return 无符号哈希值
     * @note BKDRHash是工业界常用的字符串哈希算法，比std::hash更可控（跨平台一致性更好）
     */
    inline size_t stringHash(const std::string& str) {
        size_t hash = 0;
        const size_t seed = 131; // 经典BKDR种子值（131/1313/13131等）
        for (unsigned char c : str) { // 用unsigned char避免tolower/toupper的未定义行为
            hash = hash * seed + c;
        }
        return hash;
    }

    /**
     * @brief 备选哈希函数（基于标准库std::hash）
     * @param str 待计算哈希的字符串
     * @return 无符号哈希值
     * @note 结果可能因编译器/平台不同而变化，适合对跨平台一致性无要求的场景
     */
    inline size_t stringHashStd(const std::string& str) {
        std::hash<std::string> hasher;
        return hasher(str);
    }

    /**
     * @brief 去除字符串首尾的空白字符（空格、制表符、换行符等）
     * @param str 待处理的字符串
     * @return 去除首尾空白后的新字符串
     */
    inline std::string trim(const std::string& str) {
        if (str.empty()) {
            return str;
        }

        // 找到第一个非空白字符的位置
        auto start = str.begin();
        while (start != str.end() && std::isspace(static_cast<unsigned char>(*start))) {
            ++start;
        }

        // 找到最后一个非空白字符的位置
        auto end = str.end();
        do {
            --end;
        } while (std::distance(start, end) > 0 && std::isspace(static_cast<unsigned char>(*end)));

        return std::string(start, end + 1);
    }

    /**
     * @brief 去除字符串左侧的空白字符
     * @param str 待处理的字符串
     * @return 去除左侧空白后的新字符串
     */
    inline std::string trimLeft(const std::string& str) {
        if (str.empty()) {
            return str;
        }

        auto start = str.begin();
        while (start != str.end() && std::isspace(static_cast<unsigned char>(*start))) {
            ++start;
        }

        return std::string(start, str.end());
    }

    /**
     * @brief 去除字符串右侧的空白字符
     * @param str 待处理的字符串
     * @return 去除右侧空白后的新字符串
     */
    inline std::string trimRight(const std::string& str) {
        if (str.empty()) {
            return str;
        }

        auto end = str.end();
        do {
            --end;
        } while (end != str.begin() && std::isspace(static_cast<unsigned char>(*end)));

        return std::string(str.begin(), std::isspace(static_cast<unsigned char>(*end)) ? end : end + 1);
    }

    /**
     * @brief 将字符串转换为大写
     * @param str 待转换的字符串
     * @return 大写后的新字符串
     */
    inline std::string toUpper(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char c) { return std::toupper(c); });
        return result;
    }

    /**
     * @brief 将字符串转换为小写
     * @param str 待转换的字符串
     * @return 小写后的新字符串
     */
    inline std::string toLower(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return result;
    }

    /**
     * @brief 判断字符串是否为空或仅包含空白字符
     * @param str 待判断的字符串
     * @return true: 空/全空白；false: 包含有效字符
     */
    inline bool isEmptyOrWhitespace(const std::string& str) {
        if (str.empty()) {
            return true;
        }
        return std::all_of(str.begin(), str.end(),
            [](unsigned char c) { return std::isspace(c); });
    }

    /**
     * @brief 将字符串数组用指定分隔符拼接成一个字符串
     * @param vec 待拼接的字符串数组
     * @param delimiter 分隔符（单个字符）
     * @return 拼接后的字符串
     */
    inline std::string stringJoin(const std::vector<std::string>& vec, const char delimiter) {
        if (vec.empty()) {
            return "";
        }

        std::string result;
        for (size_t i = 0; i < vec.size() - 1; ++i) {
            result += vec[i];
            result += delimiter;
        }
        result += vec.back(); // 最后一个元素不加分隔符
        return result;
    }

    /**
     * @brief 替换字符串中的指定子串
     * @param str 源字符串（会直接修改）
     * @param oldSub 待替换的子串
     * @param newSub 替换后的新子串
     * @return 替换的次数
     * @throw std::invalid_argument 如果oldSub为空字符串
     */
    inline size_t stringReplace(std::string& str, const std::string& oldSub, const std::string& newSub) {
        if (oldSub.empty()) {
            throw std::invalid_argument("oldSub cannot be empty");
        }

        size_t count = 0;
        size_t pos = 0;
        while ((pos = str.find(oldSub, pos)) != std::string::npos) {
            str.replace(pos, oldSub.length(), newSub);
            pos += newSub.length(); // 跳过新替换的内容，避免重复匹配
            ++count;
        }
        return count;
    }

    /**
     * @brief 判断字符串是否以指定前缀开头
     * @param str 源字符串
     * @param prefix 前缀字符串
     * @return true: 以prefix开头；false: 否
     */
    inline bool startsWith(const std::string& str, const std::string& prefix) {
        if (prefix.length() > str.length()) {
            return false;
        }
        return str.compare(0, prefix.length(), prefix) == 0;
    }

    /**
     * @brief 判断字符串是否以指定后缀结尾
     * @param str 源字符串
     * @param suffix 后缀字符串
     * @return true: 以suffix结尾；false: 否
     */
    inline bool endsWith(const std::string& str, const std::string& suffix) {
        if (suffix.length() > str.length()) {
            return false;
        }
        return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
    }

} // namespace stringlib

#endif // STRINGLIB_HPP