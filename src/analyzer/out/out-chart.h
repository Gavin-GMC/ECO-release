//------------------------Description------------------------
// SVG 图表输出。零第三方依赖——直接写 SVG 文本(同 out-txt.h/out-latex.h 范式)。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>

namespace FileOut
{
    // ---- 内置色表(每表 16 色,超出按 16 循环) ----
    enum class Palette { ColorblindSafe, Categorical, Grayscale, Vibrant };
    // ---- 框架文字语言(算法名不翻译) ----
    enum class Lang { EN, ZH };

    // ---- 输出格式(v2.2.3)。与表格的 `Format` 分立 —— 图表画不出 Txt/Csv。
    //   **经参数显式透传**(不靠文件扩展名嗅探:那把格式意图藏进字符串,不稳定)。
    enum class ChartFormat { Svg, Png };

    // 跨图共享的样式:一批结果设一套 → 色表一致、同对象同色。标题不在此(见各图接口)。
    class ChartStyle
    {
    public:
        ChartStyle& palette(Palette p) { _palette = p;  return *this; }
        ChartStyle& lang(Lang l)       { _lang    = l;  return *this; }
        ChartStyle& caption(bool on)   { _caption = on; return *this; }
        // 对数纵轴:只改【轴刻度】,不动数据 —— 故归 style(展示层),与 ChartSpec::norm(数据变换)分属两侧。
        // 非法组合(norm=MinMax/PooledRank + log)由 analyzer 侧守卫降级,见 ec-analyzer.h。
        ChartStyle& logScale(bool on)  { _log     = on; return *this; }
        // 纵轴反向(顶部 = 轴域下端)。同属"只改轴呈现、不动数据",故与 logScale 并列在此。
        //   默认关 ⇒ 一律"下方=更优",与原始目标值轴(越小越优)方向一致;
        //   开启后适合 norm=PooledRank 等"想把最优摆在顶部"的读法。
        ChartStyle& reverseY(bool on)  { _revy    = on; return *this; }
        Palette getPalette()  const { return _palette; }
        Lang    getLang()     const { return _lang;    }
        bool    hasCaption()  const { return _caption; }
        bool    hasLogScale() const { return _log;     }
        bool    hasReverseY() const { return _revy;    }
    private:
        Palette _palette = Palette::ColorblindSafe;   // 默认:色盲友好(16 色)
        Lang    _lang    = Lang::EN;                  // 默认:英文
        bool    _caption = true;                      // 默认:带下方说明
        bool    _log     = false;                     // 默认:线性轴(用户定)
        bool    _revy    = false;                     // 默认:不反向(用户定)
    };

    // 对象索引 → 颜色(按 16 循环)。同一 objIdx 在任何图里恒得同色 ⇒ 跨图一致。
    inline std::string paletteColor(Palette p, int objIdx)
    {
        // ColorblindSafe: Okabe-Ito(8) + Paul Tol muted(8) = 16,色盲友好
        static const char* CB[16] = {
            "#0072B2","#E69F00","#009E73","#CC79A7","#56B4E9","#D55E00","#F0E442","#000000",
            "#332288","#117733","#88CCEE","#882255","#44AA99","#999933","#AA4499","#DDCC77" };
        static const char* CA[16] = {   // Tableau 风(分类)
            "#4E79A7","#F28E2B","#E15759","#76B7B2","#59A14F","#EDC948","#B07AA1","#FF9DA7",
            "#9C755F","#BAB0AC","#2C4B7C","#C1553B","#3F7D6E","#A0842C","#7B5C8D","#6B6B6B" };
        static const char* GS[16] = {   // 灰度(印刷/黑白论文)
            "#000000","#4D4D4D","#808080","#B3B3B3","#1A1A1A","#666666","#999999","#CCCCCC",
            "#262626","#595959","#8C8C8C","#BFBFBF","#333333","#737373","#A6A6A6","#D9D9D9" };
        static const char* VB[16] = {   // 高饱和
            "#E6194B","#3CB44B","#4363D8","#F58231","#911EB4","#42D4F4","#F032E6","#BFEF45",
            "#FABED4","#469990","#DCBEFF","#9A6324","#800000","#AAFFC3","#808000","#000075" };
        int i = ((objIdx % 16) + 16) % 16;
        switch (p) {
        case Palette::Categorical: return CA[i];
        case Palette::Grayscale:   return GS[i];
        case Palette::Vibrant:     return VB[i];
        default:                   return CB[i];
        }
    }

    // 框架文字 i18n(EN 默认)。key 见各 draw* 用法。
    inline std::string t(Lang l, const std::string& key)
    {
        struct KV { const char* k; const char* en; const char* zh; };
        static const KV T[] = {
            {"rank",        "Average rank",                 "平均秩"},
            {"better",      "better",                       "更优"},
            {"cd",          "CD",                           "临界差 CD"},
            {"cd_caption",  "Thick bar: no significant difference within group (rank gap < CD).",
                            "粗线:组内两两秩差 < CD,即无显著差异。"},
            // 箱线图 / 柱状图族
            {"box_caption", "Box = Q1-Q3, line = median, whiskers = min/max.",
                            "盒 = Q1~Q3,中线 = 中位数,须 = 最小/最大值。"},
            {"box_caption_pt", "Box = Q1-Q3, line = median, whiskers = min/max; dots = individual runs (x-jitter avoids overlap).",
                            "盒 = Q1~Q3,中线 = 中位数,须 = 最小/最大值;圆点 = 每次独立运行的结果(横向抖动仅为避免重叠)。"},
            {"bar_caption", "Bar = selected statistic (from 0), error bar = spread band.",
                            "柱高 = 所选统计量(自 0 起),误差棒 = 离散带。"},
            {"logaxis",     " Log axis: tick spacing only, the data is not transformed.",
                            " 对数轴:仅改刻度间距,数据未做变换。"},
            {"cross_raw",   " Scales differ across problems, so bar/box heights are not comparable across groups.",
                            " 各问题量纲不同,跨组高度不可比。"},
            // 纵轴名(随 ChartSpec::norm)
            {"y_raw",       "Objective (raw)",              "目标值(原始)"},
            {"y_minmax",    "Normalized",                   "组内归一化"},
            {"y_rank",      "Pooled run rank (1 = best)",   "混合运行秩(1=最优)"},
            {"y_ratio",     "Ratio to best (1 = best)",     "相对最优倍数(1=最优)"},
            // 口径说明(随 ChartSpec::norm)
            {"n_none",      " Raw values, no transformation applied.",
                            " 原始数值,未做任何变换。"},
            {"n_minmax",    " Min-max normalized within each problem; compare within a group only.",
                            " 组内按该问题跨算法 min/max 压到 [0,1],只可组内横比。"},
            {"n_rank",      " Runs pooled and ranked within each problem (ties averaged); lower is better.",
                            " 组内把各算法所有运行混合排秩(并列取平均秩),越小越优。"},
            {"n_ratio",     " Divided by the best value of the same problem; lower is better.",
                            " 除以同问题的最优值,越小越优。"},
            // 三线表 / 胜平负堆叠条 / 排名热力图(v2.2.2.1)
            {"table_caption", "Three-line table; bold = best. Same table buffer as the Txt/Csv/LaTeX report.",
                              "三线表;加粗 = 最优。与 Txt/Csv/LaTeX 报表同源于分析器表格缓冲。"},
            {"wtl_caption", "Bar length = number of problems, split into win / tie / lose against the baseline.",
                            "条长 = 问题数,按相对基线的 胜 / 平 / 负 分段。"},
            {"wtl_win",     "win",                          "胜"},
            {"wtl_tie",     "tie",                          "平"},
            {"wtl_lose",    "lose",                         "负"},
            {"heat_caption","Colour is normalised within each row (per problem); the number is the raw value.",
                            "着色按【行内】归一化(每问题自成一档),格内数字为原值。"},
            {"heat_low",    " Darker = better (smaller).",  " 越深越优(值越小)。"},
            {"heat_high",   " Darker = better (larger).",   " 越深越优(值越大)。"},
            {"conv_caption","Best-so-far against evaluation count; each line is the aggregate across runs at that budget.",
                            "历史最优随评估次数的变化;每条线是该预算处跨多次运行的聚合值。"},
            // Pareto 前沿 / 平均秩条(v2.2.2.5)
            {"pf_caption",  "Objective space; each point is one archived solution. The archive already holds the non-dominated set, so no extra sorting is applied.",
                            "目标空间;每个点是档案中的一个解。档案本身即非支配集,未再做任何排序。"},
            {"pf_connect",  " Points of one algorithm are joined in ascending first-objective order -- the line is a reading aid, not an interpolated front.",
                            " 同一算法的点按第一目标升序相连 —— 连线只是阅读辅助,不代表前沿上真实存在这些中间解。"},
            {"pf_runs",     " All runs are overlaid.",       " 各次运行叠加显示。"},
            {"pf_run1",     " A single run is shown.",       " 仅显示单次运行。"},
            {"rb_caption",  "Average rank across problems (smaller is better); bars are sorted best-first.",
                            "跨问题平均秩(越小越优);按最优在上排序。"},
            {"rb_src_rank", " Source: Compare::Rank (per-problem ranks averaged over all problems).",
                            " 口径:Compare::Rank(逐问题名次在全部问题上求平均)。"},
            {"rb_src_nem",  " Source: Compare::Nemenyi (Friedman block ranks; only problems where every algorithm has data are included).",
                            " 口径:Compare::Nemenyi(Friedman 块内秩;仅纳入所有算法都有数据的问题)。"},
            {"rb_axis",     "Average rank",                  "平均秩"},
        };
        for (const auto& kv : T) if (key == kv.k) return (l == Lang::ZH) ? kv.zh : kv.en;
        return key;
    }

