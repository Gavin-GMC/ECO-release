#ifndef ALG_STRING_H
#define ALG_STRING_H
#include<string>
#include<vector>

namespace alglib {

    void stringSplit(const std::string& str, const char split, std::vector<std::string>& res)
    {
        if (str == "")
            return;
        //在字符串末尾也加入分隔符，方便截取最后一段
        std::string strs = str + split;
        size_t pos = strs.find(split);

        // 若找不到内容则字符串搜索函数返回 npos
        while (pos != strs.npos)
        {
            std::string temp = strs.substr(0, pos);
            res.push_back(temp);
            //去掉已分割的字符串,在剩下的字符串中进行分割
            strs = strs.substr(pos + 1, strs.size());
            pos = strs.find(split);
        }
    }

    // 讲字符串转化为唯一int
    size_t stringHash(const std::string& str)
    {
        std::hash<std::string> hash_fn;
        size_t hash_value = hash_fn(str);
        return hash_value;
    }
}

#endif // !ALG_STRING_H



