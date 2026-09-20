//   独立 TU 的原因:stb 的 *_IMPLEMENTATION 只能在**一个**编译单元展开
//   (避免实现宏经公共头扩散到其他翻译单元)。

// SPDX-License-Identifier: BSD-3-Clause

#define STB_TRUETYPE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_truetype.h"
#include "stb_image_write.h"

#include "out-png.h"
#include <algorithm>
#include <map>

namespace FileOut
{
    namespace
    {
        // ---------- 像素缓冲(RGB,白底) ----------
        struct Raster
        {
            int W = 0, H = 0;
            std::vector<unsigned char> px;      // RGB
            Raster(int w, int h) : W(w), H(h), px((size_t)w * h * 3, 255) {}

            void blend(int x, int y, double r, double g, double b, double a)
            {
                if (x < 0 || y < 0 || x >= W || y >= H || a <= 0) return;
                if (a > 1) a = 1;
                size_t i = ((size_t)y * W + x) * 3;
                px[i + 0] = (unsigned char)std::lround(px[i + 0] * (1 - a) + r * a);
                px[i + 1] = (unsigned char)std::lround(px[i + 1] * (1 - a) + g * a);
                px[i + 2] = (unsigned char)std::lround(px[i + 2] * (1 - a) + b * a);
            }
        };

        struct RGB { double r = 0, g = 0, b = 0; };

        // "#RRGGBB" → RGB;非法/空 → 黑
        RGB parseColor(const std::string& c)
        {
            RGB o;
            if (c.size() >= 7 && c[0] == '#')
            {
                auto hx = [&](int i) { return (double)std::stoi(c.substr(i, 2), nullptr, 16); };
                try { o.r = hx(1); o.g = hx(3); o.b = hx(5); } catch (...) { o = RGB(); }
            }
            return o;
        }

        // 点到线段的距离 —— 线宽 AA 的基础
        double distToSeg(double px, double py, double x1, double y1, double x2, double y2)
        {
            double dx = x2 - x1, dy = y2 - y1;
            double len2 = dx * dx + dy * dy;
            double t = (len2 > 0) ? ((px - x1) * dx + (py - y1) * dy) / len2 : 0.0;
            t = (std::max)(0.0, (std::min)(1.0, t));
            double qx = x1 + t * dx, qy = y1 + t * dy;
            return std::sqrt((px - qx) * (px - qx) + (py - qy) * (py - qy));
        }

        // 抗锯齿线段:按到中心线的距离算覆盖率
        void drawLine(Raster& R, double x1, double y1, double x2, double y2,
                      double w, const RGB& c, double alpha = 1.0)
        {
            double half = (std::max)(w, 0.6) * 0.5;
            int x0 = (int)std::floor((std::min)(x1, x2) - half - 1);
            int xe = (int)std::ceil ((std::max)(x1, x2) + half + 1);
            int y0 = (int)std::floor((std::min)(y1, y2) - half - 1);
            int ye = (int)std::ceil ((std::max)(y1, y2) + half + 1);
            for (int y = y0; y <= ye; y++)
                for (int x = x0; x <= xe; x++)
                {
                    double d = distToSeg(x + 0.5, y + 0.5, x1, y1, x2, y2);
                    double cov = half + 0.5 - d;            // 覆盖率线性近似
                    if (cov > 0) R.blend(x, y, c.r, c.g, c.b, (std::min)(cov, 1.0) * alpha);
                }
        }

        void fillRect(Raster& R, double x, double y, double w, double h, const RGB& c, double alpha)
        {
            int x0 = (int)std::floor(x), xe = (int)std::ceil(x + w);
            int y0 = (int)std::floor(y), ye = (int)std::ceil(y + h);
            for (int py = y0; py < ye; py++)
                for (int px = x0; px < xe; px++)
                {
                    // 边缘按覆盖面积做 AA
                    double cx = (std::min)((double)px + 1, x + w) - (std::max)((double)px, x);
                    double cy = (std::min)((double)py + 1, y + h) - (std::max)((double)py, y);
                    double cov = (std::max)(0.0, cx) * (std::max)(0.0, cy);
                    if (cov > 0) R.blend(px, py, c.r, c.g, c.b, cov * alpha);
                }
        }