    // ---- 图元清单(display list,v2.2.3)----
    //   ChartCanvas 在写 SVG 文本的【同时】把每个图元记一份。出 PNG 时把同一份清单重放到
    //   像素缓冲 ⇒ **一份绘图源、两个后端,无需 SVG 解析器**。
    //   (原 TODO 拟用 nanosvg 解析自家 SVG 再栅格,但 NanoSVG 不支持 <text>,
    //    而我们的图 text 元素占比极高 → 那条路产出的是无字废图。)
    struct DrawOp
    {
        enum Kind { Line, Rect, Circle, Polyline, Text } kind = Line;
        double x1 = 0, y1 = 0, x2 = 0, y2 = 0;   // Line:两端 / Rect:左上+宽高 / Circle:圆心+半径(x2)
        double w = 1;                            // 线宽 / 描边宽
        double opacity = 1.0;
        std::string fill, stroke;
        std::vector<std::pair<double, double>> pts;   // Polyline
        std::string text; double size = 13; std::string anchorPos = "start"; bool bold = false;
    };

    // ---- SVG 画布:图元 + 包围盒累计 + 超界检查(需求⑤) ----
    class ChartCanvas
    {
    public:
        ChartCanvas(double w, double h) : _W(w), _H(h) {}

        void line(double x1, double y1, double x2, double y2, const std::string& col = "#333",
                  double w = 1, const std::string& cap = "butt")
        {
            _o << "<line x1='" << x1 << "' y1='" << y1 << "' x2='" << x2 << "' y2='" << y2
               << "' stroke='" << col << "' stroke-width='" << w << "' stroke-linecap='" << cap << "'/>\n";
            _track(x1 - w, y1 - w); _track(x2 + w, y2 + w);
            { DrawOp o; o.kind = DrawOp::Line; o.x1=x1; o.y1=y1; o.x2=x2; o.y2=y2; o.w=w; o.stroke=col; _ops.push_back(o); }
        }
        // opacity<1 时显式写 fill-opacity(不用 8 位 hex #RRGGBBAA——那是 SVG2 写法,SVG1.1 查看器不认)
        void rect(double x, double y, double w, double h, const std::string& fill,
                  const std::string& stroke = "#333", double sw = 1, double opacity = 1.0)
        {
            _o << "<rect x='" << x << "' y='" << y << "' width='" << w << "' height='" << h
               << "' fill='" << fill << "'" << _op(opacity)
               << " stroke='" << stroke << "' stroke-width='" << sw << "'/>\n";
            _track(x - sw, y - sw); _track(x + w + sw, y + h + sw);
            { DrawOp o; o.kind = DrawOp::Rect; o.x1=x; o.y1=y; o.x2=w; o.y2=h; o.w=sw;
              o.fill=fill; o.stroke=stroke; o.opacity=opacity; _ops.push_back(o); }
        }
        void circle(double cx, double cy, double r, const std::string& fill, double opacity = 1.0)
        {
            _o << "<circle cx='" << cx << "' cy='" << cy << "' r='" << r << "' fill='" << fill << "'"
               << _op(opacity) << "/>\n";
            { DrawOp o; o.kind = DrawOp::Circle; o.x1=cx; o.y1=cy; o.x2=r;
              o.fill=fill; o.opacity=opacity; _ops.push_back(o); }
            _track(cx - r, cy - r); _track(cx + r, cy + r);
        }
        void polyline(const std::vector<std::pair<double, double>>& pts, const std::string& col, double w = 2)
        {
            _o << "<polyline fill='none' stroke='" << col << "' stroke-width='" << w << "' points='";
            for (auto& p : pts) { _o << p.first << "," << p.second << " "; _track(p.first, p.second); }
            _o << "'/>\n";
            { DrawOp o; o.kind = DrawOp::Polyline; o.pts = pts; o.w=w; o.stroke=col; _ops.push_back(o); }
        }
        // anchor: start / middle / end;bold=true 走真 font-weight(不再靠偏移重描伪加粗)
        void text(double x, double y, const std::string& s, double size = 13,
                  const std::string& col = "#222", const std::string& anchor = "start",
                  bool bold = false)
        {
            _o << "<text x='" << x << "' y='" << y << "' font-size='" << size << "' fill='" << col
               << "' text-anchor='" << anchor << "'"
               << (bold ? " font-weight='bold'" : "") << ">" << _esc(s) << "</text>\n";
            double w = _textWidth(s, size) * (bold ? 1.06 : 1.0), h = size;   // 粗体略宽,超界检查取保守值
            double x0 = (anchor == "middle") ? x - w / 2 : (anchor == "end") ? x - w : x;
            _track(x0, y - h); _track(x0 + w, y + h * 0.3);
            { DrawOp o; o.kind = DrawOp::Text; o.x1=x; o.y1=y; o.text=s; o.size=size;
              o.fill=col; o.anchorPos=anchor; o.bold=bold; _ops.push_back(o); }
        }

        // 需求⑤:内容是否超出画布
        bool overflow() const
        { return _has && (_minx < -0.5 || _miny < -0.5 || _maxx > _W + 0.5 || _maxy > _H + 0.5); }
        std::string overflowInfo() const
        {
            if (!overflow()) return "";
            std::ostringstream s;
            s << "content bbox [" << _minx << "," << _miny << "]-[" << _maxx << "," << _maxy
              << "] exceeds canvas " << _W << "x" << _H;
            return s.str();
        }

