
#pragma once
#include<string>

namespace FileOut
{
	class FileWord
	{
	public:

		void table(std::string* data, int height, int length);

	};
}

/*
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <docx/document.h>
#include <docx/paragraph.h>
#include <docx/table.h>
#include <docx/table_row.h>
#include <docx/table_cell.h>
#include <docx/run.h>
#include <docx/text.h>

// 函数：将表格数据输出到Word文件
void writeTableToWord(const std::string* headers, const std::string* columnTitles, int rows, int cols, const double* data, const std::string& filename) {
    // 创建一个新的Word文档
    docx::Document doc;

    // 创建表格
    docx::Table table(rows + 1, cols + 1);

    // 写入表头
    docx::TableRow& headerRow = table.row(0);
    headerRow.cell(0).addParagraph().addRun().addText(" ");
    for (int col = 0; col < cols; ++col) {
        headerRow.cell(col + 1).addParagraph().addRun().addText(headers[col]);
    }

    // 写入每一列的标题和数据
    for (int row = 0; row < rows; ++row) {
        docx::TableRow& dataRow = table.row(row + 1);
        dataRow.cell(0).addParagraph().addRun().addText(columnTitles[row]);
        for (int col = 0; col < cols; ++col) {
            dataRow.cell(col + 1).addParagraph().addRun().addText(std::to_string(data[row * cols + col]));
        }
    }

    // 将表格添加到文档中
    doc.addTable(table);

    // 保存文档
    doc.save(filename);
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
    std::string filename = "output.docx";

    // 调用函数将表格数据输出到Word文件
    writeTableToWord(headers, columnTitles, rows, cols, data, filename);

    std::cout << "Table has been written to " << filename << std::endl;

    return 0;
}
*/