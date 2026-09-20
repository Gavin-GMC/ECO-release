//------------------------Description------------------------
// PNG(位图)图表输出。把 ChartCanvas 的【图元清单】重放到像素缓冲,再经 stb_image_write 落 PNG。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <string>
#include <vector>
#include <cmath>
#include <cstdio>
#include <fstream>
#include "out-chart.h"

namespace FileOut
{
    // 字体配置:英文/中文各一,按码点分派(< 0x80 走 latin,其余走 cjk)
    struct PngFontConfig
    {
        std::string latin = "C:/Windows/Fonts/times.ttf";    // Times New Roman(用户定)
        std::string cjk   = "C:/Windows/Fonts/simsun.ttc";   // 宋体(用户定);TTC → 取 index 0
        int         cjkIndex = 0;
        double      scale = 2.0;   // 超采样倍率:PNG 像素 = SVG 单位 × scale(2 倍即"2x 图")
    };

    // 把 ChartCanvas 的图元清单栅格成 PNG。返回空串 = 成功,否则为错误信息。
    std::string writeCanvasToPng(const ChartCanvas& canvas, const std::string& path,
                                 const PngFontConfig& font);

    // 当前生效的字体配置。ChartCanvas 的钩子签名不带字体参数(它不该知道字体这回事),
    //   故 analyzer 出图前经此设置。单线程分析流程下安全。
    PngFontConfig& currentPngFont();
}