        std::string str() const
        {
            std::ostringstream s;
            s << "<svg xmlns='http://www.w3.org/2000/svg' width='" << _W << "' height='" << _H
              << "' viewBox='0 0 " << _W << " " << _H << "' font-family='Segoe UI,Arial,sans-serif'>\n"
              << "<rect width='100%' height='100%' fill='white'/>\n" << _o.str() << "</svg>\n";
            return s.str();
        }
        // v2.2.3:PNG 后端钩子。out-png.cpp 在静态初始化时注册;未链接 PNG 后端时为空。
        //   如此 11 个 draw*() 只需透传格式,不必各自知道栅格细节。
        typedef std::string (*PngWriterFn)(const ChartCanvas&, const std::string&);
        static PngWriterFn& pngWriter() { static PngWriterFn fn = nullptr; return fn; }

        // 落盘;返回告警(空=正常)。格式【由参数显式给定】,不从路径推断。
        std::string save(const std::string& path, ChartFormat fmt = ChartFormat::Svg) const
        {
            if (fmt == ChartFormat::Png)
            {
                if (!pngWriter()) return "PNG backend not linked (out-png.cpp missing from build)";
                std::string w = pngWriter()(*this, path);
                return w.empty() ? overflowInfo() : w;
            }
            std::ofstream f(path); f << str(); return overflowInfo();
        }

        // v2.2.3:供 PNG 后端读取(尺寸 + 图元清单)
        const std::vector<DrawOp>& ops() const { return _ops; }
        double width()  const { return _W; }
        double height() const { return _H; }

    private:
        std::ostringstream _o; double _W, _H;
        std::vector<DrawOp> _ops;               // v2.2.3:图元清单,供 PNG 后端重放
        bool _has = false; double _minx = 0, _miny = 0, _maxx = 0, _maxy = 0;
        void _track(double x, double y)
        {
            if (!_has) { _minx = _maxx = x; _miny = _maxy = y; _has = true; return; }
            _minx = (std::min)(_minx, x); _maxx = (std::max)(_maxx, x);
            _miny = (std::min)(_miny, y); _maxy = (std::max)(_maxy, y);
        }
        // 估算文本宽度:CJK 约 1.0×字号,拉丁约 0.55×字号(用于包围盒/超界检查)
        static double _textWidth(const std::string& s, double size)
        {
            double w = 0; size_t i = 0;
            while (i < s.size()) {
                unsigned char c = (unsigned char)s[i];
                if (c < 0x80)      { w += 0.55; i += 1; }
                else if (c < 0xE0) { w += 0.55; i += 2; }
                else if (c < 0xF0) { w += 1.00; i += 3; }   // CJK 多在此段
                else               { w += 1.00; i += 4; }
            }
            return w * size;
        }
        // 透明度属性:1.0 时不输出(保持产物简洁),否则写 SVG1.1 标准的 fill-opacity
        static std::string _op(double o)
        {
            if (o >= 1.0) return "";
            if (o < 0) o = 0;
            std::ostringstream s; s << " fill-opacity='" << o << "'";
            return s.str();
        }
        static std::string _esc(const std::string& s)
        {
            std::string r; r.reserve(s.size());
            for (char c : s) {
                if (c == '&') r += "&amp;"; else if (c == '<') r += "&lt;";
                else if (c == '>') r += "&gt;"; else r += c;
            }
            return r;
        }
    };

    // =====================================================================
    //  箱线图 / 柱状图族(共享分组骨架)
    //  【边界】本层【不做任何统计与变换】—— 收到的 BoxCell/BarCell 已是 analyzer
    //  算好的数(含按 ChartSpec::norm 的变换,落 analyzer 的 _chart_scratch)。
    //  单问题图 = groups.size()==1(此时以算法名标注横轴、不画图例);
    //  跨问题图 = groups.size()>1(横轴标问题、顶部画算法图例)。
    // =====================================================================

    struct BoxCell            // 一个(问题,算法)格的五数 + 可选逐 run 散点
    {
        double lo = 0, q1 = 0, med = 0, q3 = 0, hi = 0;
        std::vector<double> points;      // 空 = 不画散点层
    };
    struct BarCell            // 一个(问题,算法)格的柱高 + 可选误差棒
    {
        double value = 0;
        double lo = std::numeric_limits<double>::quiet_NaN();   // NaN = 无误差棒
        double hi = std::numeric_limits<double>::quiet_NaN();
    };

    namespace detail
    {
        // 分组图共享骨架:算图幅、画标题/图例/纵轴/网格/分组分隔与标签。
        // 回填绘图区参数供调用方摆放格子。
        struct Frame
        {
            double W = 0, H = 0, L = 0, R = 0, T = 0, B = 0;
            double gslot = 0, cslot = 0, cw = 0;   // 组宽 / 组内单元宽 / 单元实绘宽
            bool   log = false, rev = false;
            double dlo = 0, dhi = 1;
            double y(double v) const
            {
                double f = log
                    ? (std::log10((std::max)(v, dlo)) - std::log10(dlo)) / (std::log10(dhi) - std::log10(dlo))
                    : (v - dlo) / (dhi - dlo);
                return rev ? T + f * (B - T) : B - f * (B - T);   // rev:轴域下端在顶部
            }
            double baseY() const { return y(dlo); }               // 柱的基线(反向时在顶部)
            double cx(int g, int c) const { return L + gslot * g + gslot * 0.11 + cslot * (c + 0.5); }
        };

        inline double textWidth(const std::string& s, double size)
        {
            double w = 0; size_t i = 0;
            while (i < s.size()) {
                unsigned char c = (unsigned char)s[i];
                if (c < 0x80)      { w += 0.55; i += 1; }
                else if (c < 0xE0) { w += 0.55; i += 2; }
                else if (c < 0xF0) { w += 1.00; i += 3; }
                else               { w += 1.00; i += 4; }
            }
            return w * size;
        }
        inline std::string fmtNum(double v)
        {
            char b[32];
            std::snprintf(b, 32, (std::fabs(v) >= 1000 || (v != 0 && std::fabs(v) < 0.01)) ? "%.2e" : "%.3g", v);
            return b;
        }

        // ① 只算尺寸/值域,不画 —— 画布必须先知道自己多大(超界检查才有意义)
        inline Frame computeFrame(const std::vector<std::string>& algos,
                                  const std::vector<std::string>& groups,
                                  double vlo, double vhi, bool zeroBased,
                                  const std::string& caption, const ChartStyle& style)
        {
            int k = (int)algos.size(), ng = (int)groups.size();
            bool multi = ng > 1;
            Frame f;
            f.L = multi ? 74.0 : 78.0;
            double capW = style.hasCaption() ? textWidth(caption, 10.5) : 0;
            f.W = (std::max)(multi ? 880.0 : 760.0, f.L + capW + 26);
            f.T = multi ? 78.0 : 62.0;
            f.B = 330;
            f.H = f.B + 70 + (style.hasCaption() ? 26 : 0);
            f.R = f.W - 26;

            f.rev = style.hasReverseY();
            f.log = style.hasLogScale() && vlo > 0;
            if (f.log) { f.dlo = vlo; f.dhi = vhi > vlo ? vhi : vlo * 10; }
            else {
                double lo = zeroBased ? (std::min)(0.0, vlo) : vlo, hi = vhi;
                double pad = (hi - lo) * (zeroBased ? 0.12 : 0.07);
                if (pad <= 0) pad = (std::fabs(hi) > 0 ? std::fabs(hi) : 1.0) * 0.1;
                f.dlo = zeroBased ? lo : lo - pad;
                f.dhi = hi + pad;
            }

            f.gslot = (f.R - f.L) / ng;
            f.cslot = multi ? f.gslot * 0.78 / k : f.gslot / k;
            f.cw    = multi ? (std::min)(30.0, f.cslot * 0.64) : (std::min)(54.0, f.cslot * 0.48);
            return f;
        }

