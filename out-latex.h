#pragma once
#include<string>
#include<fstream>
#include<vector>

namespace FileOut
{
    // 函数：将表格数据输出为LaTeX格式的字符串
    // 函数：将表格数据输出为完整的LaTeX文件
    bool writeTableToLaTeX(const std::vector<std::string>& headers,
        const std::vector<std::string>& subheaders,
        const std::vector<std::string>& rowtitles,
        const std::vector<std::vector<std::string>>& tabledata,
        const std::string& filepath,
        const std::vector<std::vector<int>>& bolddata,
        bool ThreeLine = true)
    {
        // 打开文件
        std::ofstream outFile(filepath);
        if (!outFile.is_open()) {
            return false;
        }

        int subcolomn_number;
        if (subheaders.size() == 0)
            subcolomn_number = 1;
        else
            subcolomn_number = subheaders.size();
        int max_colomns = subcolomn_number * headers.size();

        // 写入LaTeX文档头部信息
        outFile << "\\documentclass{article}\n";
        outFile << "\\usepackage[utf8]{inputenc}\n";
        outFile << "\\usepackage{geometry}\n";
        outFile << "\\usepackage{multirow}\n";
        outFile << "\\geometry{a4paper, margin=1in}\n";
        outFile << "\\begin{document}\n\n";

        // 写入表格开始标记
        outFile << "\\begin{table}[ht]\n";
        outFile << "\\centering\n";
        if (ThreeLine) // 三线表
        {
            std::string format = "|c|";
            std::string colomns_format = std::string(subcolomn_number, 'c') + "|";
            for (int i = 0; i < headers.size(); i++)
            {
                format += colomns_format;
            }
            outFile << "\\begin{tabular}{" << format << "}\n";
        }
        else // 全表
        {
            std::string format = "|c|";
            for (int i = 0; i < max_colomns; i++)
            {
                format += "c|";
            }
            outFile << "\\begin{tabular}{" << format << "}\n";
        }
        outFile << "\\hline\n";

        // 写入表头
        if (subheaders.size() == 0) // 无子标题
        {
            // 写入标题
            outFile << " & ";
            for (int col = 0; col < headers.size(); col++) {
                outFile << headers[col];
                if (col < headers.size() - 1) {
                    outFile << " & ";
                }
            }
            outFile << " \\\\ \\hline\n";
        }
        else
        {
            // 写入标题
            outFile << "\\multirow{2}{*}{ } & ";
            for (int col = 0; col < headers.size(); col++) {
                outFile << "\\multicolumn{" << subheaders.size() << "}{c|}{" << headers[col] << "}";
                if (col < headers.size() - 1) {
                    outFile << " & ";
                }
            }
            outFile << " \\\\ \\cline{2-" + std::to_string(1 + max_colomns) + "}\n";

            // 写入子标题
            outFile << " & ";
            for (int col = 0; col < headers.size(); col++) {
                for (int subcol = 0; subcol < subheaders.size(); subcol++) {
                    outFile << subheaders[subcol];
                    if (subcol < subheaders.size() - 1) {
                        outFile << " & ";
                    }
                }
                if (col < headers.size() - 1) {
                    outFile << " & ";
                }
            }
            outFile << " \\\\ \\hline\n";
        }

        // 写入数据
        for (int row = 0; row < rowtitles.size(); ++row)
        {
            bool* bold = new bool[tabledata[row].size()];
            for (int i = 0; i < tabledata[row].size(); i++)
                bold[i] = false;
            if (row < bolddata.size())
                for (int i = 0; i < bolddata[row].size(); i++)
                    bold[bolddata[row][i]] = true;

            outFile << rowtitles[row] << " & ";

            if (tabledata[row].size() != max_colomns)
            {
                int merges = max_colomns / tabledata[row].size();
                for (int col = 0; col < tabledata[row].size(); ++col) {
                    outFile << "\\multicolumn{" + std::to_string(merges) + "}{c|}{" << tabledata[row][col] << "}";
                    if (col < tabledata[row].size() - 1) {
                        outFile << " & ";
                    }
                }
            }
            else
            {
                for (int col = 0; col < tabledata[row].size(); ++col) {
                    outFile << (bold[col] ? "\\textbf{" + tabledata[row][col] + "}" : tabledata[row][col]);
                    ;
                    if (col < tabledata[row].size() - 1) {
                        outFile << " & ";
                    }
                }
            }

            if (row == rowtitles.size() - 1 || !ThreeLine || tabledata[row].size() != tabledata[row + 1].size())
                outFile << " \\\\ \\hline\n";
            else
                outFile << " \\\\ \n";

            delete[] bold;
        }

        // 写入表格结束标记
        outFile << "\\end{tabular}\n";
        outFile << "\\caption{Table}\n";
        outFile << "\\label{table:example}\n";
        outFile << "\\end{table}\n\n";

        // 写入LaTeX文档尾部信息
        outFile << "\\end{document}\n";

        // 关闭文件
        outFile.close();

        return true;
    }
}