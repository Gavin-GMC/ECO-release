
#pragma once
#include<string>

namespace FileOut
{
	class FilePNG
	{
	public:

		void plot(double* data, int number);

		void line(double* data, int number);

		void bar(double* data, int number);
	};
}

/*
#include <cairo/cairo.h>
#include <iostream>
#include <string>
#include <vector>

// 函数：将表格数据输出为PNG图片
void writeTableToPNG(const std::string* headers, const std::string* columnTitles, int rows, int cols, const double* data, const std::string& filename) {
    // 定义图片尺寸
    int width = 800;
    int height = 600;
    int cellWidth = width / (cols + 1);
    int cellHeight = height / (rows + 1);

    // 创建Cairo表面
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t* cr = cairo_create(surface);

    // 设置背景颜色
    cairo_set_source_rgb(cr, 1, 1, 1); // 白色
    cairo_paint(cr);

    // 绘制表格线
    cairo_set_source_rgb(cr, 0, 0, 0); // 黑色
    cairo_set_line_width(cr, 1.0);

    for (int i = 0; i <= rows + 1; ++i) {
        cairo_move_to(cr, 0, i * cellHeight);
        cairo_line_to(cr, width, i * cellHeight);
    }
    for (int i = 0; i <= cols + 1; ++i) {
        cairo_move_to(cr, i * cellWidth, 0);
        cairo_line_to(cr, i * cellWidth, height);
    }
    cairo_stroke(cr);

    // 绘制表头
    for (int col = 0; col < cols; ++col) {
        cairo_move_to(cr, (col + 1) * cellWidth + 10, cellHeight / 2 + 20);
        cairo_show_text(cr, headers[col].c_str());
    }

    // 绘制每一列的标题和数据
    for (int row = 0; row < rows; ++row) {
        cairo_move_to(cr, 10, (row + 2) * cellHeight - 10);
        cairo_show_text(cr, columnTitles[row].c_str());
        for (int col = 0; col < cols; ++col) {
            cairo_move_to(cr, (col + 1) * cellWidth + 10, (row + 2) * cellHeight - 10);
            cairo_show_text(cr, std::to_string(data[row * cols + col]).c_str());
        }
    }

    // 保存图片
    cairo_surface_write_to_png(surface, filename.c_str());

    // 清理
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
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
    std::string filename = "table.png";

    // 调用函数将表格数据输出为PNG图片
    writeTableToPNG(headers, columnTitles, rows, cols, data, filename);

    std::cout << "Table has been written to " << filename << std::endl;

    return 0;
}
*/