        // ② 按算好的 Frame 画骨架:标题/图例/纵轴/网格/分组分隔与标签/说明
        inline void drawFrame(ChartCanvas& c, const Frame& f,
                              const std::vector<std::string>& algos,
                              const std::vector<std::string>& groups,
                              const std::string& yLabel, const std::string& caption,
                              const std::string& title, const ChartStyle& style)
        {
            int k = (int)algos.size(), ng = (int)groups.size();
            bool multi = ng > 1;
            bool zeroBased = !f.log && f.dlo <= 0;

            c.text(f.W / 2, multi ? 28 : 30, title, 15, "#111", "middle");

            if (multi) {                                    // 跨问题图:顶部算法图例
                double lx = f.L;
                for (int i = 0; i < k; i++) {
                    std::string col = paletteColor(style.getPalette(), i);
                    c.rect(lx, 42, 12, 12, col, col, 1.4, 0.42);
                    c.text(lx + 17, 52, algos[i], 11.5, "#333", "start");
                    lx += 17 + textWidth(algos[i], 11.5) + 20;
                }
            }
            for (int i = 0; i <= 4; i++) {                  // 纵轴刻度 + 淡网格
                double v = f.log
                    ? std::pow(10, std::log10(f.dlo) + (std::log10(f.dhi) - std::log10(f.dlo)) * i / 4.0)
                    : f.dlo + (f.dhi - f.dlo) * i / 4.0;
                double y = f.y(v);
                c.line(f.L, y, f.R, y, "#eee", 1);
                c.line(f.L - 5, y, f.L, y, "#333", 1);
                c.text(f.L - 9, y + 4, fmtNum(v), 11, "#444", "end");
            }
            c.line(f.L, f.T, f.L, f.B, "#333", 1.2);
            c.line(f.L, f.B, f.R, f.B, "#333", zeroBased ? 1.4 : 1.2);
            c.text(6, f.T - 13, yLabel, 12, "#444", "start");   // 横排,避免 middle 锚点左溢

            for (int g = 0; g < ng; g++) {
                if (g) c.line(f.L + f.gslot * g, f.T, f.L + f.gslot * g, f.B, "#e8e8e8", 1);
                if (multi) c.text(f.L + f.gslot * (g + 0.5), f.B + 20, groups[g], 12, "#222", "middle");
            }
            if (!multi) for (int i = 0; i < k; i++)          // 单问题图:横轴标算法名
                c.text(f.cx(0, i), f.B + 19, algos[i], 12, "#222", "middle");

            if (style.hasCaption()) c.text(f.L, f.H - 12, caption, 10.5, "#666", "start");
            (void)zeroBased;
        }

        // 只画纵轴框架(标题/刻度/网格/轴线/轴名/说明)——横轴是连续量的图(如收敛曲线)用,
        // 不画分组分隔与分组标签,横轴刻度与图例由调用方自绘。
        inline void drawFrameAxisOnly(ChartCanvas& c, const Frame& f, const std::string& yLabel,
                                      const std::string& caption, const std::string& title,
                                      const ChartStyle& style)
        {
            c.text(f.W / 2, 30, title, 15, "#111", "middle");
            for (int i = 0; i <= 4; i++)
            {
                double v = f.log
                    ? std::pow(10, std::log10(f.dlo) + (std::log10(f.dhi) - std::log10(f.dlo)) * i / 4.0)
                    : f.dlo + (f.dhi - f.dlo) * i / 4.0;
                double y = f.y(v);
                c.line(f.L, y, f.R, y, "#eee", 1);
                c.line(f.L - 5, y, f.L, y, "#333", 1);
                c.text(f.L - 9, y + 4, fmtNum(v), 11, "#444", "end");
            }
            c.line(f.L, f.T, f.L, f.B, "#333", 1.2);
            c.line(f.L, f.B, f.R, f.B, "#333", 1.2);
            c.text(6, f.T - 13, yLabel, 12, "#444", "start");
            if (style.hasCaption()) c.text(f.L, f.H - 12, caption, 10.5, "#666", "start");
        }
    }

    // 箱线图族。data[组][算法];caption 由 analyzer 组装(口径说明随 norm 变)。返回超界告警(空=正常)。
    inline std::string drawBoxChart(const std::vector<std::string>& algos,
                                    const std::vector<std::string>& groups,
                                    const std::vector<std::vector<BoxCell>>& data,
                                    const std::string& yLabel, const std::string& caption,
                                    const std::string& title, const std::string& path,
                                    const ChartStyle& style = ChartStyle(), ChartFormat fmt = ChartFormat::Svg)
    {
        int k = (int)algos.size(), ng = (int)groups.size();
        if (k == 0 || ng == 0) return "empty chart data";

        double vlo = 1e300, vhi = -1e300;
        for (auto& row : data) for (auto& b : row) {
            vlo = (std::min)(vlo, b.lo); vhi = (std::max)(vhi, b.hi);
            for (double p : b.points) { vlo = (std::min)(vlo, p); vhi = (std::max)(vhi, p); }
        }
        detail::Frame f = detail::computeFrame(algos, groups, vlo, vhi, false, caption, style);
        ChartCanvas cv(f.W, f.H);
        detail::drawFrame(cv, f, algos, groups, yLabel, caption, title, style);

        unsigned long long seed = 88172645463325252ULL;      // 抖动用确定性 RNG(同数据同图)
        auto jit = [&]() { seed ^= seed << 13; seed ^= seed >> 7; seed ^= seed << 17;
                           return ((seed >> 11) & 0xFFFFF) / 1048575.0 - 0.5; };
        for (int g = 0; g < ng; g++)
            for (int i = 0; i < k; i++) {
                const BoxCell& b = data[g][i];
                double cx = f.cx(g, i), bw = f.cw;
                std::string col = paletteColor(style.getPalette(), i);
                cv.line(cx, f.y(b.lo), cx, f.y(b.hi), "#555", 1.2);
                cv.line(cx - bw * 0.27, f.y(b.lo), cx + bw * 0.27, f.y(b.lo), "#555", 1.2);
                cv.line(cx - bw * 0.27, f.y(b.hi), cx + bw * 0.27, f.y(b.hi), "#555", 1.2);
                cv.rect(cx - bw / 2, f.y(b.q3), bw, f.y(b.q1) - f.y(b.q3), col, col, 1.5, 0.28);
                cv.line(cx - bw / 2, f.y(b.med), cx + bw / 2, f.y(b.med), col, 2.4);
                for (double p : b.points) cv.circle(cx + jit() * bw * 0.72, f.y(p), 2.3, col, 0.6);
            }
        return cv.save(path, fmt);
    }