        void fillCircle(Raster& R, double cx, double cy, double r, const RGB& c, double alpha)
        {
            int x0 = (int)std::floor(cx - r - 1), xe = (int)std::ceil(cx + r + 1);
            int y0 = (int)std::floor(cy - r - 1), ye = (int)std::ceil(cy + r + 1);
            for (int y = y0; y <= ye; y++)
                for (int x = x0; x <= xe; x++)
                {
                    double dx = x + 0.5 - cx, dy = y + 0.5 - cy;
                    double cov = r + 0.5 - std::sqrt(dx * dx + dy * dy);
                    if (cov > 0) R.blend(x, y, c.r, c.g, c.b, (std::min)(cov, 1.0) * alpha);
                }
        }

        // ---------- UTF-8 → 码点 ----------
        std::vector<unsigned> utf8ToCodepoints(const std::string& s)
        {
            std::vector<unsigned> v; size_t i = 0;
            while (i < s.size())
            {
                unsigned char c = (unsigned char)s[i];
                unsigned cp = c; int n = 0;
                if      (c < 0x80) { cp = c;          n = 0; }
                else if (c < 0xE0) { cp = c & 0x1F;   n = 1; }
                else if (c < 0xF0) { cp = c & 0x0F;   n = 2; }
                else               { cp = c & 0x07;   n = 3; }
                for (int k = 1; k <= n && i + k < s.size(); k++)
                    cp = (cp << 6) | ((unsigned char)s[i + k] & 0x3F);
                v.push_back(cp);
                i += n + 1;
            }
            return v;
        }

        // ---------- 字体 ----------
        struct Face
        {
            std::vector<unsigned char> data;
            stbtt_fontinfo info;
            bool ok = false;

            bool load(const std::string& path, int index)
            {
                std::ifstream f(path, std::ios::binary);
                if (!f.is_open()) return false;
                data.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
                if (data.empty()) return false;
                int off = stbtt_GetFontOffsetForIndex(data.data(), index);   // TTC 取子字体
                if (off < 0) return false;
                ok = stbtt_InitFont(&info, data.data(), off) != 0;
                return ok;
            }
        };

        // 一段文本的宽度(用于 anchor 定位);缺字形按 0 计
        double textWidthPx(Face& latin, Face& cjk, const std::vector<unsigned>& cps, double px)
        {
            double w = 0;
            for (size_t i = 0; i < cps.size(); i++)
            {
                Face& F = (cps[i] < 0x80) ? latin : cjk;
                if (!F.ok) continue;
                double sc = stbtt_ScaleForPixelHeight(&F.info, (float)px);
                int adv = 0, lsb = 0;
                stbtt_GetCodepointHMetrics(&F.info, (int)cps[i], &adv, &lsb);
                w += adv * sc;
            }
            return w;
        }

        void drawText(Raster& R, Face& latin, Face& cjk, const DrawOp& op, double scale)
        {
            std::vector<unsigned> cps = utf8ToCodepoints(op.text);
            if (cps.empty()) return;
            double px = op.size * scale;
            RGB c = parseColor(op.fill);

            double w = textWidthPx(latin, cjk, cps, px);
            double x = op.x1 * scale;
            if (op.anchorPos == "middle") x -= w / 2;
            else if (op.anchorPos == "end") x -= w;
            double baseline = op.y1 * scale;      // SVG 的 y 即基线

            for (size_t i = 0; i < cps.size(); i++)
            {
                Face& F = (cps[i] < 0x80) ? latin : cjk;
                if (!F.ok) continue;
                double sc = stbtt_ScaleForPixelHeight(&F.info, (float)px);
                int gw = 0, gh = 0, gx = 0, gy = 0;
                unsigned char* bmp = stbtt_GetCodepointBitmap(&F.info, 0, (float)sc,
                                                              (int)cps[i], &gw, &gh, &gx, &gy);
                if (bmp)
                {
                    for (int yy = 0; yy < gh; yy++)
                        for (int xx = 0; xx < gw; xx++)
                        {
                            double a = bmp[yy * gw + xx] / 255.0;
                            if (a > 0)
                                R.blend((int)std::lround(x) + gx + xx,
                                        (int)std::lround(baseline) + gy + yy, c.r, c.g, c.b, a);
                        }
                    stbtt_FreeBitmap(bmp, nullptr);
                }
                int adv = 0, lsb = 0;
                stbtt_GetCodepointHMetrics(&F.info, (int)cps[i], &adv, &lsb);
                x += adv * sc;
                // 伪加粗:再描一次,横向偏移 ~1px(与 SVG 的 font-weight='bold' 视觉近似)
                if (op.bold) { /* 见下:整串重描,避免逐字偏移累积 */ }
            }
            if (op.bold)      // 粗体:整串再画一遍,右移半像素
            {
                DrawOp o2 = op; o2.bold = false;
                o2.x1 = op.x1 + 0.4 / scale;
                drawText(R, latin, cjk, o2, scale);
            }
        }
    }

