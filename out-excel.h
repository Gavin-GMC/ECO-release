#pragma once
#include <string>
#include <fstream>
#include <vector>

namespace FileOut
{
    // 函数：将表格数据输出为CSV文件
    bool writeTableToCSV(const std::vector<std::string>& headers,
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
        outFile << ",";
        for (int col = 0; col < headers.size(); col++)
        {
            outFile << headers[col];
            for (int i = 0; i < after_t; i++)
                outFile << ",";
            if (col < headers.size() - 1) {
                outFile << ","; // 使用逗号分隔
            }
        }
        outFile << "\n";

        // 写入子标题
        if (subheaders.size() != 0)
        {
            outFile << ",";
            for (int col = 0; col < headers.size(); col++)
            {
                for (int subcol = 0; subcol < subheaders.size(); subcol++)
                {
                    outFile << subheaders[subcol];
                    if (subcol < subheaders.size() - 1) {
                        outFile << ","; // 使用制表符分隔
                    }
                }
                if (col < headers.size() - 1) {
                    outFile << ","; // 使用制表符分隔
                }
            }
            outFile << "\n";
        }

        // 写入数据
        for (int row = 0; row < rowtitles.size(); ++row)
        {
            outFile << rowtitles[row] << ","; // 每行的标题

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
                    outFile << ",";
                if (col < tabledata[row].size() - 1) {
                    outFile << ","; // 使用制表符分隔
                }
            }
            outFile << "\n";
        }

        // 关闭文件
        outFile.close();

        return true;
    }

}
/*
#include <iostream>
#include <string>
#include <vector>
#include <xlsxwriter.h>

// 函数：将表格数据输出到Excel文件
void writeTableToExcel(const std::string* headers, const std::string* columnTitles, int rows, int cols, const double* data, const std::string& filename) {
    // 创建一个新的Excel文件
    lxw_workbook* workbook = workbook_new(filename.c_str());
    lxw_worksheet* worksheet = workbook_add_worksheet(workbook, NULL);

    // 写入表头
    for (int col = 0; col < cols; ++col) {
        worksheet_write_string(worksheet, 0, col, headers[col].c_str(), NULL);
    }

    // 写入每一列的标题
    for (int row = 0; row < rows; ++row) {
        worksheet_write_string(worksheet, row + 1, 0, columnTitles[row].c_str(), NULL);
    }

    // 写入数据
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            worksheet_write_number(worksheet, row + 1, col + 1, data[row * cols + col], NULL);
        }
    }

    // 关闭工作簿
    workbook_close(workbook);
}

int main() {
    // 表头
    std::string headers[] = {"Header1", "Header2", "Header3"};
    int cols = sizeof(headers) / sizeof(headers[0]);

    // 每一列的标题
    std::string columnTitles[] = {"Row1", "Row2", "Row3", "Row4"};
    int rows = sizeof(columnTitles) / sizeof(columnTitles[0]);

    // 表格数据
    double data[] = {
        1.1, 2.1, 3.1,
        1.2, 2.2, 3.2,
        1.3, 2.3, 3.3,
        1.4, 2.4, 3.4
    };

    // 输出文件名
    std::string filename = "output.xlsx";

    // 调用函数将表格数据输出到Excel文件
    writeTableToExcel(headers, columnTitles, rows, cols, data, filename);

    std::cout << "Table has been written to " << filename << std::endl;

    return 0;
}
*/