    // 柱状图族。柱自 0 起(zeroBased);lo/hi 为 NaN 时不画误差棒。
    inline std::string drawBarChart(const std::vector<std::string>& algos,
                                    const std::vector<std::string>& groups,
                                    const std::vector<std::vector<BarCell>>& data,
                                    const std::string& yLabel, const std::string& caption,
                                    const std::string& title, const std::string& path,
                                    const ChartStyle& style = ChartStyle(), ChartFormat fmt = ChartFormat::Svg)
    {
        int k = (int)algos.size(), ng = (int)groups.size();
        if (k == 0 || ng == 0) return "empty chart data";

        double vlo = 1e300, vhi = -1e300;
        for (auto& row : data) for (auto& b : row) {
            vlo = (std::min)(vlo, b.value); vhi = (std::max)(vhi, b.value);
            if (!std::isnan(b.lo)) vlo = (std::min)(vlo, b.lo);
            if (!std::isnan(b.hi)) vhi = (std::max)(vhi, b.hi);
        }
        bool zeroBased = !style.hasLogScale();     // 对数轴无法自 0 起
        detail::Frame f = detail::computeFrame(algos, groups, vlo, vhi, zeroBased, caption, style);
        ChartCanvas cv(f.W, f.H);
        detail::drawFrame(cv, f, algos, groups, yLabel, caption, title, style);

        for (int g = 0; g < ng; g++)
            for (int i = 0; i < k; i++) {
                const BarCell& b = data[g][i];
                double cx = f.cx(g, i), bw = f.cw;
                std::string col = paletteColor(style.getPalette(), i);
                // 柱自基线长出;反向轴时基线在顶部(悬挂柱),故按 |value − 基线| 取高
                double top = f.y(b.value), base = f.baseY();
                double h = std::fabs(top - base), y0 = (std::min)(top, base);
                // 零高度柱(如 min-max 口径下该组最优)仍留基线残迹,否则读者看不出它存在
                if (h > 0.5) cv.rect(cx - bw / 2, y0, bw, h, col, col, 1.4, 0.55);
                else         cv.line(cx - bw / 2, base, cx + bw / 2, base, col, 2.6);
                if (!std::isnan(b.lo) && !std::isnan(b.hi)) {
                    cv.line(cx, f.y(b.lo), cx, f.y(b.hi), "#333", 1.4);
                    cv.line(cx - 8, f.y(b.lo), cx + 8, f.y(b.lo), "#333", 1.4);
                    cv.line(cx - 8, f.y(b.hi), cx + 8, f.y(b.hi), "#333", 1.4);
                }
            }
        return cv.save(path, fmt);
    }

    // =====================================================================
    //  语义三色 / 单色渐变
    //   胜平负是【语义】不是【类别】 —— 不能走 paletteColor(那按算法索引取色,
    //   会让"胜"和"负"撞色);热力图编码的是【量级】,需单色渐变而非分类色。
    // =====================================================================
    inline std::string semanticColor(Palette p, int kind)   // 0=胜 1=平 2=负
    {
        static const char* CB[3] = { "#009E73","#999999","#D55E00" };  // Okabe-Ito 绿/灰/朱,色盲可辨
        static const char* CA[3] = { "#59A14F","#BAB0AC","#E15759" };
        static const char* GS[3] = { "#2B2B2B","#8C8C8C","#E0E0E0" };  // 灰度:靠明度分三档(段内另有数字)
        static const char* VB[3] = { "#2CA02C","#AAAAAA","#D62728" };
        int i = kind < 0 ? 0 : (kind > 2 ? 2 : kind);
        switch (p) {
        case Palette::Categorical: return CA[i];
        case Palette::Grayscale:   return GS[i];
        case Palette::Vibrant:     return VB[i];
        default:                   return CB[i];
        }
    }

    // 单色渐变:白 → 该色表首色。t∈[0,1],0=最浅。热力图用。
    inline std::string rampColor(Palette p, double t)
    {
        std::string base = paletteColor(p, 0);
        if (base.size() < 7) base = "#0072B2";
        auto hx = [&](int i) { return std::stoi(base.substr(i, 2), nullptr, 16); };
        int r = hx(1), g = hx(3), b = hx(5);
        if (t < 0) t = 0; if (t > 1) t = 1;
        int R = (int)std::lround(255 + (r - 255) * t);
        int G = (int)std::lround(255 + (g - 255) * t);
        int B = (int)std::lround(255 + (b - 255) * t);
        char buf[16]; std::snprintf(buf, 16, "#%02X%02X%02X", R, G, B);
        return buf;
    }

    // ---- 三线表图片化(booktabs 风格) ----
    //   参数约定与 writeTableToTxt/LaTeX 【完全一致】(同一份 _analysis_table 产物):
    //   headers=算法名(H) / subheaders=统计量名(S) / 总列数=H*S;
    //   rowtitles=行标签;cells[row] 可短于总列数(汇总行按 span 横跨),可为空(空行);
    //   bolds[row]=该行需加粗的【cells 下标】。
    inline std::string drawTableChart(const std::vector<std::string>& headers,
                                      const std::vector<std::string>& subheaders,
                                      const std::vector<std::string>& rowtitles,
                                      const std::vector<std::vector<std::string>>& cells,
                                      const std::vector<std::vector<int>>& bolds,
                                      const std::string& title, const std::string& path,
                                      const ChartStyle& style = ChartStyle(), ChartFormat fmt = ChartFormat::Svg)
    {
        int H = (int)headers.size(), S = (int)subheaders.size();
        int ncol = (S == 0) ? H : H * S;
        int nrow = (int)rowtitles.size();
        if (ncol <= 0 || nrow <= 0) return "empty table data";

        const double fs = 12.0, padx = 12, rowh = 24;
        double labelW = 0;
        for (int r = 0; r < nrow; r++) labelW = (std::max)(labelW, detail::textWidth(rowtitles[r], fs));
        labelW += padx * 2;

        double colW = 0;                                  // 统一列宽:跨列单元按 宽/跨度 折算
        for (int c = 0; c < S; c++) colW = (std::max)(colW, detail::textWidth(subheaders[c], fs));
        for (int c = 0; c < H; c++) colW = (std::max)(colW, detail::textWidth(headers[c], fs) / (S ? S : 1));
        for (int r = 0; r < nrow; r++) {
            if (r >= (int)cells.size() || cells[r].empty()) continue;
            int n = (int)cells[r].size(), span = (n > 0) ? ncol / n : 1; if (span < 1) span = 1;
            for (int c = 0; c < n; c++)
                colW = (std::max)(colW, detail::textWidth(cells[r][c], fs) / span);
        }
        colW += padx * 2;

        std::string caption = style.hasCaption() ? t(style.getLang(), "table_caption") : "";
        double L = 24, T = 52;
        double tw = labelW + colW * ncol;
        double W = L * 2 + (std::max)(tw, style.hasCaption() ? detail::textWidth(caption, 10.5) : 0.0);
        int headRows = (S == 0) ? 1 : 2;
        double H_ = T + rowh * (headRows + nrow) + 24 + (style.hasCaption() ? 24 : 0);
        ChartCanvas c(W, H_);
        c.text(W / 2, 30, title, 15, "#111", "middle");

        double y = T;
        c.line(L, y, L + tw, y, "#111", 2.0);                       // ① 顶线(粗)

        // 表头第一层:算法名居中于其 S 个列之上,下面各画一条 cmidrule
        c.text(L + padx, y + rowh * 0.68, "", fs);                  // 左上角留空
        for (int h = 0; h < H; h++) {
            double x0 = L + labelW + colW * (S ? h * S : h);
            double w  = colW * (S ? S : 1);
            c.text(x0 + w / 2, y + rowh * 0.68, headers[h], fs, "#111", "middle", true);
            if (S) c.line(x0 + 3, y + rowh - 2, x0 + w - 3, y + rowh - 2, "#111", 0.8);
        }
        y += rowh;
        if (S) {                                                    // 表头第二层:统计量名
            for (int h = 0; h < H; h++)
                for (int s = 0; s < S; s++) {
                    double x0 = L + labelW + colW * (h * S + s);
                    c.text(x0 + colW / 2, y + rowh * 0.68, subheaders[s], fs, "#111", "middle", true);
                }
            y += rowh;
        }
        c.line(L, y, L + tw, y, "#111", 1.0);                       // ② 表头线(细)

        for (int r = 0; r < nrow; r++) {
            c.text(L + padx, y + rowh * 0.68, rowtitles[r], fs, "#222", "start");
            if (r < (int)cells.size() && !cells[r].empty()) {
                int n = (int)cells[r].size(), span = ncol / n; if (span < 1) span = 1;
                for (int cc = 0; cc < n; cc++) {
                    double x0 = L + labelW + colW * (double)cc * span;
                    bool bd = false;
                    if (r < (int)bolds.size())
                        for (size_t k = 0; k < bolds[r].size(); k++) if (bolds[r][k] == cc) { bd = true; break; }
                    c.text(x0 + colW * span / 2, y + rowh * 0.68, cells[r][cc], fs,
                           bd ? "#000" : "#222", "middle", bd);
                }
            }
            y += rowh;
        }
        c.line(L, y, L + tw, y, "#111", 2.0);                       // ③ 底线(粗)
        if (style.hasCaption()) c.text(L, H_ - 10, caption, 10.5, "#666", "start");
        return c.save(path, fmt);
    }

