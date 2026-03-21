#pragma once
#include<string>
#include<fstream>
#include<vector>

namespace FileOut
{
    // 函数：将表格数据输出为TXT文件，使用制表符作为分隔符
    bool writeTableToTxt(const std::vector<std::string>& headers,
        const std::vector<std::string>& subheaders,
        const std::vector<std::string>& rowtitles,
        const std::vector<std::vector<std::string>>& tabledata,
        const std::string& filepath)
    {
        // 打开文件
        std::ofstream outFile(filepath);
        if (!outFile.is_open()) {
            return false;
        }

        int colomns;
        if (subheaders.size() == 0)
            colomns = headers.size();
        else
            colomns = headers.size() * subheaders.size();

        // 左对齐控制
        int after_t;

        // 写入表头
        after_t = (colomns - headers.size()) / headers.size();
        outFile << "\t";
        for (int col = 0; col < headers.size(); col++)
        {
            outFile << headers[col];
            for (int i = 0; i < after_t; i++)
                outFile << "\t";
            if (col < headers.size() - 1) {
                outFile << "\t"; // 使用制表符分隔
            }
        }
        outFile << "\n";

        // 写入子标题
        if (subheaders.size() != 0)
        {
            outFile << "\t";
            for (int col = 0; col < headers.size(); col++)
            {
                for (int subcol = 0; subcol < subheaders.size(); subcol++)
                {
                    outFile << subheaders[subcol];
                    if (subcol < subheaders.size() - 1) {
                        outFile << "\t"; // 使用制表符分隔
                    }
                }
                if (col < headers.size() - 1) {
                    outFile << "\t"; // 使用制表符分隔
                }
            }
            outFile << "\n";
        }

        // 写入数据
        for (int row = 0; row < rowtitles.size(); ++row)
        {
            outFile << rowtitles[row] << "\t"; // 每行的标题

            if (tabledata[row].size() == 0) // 数据缺失
            {
                outFile << "\n";
                continue;
            }

            // 行数据
            after_t = (colomns - tabledata[row].size()) / tabledata[row].size();
            for (int col = 0; col < tabledata[row].size(); ++col) {
                outFile << tabledata[row][col];
                for (int i = 0; i < after_t; i++)
                    outFile << "\t";
                if (col < tabledata[row].size() - 1) {
                    outFile << "\t"; // 使用制表符分隔
                }
            }
            outFile << "\n";
        }

        // 关闭文件
        outFile.close();

        return true;
    }
}