    PngFontConfig& currentPngFont()
    {
        static PngFontConfig cfg;
        return cfg;
    }

    namespace
    {
        // 静态初始化时把栅格后端注册进 ChartCanvas —— 之后任何 save("*.png") 自动走这里。
        struct PngBackendRegistrar
        {
            PngBackendRegistrar()
            {
                ChartCanvas::pngWriter() = [](const ChartCanvas& c, const std::string& p) {
                    return writeCanvasToPng(c, p, currentPngFont());
                };
            }
        };
        PngBackendRegistrar g_registrar;
    }

    std::string writeCanvasToPng(const ChartCanvas& canvas, const std::string& path,
                                 const PngFontConfig& font)
    {
        double sc = (font.scale > 0.1) ? font.scale : 1.0;
        int W = (int)std::ceil(canvas.width()  * sc);
        int H = (int)std::ceil(canvas.height() * sc);
        if (W <= 0 || H <= 0) return "canvas has zero size";

        Face latin, cjk;
        bool okL = latin.load(font.latin, 0);
        bool okC = cjk.load(font.cjk, font.cjkIndex);
        // 字体缺失【明确报错】—— 不默默产出无字图(那正是 nanosvg 方案被否的理由)
        if (!okL && !okC)
            return "no usable font: neither '" + font.latin + "' nor '" + font.cjk
                 + "' could be loaded; PNG output needs system fonts (see manual: platform notes)";

        Raster R(W, H);
        const std::vector<DrawOp>& ops = canvas.ops();
        std::string warn;
        for (size_t i = 0; i < ops.size(); i++)
        {
            const DrawOp& o = ops[i];
            switch (o.kind)
            {
            case DrawOp::Line:
                drawLine(R, o.x1 * sc, o.y1 * sc, o.x2 * sc, o.y2 * sc,
                         (std::max)(o.w * sc, 1.0), parseColor(o.stroke));
                break;
            case DrawOp::Rect:
            {
                if (!o.fill.empty() && o.fill != "none")
                    fillRect(R, o.x1 * sc, o.y1 * sc, o.x2 * sc, o.y2 * sc, parseColor(o.fill), o.opacity);
                if (!o.stroke.empty() && o.stroke != "none" && o.w > 0)
                {
                    RGB s = parseColor(o.stroke); double lw = (std::max)(o.w * sc, 1.0);
                    double x = o.x1 * sc, y = o.y1 * sc, w = o.x2 * sc, h = o.y2 * sc;
                    drawLine(R, x, y, x + w, y, lw, s);
                    drawLine(R, x + w, y, x + w, y + h, lw, s);
                    drawLine(R, x + w, y + h, x, y + h, lw, s);
                    drawLine(R, x, y + h, x, y, lw, s);
                }
                break;
            }
            case DrawOp::Circle:
                fillCircle(R, o.x1 * sc, o.y1 * sc, o.x2 * sc, parseColor(o.fill), o.opacity);
                break;
            case DrawOp::Polyline:
                for (size_t k = 1; k < o.pts.size(); k++)
                    drawLine(R, o.pts[k - 1].first * sc, o.pts[k - 1].second * sc,
                             o.pts[k].first * sc, o.pts[k].second * sc,
                             (std::max)(o.w * sc, 1.0), parseColor(o.stroke));
                break;
            case DrawOp::Text:
            {
                // 缺哪一侧字体就在这一侧提示(EN/ZH 图各自可能受影响)
                std::vector<unsigned> cps = utf8ToCodepoints(o.text);
                for (size_t k = 0; k < cps.size(); k++)
                {
                    if (cps[k] < 0x80 && !latin.ok && warn.empty())
                        warn = "latin font '" + font.latin + "' missing: ASCII text not rendered";
                    if (cps[k] >= 0x80 && !cjk.ok && warn.empty())
                        warn = "CJK font '" + font.cjk + "' missing: non-ASCII text not rendered";
                }
                drawText(R, latin, cjk, o, sc);
                break;
            }
            }
        }

        if (!stbi_write_png(path.c_str(), W, H, 3, R.px.data(), W * 3))
            return "failed to write " + path;
        return warn;
    }
}