    // ---- 胜/平/负 堆叠条 ----
    //   每算法一根横条,三段按占比分宽,段内标数字;颜色走 semanticColor(非算法色)。
    inline std::string drawWinTieLose(const std::vector<std::string>& algos,
                                      const std::vector<int>& win, const std::vector<int>& tie,
                                      const std::vector<int>& lose,
                                      const std::string& title, const std::string& path,
                                      const ChartStyle& style = ChartStyle(), ChartFormat fmt = ChartFormat::Svg)
    {
        int k = (int)algos.size();
        if (k == 0) return "empty win/tie/lose data";
        Lang lg = style.getLang();
        std::string caption = style.hasCaption() ? t(lg, "wtl_caption") : "";
        const char* legend[3] = { "wtl_win", "wtl_tie", "wtl_lose" };

        double labelW = 0;
        for (int i = 0; i < k; i++) labelW = (std::max)(labelW, detail::textWidth(algos[i], 12.0));
        double L = 24 + labelW + 12, T = 76, barh = 26, gap = 12;
        double body = barh * k + gap * (k - 1);
        double W = (std::max)(760.0, L + 320 + 40);
        if (style.hasCaption()) W = (std::max)(W, 24 + detail::textWidth(caption, 10.5) + 24);
        double R = W - 30, H = T + body + 40 + (style.hasCaption() ? 24 : 0);
        ChartCanvas c(W, H);
        c.text(W / 2, 30, title, 15, "#111", "middle");

        double lx = 24;                                            // 语义图例
        for (int s = 0; s < 3; s++) {
            c.rect(lx, 46, 12, 12, semanticColor(style.getPalette(), s), "#555", 0.8);
            std::string lb = t(lg, legend[s]);
            c.text(lx + 17, 56, lb, 11.5, "#333", "start");
            lx += 17 + detail::textWidth(lb, 11.5) + 22;
        }

        int total = 0;
        for (int i = 0; i < k; i++) total = (std::max)(total, win[i] + tie[i] + lose[i]);
        if (total <= 0) total = 1;

        for (int i = 0; i < k; i++) {
            double y = T + (barh + gap) * i;
            c.text(24, y + barh * 0.68, algos[i], 12, "#222", "start");
            int seg[3] = { win[i], tie[i], lose[i] };
            double x = L;
            for (int s = 0; s < 3; s++) {
                double w = (R - L) * (double)seg[s] / (double)total;
                if (w <= 0.01) continue;
                c.rect(x, y, w, barh, semanticColor(style.getPalette(), s), "#555", 0.8, 0.85);
                if (w > 16)                                        // 段够宽才标数字
                    c.text(x + w / 2, y + barh * 0.66, std::to_string(seg[s]), 11,
                           (style.getPalette() == Palette::Grayscale && s == 0) ? "#FFF" : "#111", "middle");
                x += w;
            }
        }
        if (style.hasCaption()) c.text(24, H - 12, caption, 10.5, "#666", "start");
        return c.save(path, fmt);
    }

    // ---- 排名热力图 ----
    //   grid[问题][算法];着色按【行内】归一化(每问题自成一档),格内标原值。
    //   lowIsBetter=true 时小值配深色(名次 1 最深)。
    inline std::string drawHeatmap(const std::vector<std::string>& algos,
                                   const std::vector<std::string>& problems,
                                   const std::vector<std::vector<double>>& grid,
                                   bool lowIsBetter, const std::string& valueLabel,
                                   const std::string& title, const std::string& path,
                                   const ChartStyle& style = ChartStyle(), ChartFormat fmt = ChartFormat::Svg)
    {
        int k = (int)algos.size(), np = (int)problems.size();
        if (k == 0 || np == 0) return "empty heatmap data";
        Lang lg = style.getLang();
        std::string caption = style.hasCaption()
            ? (t(lg, "heat_caption") + (lowIsBetter ? t(lg, "heat_low") : t(lg, "heat_high"))) : "";

        const double cellh = 28;
        double labelW = 0;
        for (int p = 0; p < np; p++) labelW = (std::max)(labelW, detail::textWidth(problems[p], 12.0));
        labelW += 24;
        double cellw = 0;
        for (int i = 0; i < k; i++) cellw = (std::max)(cellw, detail::textWidth(algos[i], 12.0) + 22);
        cellw = (std::max)(cellw, 68.0);

        double L = 24, T = 74;
        double tw = labelW + cellw * k;
        double W = (std::max)(L * 2 + tw, style.hasCaption() ? 24 + detail::textWidth(caption, 10.5) + 24 : 0.0);
        double H = T + cellh * np + 54 + (style.hasCaption() ? 24 : 0);
        ChartCanvas c(W, H);
        c.text(W / 2, 30, title, 15, "#111", "middle");
        c.text(L, 52, valueLabel, 12, "#444", "start");

        for (int i = 0; i < k; i++)                                  // 列头 = 算法
            c.text(L + labelW + cellw * (i + 0.5), T - 8, algos[i], 12, "#222", "middle");

        for (int p = 0; p < np; p++) {
            double y = T + cellh * p;
            c.text(L + 4, y + cellh * 0.66, problems[p], 12, "#222", "start");
            double lo = 1e300, hi = -1e300;                          // 行内归一化
            for (int i = 0; i < k; i++) {
                double v = grid[p][i];
                if (std::isnan(v)) continue;
                lo = (std::min)(lo, v); hi = (std::max)(hi, v);
            }
            double sp = (hi > lo) ? (hi - lo) : 1.0;
            for (int i = 0; i < k; i++) {
                double x = L + labelW + cellw * i, v = grid[p][i];
                if (std::isnan(v)) {
                    c.rect(x, y, cellw, cellh, "#FFFFFF", "#DDD", 0.8);
                    c.text(x + cellw / 2, y + cellh * 0.66, "-", 11.5, "#999", "middle");
                    continue;
                }
                double t01 = (v - lo) / sp;
                if (lowIsBetter) t01 = 1.0 - t01;                    // 越优越深
                std::string fill = rampColor(style.getPalette(), t01);
                c.rect(x, y, cellw, cellh, fill, "#FFFFFF", 1.2);
                // 深底用白字,保证对比度
                c.text(x + cellw / 2, y + cellh * 0.66, detail::fmtNum(v), 11.5,
                       t01 > 0.6 ? "#FFF" : "#222", "middle");
            }
        }
        if (style.hasCaption()) c.text(L, H - 12, caption, 10.5, "#666", "start");
        return c.save(path, fmt);
    }

    // ---- 收敛曲线(v2.2.2.4) ----
    //   x = FES(用户定的横轴);每算法一条主线 + 可选带(如 Q1~Q3)。
    //   数据已由 analyzer 在网格上算好(阶梯保持重采样 + 跨 run 聚合),本层零计算。
    //   NaN = 该网格点无数据(首条记录之前)→ 断线,不连。
    struct CurveSeries
    {
        std::string name;
        std::vector<double> line;       // [网格点]
        std::vector<double> lo, hi;     // 带的上下沿;空 = 无带
    };
    inline std::string drawConvergence(const std::vector<double>& gridX,
                                       const std::vector<CurveSeries>& series,
                                       const std::string& yLabel, const std::string& caption,
                                       const std::string& title, const std::string& path,
                                       const ChartStyle& style = ChartStyle(), ChartFormat fmt = ChartFormat::Svg)
    {
        int k = (int)series.size(), G = (int)gridX.size();
        if (k == 0 || G == 0) return "empty convergence data";

        double vlo = 1e300, vhi = -1e300;
        for (int i = 0; i < k; i++)
        {
            for (int g = 0; g < G; g++) {
                double v = series[i].line[g];
                if (!std::isnan(v)) { vlo = (std::min)(vlo, v); vhi = (std::max)(vhi, v); }
            }
            for (size_t g = 0; g < series[i].lo.size(); g++) {
                double a = series[i].lo[g], b = series[i].hi[g];
                if (!std::isnan(a)) vlo = (std::min)(vlo, a);
                if (!std::isnan(b)) vhi = (std::max)(vhi, b);
            }
        }
        if (vlo > vhi) return "convergence data is all NaN";

        std::vector<std::string> algos;
        for (int i = 0; i < k; i++) algos.push_back(series[i].name);
        detail::Frame f = detail::computeFrame(algos, std::vector<std::string>(1, ""),
                                               vlo, vhi, false, caption, style);
        ChartCanvas c(f.W, f.H);
        detail::drawFrameAxisOnly(c, f, yLabel, caption, title, style);

        // 横轴:FES 刻度 + 图例(收敛曲线横轴是连续量,不走分组标签)
        double x0 = f.L, x1 = f.R, gmin = gridX.front(), gmax = gridX.back();
        auto X = [&](double v) { return (gmax > gmin) ? x0 + (v - gmin) / (gmax - gmin) * (x1 - x0) : x0; };
        for (int i = 0; i <= 4; i++)
        {
            double v = gmin + (gmax - gmin) * i / 4.0;
            c.line(X(v), f.B, X(v), f.B + 5, "#333", 1);
            c.text(X(v), f.B + 18, detail::fmtNum(v), 11, "#444", "middle");
        }
        c.text((x0 + x1) / 2, f.B + 36, style.getLang() == Lang::ZH ? "评估次数 (FES)" : "Evaluations (FES)",
               12, "#444", "middle");

        double lx = f.L;
        for (int i = 0; i < k; i++)
        {
            std::string col = paletteColor(style.getPalette(), i);
            c.line(lx, 52, lx + 18, 52, col, 2.6);
            c.text(lx + 23, 56, series[i].name, 11.5, "#333", "start");
            lx += 23 + detail::textWidth(series[i].name, 11.5) + 20;
        }

        for (int i = 0; i < k; i++)
        {
            std::string col = paletteColor(style.getPalette(), i);
            // 带:上下沿各画一条细线(不填充 —— 多算法叠加时填充互相遮挡)
            if ((int)series[i].lo.size() == G && (int)series[i].hi.size() == G)
                for (int pass = 0; pass < 2; pass++)
                {
                    const std::vector<double>& b = pass ? series[i].hi : series[i].lo;
                    std::vector<std::pair<double, double>> seg;
                    for (int g = 0; g < G; g++) {
                        if (std::isnan(b[g])) { if (seg.size() > 1) c.polyline(seg, col, 0.9); seg.clear(); }
                        else seg.push_back(std::make_pair(X(gridX[g]), f.y(b[g])));
                    }
                    if (seg.size() > 1) c.polyline(seg, col, 0.9);
                }
            // 主线:NaN 处断开,不连
            std::vector<std::pair<double, double>> seg;
            for (int g = 0; g < G; g++) {
                if (std::isnan(series[i].line[g])) { if (seg.size() > 1) c.polyline(seg, col, 2.2); seg.clear(); }
                else seg.push_back(std::make_pair(X(gridX[g]), f.y(series[i].line[g])));
            }
            if (seg.size() > 1) c.polyline(seg, col, 2.2);
        }
        return c.save(path, fmt);
    }

    // ---- Pareto 前沿(目标空间 2D 散点,v2.2.2.5) ----
    //   点由 analyzer 从 _data_buffer 取出直送 —— .rslt 存的是档案内容,配 Multiobject 档案
    //   即非支配集,故【本层与 analyzer 都不做非支配排序】。
    struct ScatterSeries
    {
        std::string name;
        std::vector<std::pair<double, double>> pts;
    };
    inline std::string drawScatter2D(const std::vector<ScatterSeries>& series,
                                     const std::string& xLabel, const std::string& yLabel,
                                     const std::string& caption, bool connect,
                                     const std::string& title, const std::string& path,
                                     const ChartStyle& style = ChartStyle(), ChartFormat fmt = ChartFormat::Svg)
    {
        int k = (int)series.size();
        if (k == 0) return "empty scatter data";
        double xlo = 1e300, xhi = -1e300, ylo = 1e300, yhi = -1e300; size_t total = 0;
        for (int i = 0; i < k; i++)
            for (size_t j = 0; j < series[i].pts.size(); j++)
            {
                double x = series[i].pts[j].first, y = series[i].pts[j].second;
                if (std::isnan(x) || std::isnan(y)) continue;
                xlo = (std::min)(xlo, x); xhi = (std::max)(xhi, x);
                ylo = (std::min)(ylo, y); yhi = (std::max)(yhi, y);
                total++;
            }
        if (total == 0) return "scatter data is all NaN";

        std::vector<std::string> algos;
        for (int i = 0; i < k; i++) algos.push_back(series[i].name);
        detail::Frame f = detail::computeFrame(algos, std::vector<std::string>(1, ""),
                                               ylo, yhi, false, caption, style);
        ChartCanvas c(f.W, f.H);
        detail::drawFrameAxisOnly(c, f, yLabel, caption, title, style);

        double xpad = (xhi > xlo) ? (xhi - xlo) * 0.07 : ((std::fabs(xhi) > 0 ? std::fabs(xhi) : 1.0) * 0.1);
        double dxlo = xlo - xpad, dxhi = xhi + xpad;
        auto X = [&](double v) { return f.L + (v - dxlo) / (dxhi - dxlo) * (f.R - f.L); };
        for (int i = 0; i <= 4; i++)
        {
            double v = dxlo + (dxhi - dxlo) * i / 4.0;
            c.line(X(v), f.B, X(v), f.B + 5, "#333", 1);
            c.text(X(v), f.B + 18, detail::fmtNum(v), 11, "#444", "middle");
        }
        c.text((f.L + f.R) / 2, f.B + 36, xLabel, 12, "#444", "middle");

        double lx = f.L;
        for (int i = 0; i < k; i++)
        {
            std::string col = paletteColor(style.getPalette(), i);
            c.circle(lx + 5, 52, 4, col, 0.85);
            c.text(lx + 14, 56, series[i].name, 11.5, "#333", "start");
            lx += 14 + detail::textWidth(series[i].name, 11.5) + 20;
        }

        for (int i = 0; i < k; i++)
        {
            std::string col = paletteColor(style.getPalette(), i);
            if (connect)
            {
                std::vector<std::pair<double, double>> sorted = series[i].pts;
                std::sort(sorted.begin(), sorted.end(),
                          [](const std::pair<double, double>& a, const std::pair<double, double>& b)
                          { return a.first < b.first; });
                std::vector<std::pair<double, double>> poly;
                for (size_t j = 0; j < sorted.size(); j++)
                    if (!std::isnan(sorted[j].first) && !std::isnan(sorted[j].second))
                        poly.push_back(std::make_pair(X(sorted[j].first), f.y(sorted[j].second)));
                if (poly.size() > 1) c.polyline(poly, col, 1.2);
            }
            for (size_t j = 0; j < series[i].pts.size(); j++)
            {
                double x = series[i].pts[j].first, y = series[i].pts[j].second;
                if (std::isnan(x) || std::isnan(y)) continue;
                c.circle(X(x), f.y(y), 3.0, col, 0.72);
            }
        }
        return c.save(path, fmt);
    }

    // ---- 平均秩条(横向,按秩升序 = 最优在上;v2.2.2.5) ----
    //   CD 图的简化版:无显著性信息,但算法多时 CD 图会挤成一团,秩条仍清晰。
    inline std::string drawRankBar(const std::vector<std::string>& algos,
                                   const std::vector<double>& avgRanks,
                                   const std::string& caption,
                                   const std::string& title, const std::string& path,
                                   const ChartStyle& style = ChartStyle(), ChartFormat fmt = ChartFormat::Svg)
    {
        int k = (int)algos.size();
        if (k == 0) return "empty rank data";
        std::vector<int> order;
        for (int i = 0; i < k; i++) if (!std::isnan(avgRanks[i])) order.push_back(i);
        if (order.empty()) return "rank data is all NaN";
        std::sort(order.begin(), order.end(),
                  [&](int a, int b) { return avgRanks[a] < avgRanks[b]; });   // 秩小 = 更优 = 排在上

        double labelW = 0;
        for (int i = 0; i < k; i++) labelW = (std::max)(labelW, detail::textWidth(algos[i], 12.0));
        double L = 24 + labelW + 12, T = 74, barh = 24, gap = 10;
        double body = barh * (int)order.size() + gap * ((int)order.size() - 1);
        double W = (std::max)(720.0, L + 340 + 60);
        if (style.hasCaption()) W = (std::max)(W, 24 + detail::textWidth(caption, 10.5) + 24);
        double R = W - 56, H = T + body + 46 + (style.hasCaption() ? 24 : 0);
        ChartCanvas c(W, H);
        c.text(W / 2, 30, title, 15, "#111", "middle");
        c.text(24, 54, t(style.getLang(), "rb_axis"), 12, "#444", "start");

        double hi = 0;
        for (size_t i = 0; i < order.size(); i++) hi = (std::max)(hi, avgRanks[order[i]]);
        if (hi <= 0) hi = 1;
        hi *= 1.06;

        for (size_t r = 0; r < order.size(); r++)
        {
            int i = order[r];
            double y = T + (barh + gap) * (double)r;
            std::string col = paletteColor(style.getPalette(), i);   // 对象索引取色 ⇒ 跨图同算法同色
            c.text(24, y + barh * 0.68, algos[i], 12, "#222", "start");
            double w = (R - L) * avgRanks[i] / hi;
            if (w > 0.5) c.rect(L, y, w, barh, col, col, 1.4, 0.62);
            char v[24]; std::snprintf(v, 24, "%.3g", avgRanks[i]);
            c.text(L + w + 6, y + barh * 0.68, v, 11.5, "#333", "start");
        }
        c.line(L, T - 4, L, T + body + 4, "#333", 1.2);      // 零基线
        if (style.hasCaption()) c.text(24, H - 12, caption, 10.5, "#666", "start");
        return c.save(path, fmt);
    }

    // ---- Nemenyi 临界差(CD)图(Demšar 2006) ----
    //   algos/avgRanks 按【对象索引】给(与 analyzer 的 optimizer 序一致 ⇒ 跨图同色);
    //   title 为每图参数;style 跨图共享。返回超界告警(空=正常)。
    inline std::string drawCdDiagram(const std::vector<std::string>& algos,
                                     const std::vector<double>& avgRanks, double CD,
                                     const std::string& title, const std::string& path,
                                     const ChartStyle& style = ChartStyle(), ChartFormat fmt = ChartFormat::Svg)
    {
        int k = (int)algos.size();
        if (k < 2) return "need >=2 algorithms";
        Lang L = style.getLang();
        // 画布随算法数自适应(需求⑤:先按内容定尺,再检查)
        double W = 720, H = 150.0 + 24.0 * ((k + 1) / 2) + (style.hasCaption() ? 28 : 8);
        double x0 = 110, x1 = W - 110, axisY = 118;
        ChartCanvas c(W, H);

        double rmin = 1, rmax = k;
        auto X = [&](double r) { return x0 + (r - rmin) / (rmax - rmin) * (x1 - x0); };

        c.text(W / 2, 30, title, 16, "#111", "middle");                       // ①自定义标题
        c.text(W / 2, 50, t(L, "rank"), 12, "#666", "middle");                // ③框架文字随语言

        c.line(x0, axisY, x1, axisY, "#333", 1.5);
        for (int r = 1; r <= k; r++) {
            c.line(X(r), axisY - 6, X(r), axisY, "#333");
            c.text(X(r), axisY - 11, std::to_string(r), 12, "#555", "middle");
        }
        c.text(x0, axisY - 30, "← " + t(L, "better"), 11, "#2c7", "start");

        // CD 尺
        double cx0 = x0, cx1 = X(1 + (std::min)(CD, (double)(k - 1)));
        c.line(cx0, axisY - 46, cx1, axisY - 46, "#c0392b", 3);
        c.line(cx0, axisY - 50, cx0, axisY - 42, "#c0392b", 2);
        c.line(cx1, axisY - 50, cx1, axisY - 42, "#c0392b", 2);
        { std::ostringstream s; s.precision(2); s << std::fixed << CD;
          c.text((cx0 + cx1) / 2, axisY - 52, t(L, "cd") + " = " + s.str(), 12, "#c0392b", "middle"); }

        // 按秩排序,前半左侧、后半右侧;②颜色按【对象索引】取 ⇒ 跨图同色
        std::vector<int> id(k); for (int i = 0; i < k; i++) id[i] = i;
        std::sort(id.begin(), id.end(), [&](int a, int b) { return avgRanks[a] < avgRanks[b]; });
        int leftN = (k + 1) / 2; double baseY = axisY + 34, dy = 24;
        for (int q = 0; q < k; q++) {
            int i = id[q]; bool left = q < leftN;
            double px = X(avgRanks[i]);
            double ty = left ? (baseY + q * dy) : (baseY + (k - 1 - q) * dy);
            double lx = left ? x0 - 6 : x1 + 6;
            std::string col = paletteColor(style.getPalette(), i);            // ②对象索引取色
            c.line(px, axisY, px, ty, col, 1.6);
            c.line(px, ty, left ? x0 : x1, ty, col, 1.6);
            std::ostringstream s; s.precision(2); s << std::fixed << avgRanks[i];
            c.text(lx, ty + 4, algos[i] + " (" + s.str() + ")", 13, col, left ? "end" : "start");
        }

        // 非显著"团"(秩差<CD)横粗线
        double cy = axisY + 12; int lastEnd = -1;
        for (int a = 0; a < k; a++) {
            int b = a;
            while (b + 1 < k && avgRanks[id[b + 1]] - avgRanks[id[a]] < CD - 1e-9) b++;
            if (b > a && b > lastEnd) {
                c.line(X(avgRanks[id[a]]) - 3, cy, X(avgRanks[id[b]]) + 3, cy, "#555", 5, "round");
                cy += 8; lastEnd = b;
            }
        }
        // ④下方解释描述:可选加 + 随语言
        if (style.hasCaption())
            c.text(W / 2, H - 10, t(L, "cd_caption"), 11, "#888", "middle");

        return c.save(path, fmt);   // ⑤返回超界告警(空=正常)
    }
}
