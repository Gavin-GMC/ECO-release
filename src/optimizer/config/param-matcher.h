//------------------------Description------------------------
// ParamMatcher:装配期**参数模板校验**。按各组件注册项的 ParameterTemplate 校验其 para——
//   元数 / 留空合法性(allow_empty) / 取值范围 [low,high] / Int·Enum 的整值性。
//   与 AssertMatcher **并列同层**(前者校"组件间契约",本者校"组件自身参数"),复用其 ValidateResult。
//-------------------------Reference-------------------------
// 全新实现。参数模板本意是**给自动化配置程序的机读入口**(parameter-template.h),
//   本模块只是给这份既有元数据**追加一个消费方**,不改其定位。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <cmath>
#include "configure-list.h"
#include "assert-matcher.h"   // ValidateResult(与断言校验共用报错结构 → 报错风格统一)
#include "ecflow-constant.h"    // EMPTYVALUE / is_empty
#include "registry.h"
#include "parameter-template.h"
// 仅需**基类**声明以实例化 Registry<Base>;tag 的自注册由调用方(optimizer-builder.h)的聚合头保证。
#include "learning-topology.h"
#include "learning-strategy.h"
#include "generator.h"
#include "selector.h"
#include "solution-repairman.h"
#include "best-archive.h"

namespace ECFlow
{
    class ParamMatcher
    {
    private:
        // 渐进披露的层数上限。`ParameterTemplate::next` 是函数指针,可返回含 next 的模板、甚至构成环
        //   (某级 next 返回自身)。**带参数的环会因 para 耗尽而自然终止,零参数的环则不会** → 必须设限。
        //   8 层远超任何现实需求(现有最深:TabuDecorator → 内层算子 = 2 层)。
        static const int MAX_DEPTH = 8;

        static const char* kindName(ParamKind k)
        {
            switch (k) { case ParamKind::Enum: return "Enum"; case ParamKind::Int: return "Int"; default: return "Real"; }
        }

        // 单个参数值 vs 其元信息
        static void checkValue(const std::string& where, const ParamInfo& pi, double v, ValidateResult& vr)
        {
            if (is_empty(v))   // ★ 先分流:下面的范围比较对 NaN 恒假,直接比会静默放行
            {
                if (!pi.allow_empty)
                    vr.errors.push_back(where + "." + pi.name + ": empty(未设置) but allow_empty=false");
                return;        // 允许留空 → 由组件内部回落默认,不再校范围
            }
            if (v < pi.low || v > pi.high)
            {
                std::ostringstream o;
                o << where << "." << pi.name << ": " << v << " out of range [" << pi.low << ", " << pi.high << "]";
                vr.errors.push_back(o.str());
            }
            if ((pi.kind == ParamKind::Int || pi.kind == ParamKind::Enum) && std::floor(v) != v)
            {
                std::ostringstream o;
                o << where << "." << pi.name << ": " << v << " not integral (kind=" << kindName(pi.kind) << ")";
                vr.errors.push_back(o.str());
            }
        }

        // 逐级解析并校验(渐进披露的递归 walk)。返回**本级及其后续各级**共消费的参数个数。
        //   * `para`/`n` 是**本级切片**(从本级第一个参数起)—— 与 ParameterTemplate::next 的契约一致,
        //     使每级模板都从自己的下标 0 看世界、不关心嵌在第几层(TabuDecorator 原样返回内层模板即靠此成立)。
        //   * 深度上限:`next` 是函数指针,可返回含 next 的模板、甚至构成环(某级 next 返回自己)→ 必须设限,
        //     且**超限报错**而非静默截断(静默截断会让元数算错、放过真错)。
        // ★ 返回值 = 本级及其后各级的**期望参数总数**(不是"实际消费数")。
        //   这是本函数唯一容易写错的地方:下一级参数不足时若返回"实际有几个",总数会恰好等于 para.size()
        //   → **元数错被自吞**(如 `Index {2}` 缺 T0/decay 将不报错,而那正是 makeAccept 的越界洞)。
        static size_t walk(const ParameterTemplate& tpl, const std::string& where,
                           const double* para, size_t n, int depth, ValidateResult& vr)
        {
            if (depth > MAX_DEPTH)
            {
                std::ostringstream o;
                o << where << ": 渐进披露层数超过上限 " << MAX_DEPTH << " —— 疑为模板 next 自引用成环";
                vr.errors.push_back(o.str());
                return n;   // 返回"剩余数"使总数恰为 para.size() → 不再叠加一条无意义的元数错
            }

            // 校验本级中**实际存在**的那些值(不足的部分由元数错覆盖)
            size_t avail = tpl.count() < n ? tpl.count() : n;
            for (size_t i = 0; i < avail; i++) checkValue(where, tpl.params[i], para[i], vr);

            // 本级都没给全 → 无法据本级取值问下一级(渐进披露的前提是前级已定)→ 只报本级期望数
            if (n < tpl.count()) return tpl.count();

            ParameterTemplate sub = tpl.nextLevel(para, n);   // ★ 传本级切片,非整个 para
            if (sub.count() == 0 && !sub.hasNext()) return tpl.count();   // 无下一级 → 到此为止
            return tpl.count() + walk(sub, where, para + tpl.count(), n - tpl.count(), depth + 1, vr);
        }

        // 对一份模板校验一组 para(逐级渐进披露),结果并入 vr
        static void checkAgainst(const ParameterTemplate& tpl, const std::string& where,
                                 const std::vector<double>& para, ValidateResult& vr)
        {
            // ★ para **全空** = "全用组件内部默认",是平台的合法约定,非"缺参数":
            //   builder 对空 para 传 **nullptr**(`c.para.empty() ? nullptr : c.para.data()`),
            //   而每个工厂都有 `p ? 读para : 用默认` 的 nullptr 路径 → 不会读任何 para。故直接放行。
            //   注:配置表(经 config-setter 生成)总把参数填满,故此路径只在**手写 ComponentConfig** 时出现
            //   (测试即如此:`{T_offspringgenerator, "Generation", {}}`)。
            // 反之 para **非空即须完整** —— 工厂按位置无条件读(`p[1]`、`p+2`),少给即**越界读**。
            //   要"只设前几个、后面用默认"→ 显式传 EMPTYVALUE(需 allow_empty),而非省略。
            if (para.empty()) return;

            size_t expect = walk(tpl, where, para.data(), para.size(), 0, vr);
            if (para.size() != expect)
            {
                std::ostringstream o;
                o << where << ": para count " << para.size() << " != expected " << expect;
                if (tpl.hasNext()) o << " [各级由前级取值渐进披露]";
                vr.errors.push_back(o.str());
            }
        }

        // 对一个已定位到 Registry 的组件校验其 para
        template <class Base>
        static void checkComponent(const Registry<Base>& reg, const std::string& kind,
                                   const std::string& tag, const std::vector<double>& para, ValidateResult& vr)
        {
            const ParameterTemplate* tpl = reg.params(tag);
            if (tpl == nullptr) return;   // 未注册 tag:由 AssertMatcher 的 N5 管,不在本模块职责
            checkAgainst(*tpl, kind + " '" + tag + "'", para, vr);
        }

    public:
        // 直接对**一份模板**校验一组 para(不经 Registry)。供"手上已有模板"的消费方——
        //   如自动化配置生成器采完样后自检、或对 nextLevel() 取到的子模板单独校验。
        static ValidateResult validate(const ParameterTemplate& tpl, const std::vector<double>& para,
                                       const std::string& where = "params")
        {
            ValidateResult vr;
            checkAgainst(tpl, where, para, vr);
            vr.valid = vr.errors.empty();
            return vr;
        }

        // 校验一个 workflow 的全部组件 + 初始化器无关(初始化器非注册表,不在此)
        static ValidateResult validate(const WorkflowConfig& wf)
        {
            ValidateResult vr;
            for (const ComponentConfig& c : wf.components)
            {
                switch (c.c_type)
                {
                case ModuleType::T_learntopology:
                    checkComponent(Registry<LearningTopology>::instance(),   "topology",  c.tag, c.para, vr); break;
                case ModuleType::T_learnstrategy:
                    checkComponent(Registry<LearningStrategy>::instance(),   "strategy",  c.tag, c.para, vr); break;
                case ModuleType::T_offspringgenerator:
                    checkComponent(Registry<OffspringGenerator>::instance(), "generator", c.tag, c.para, vr); break;
                case ModuleType::T_selector:
                    checkComponent(Registry<EnvirSelect>::instance(),        "selector",  c.tag, c.para, vr); break;
                case ModuleType::T_Repair:
                    checkComponent(Registry<SolutionRepaireman>::instance(), "repair",    c.tag, c.para, vr); break;
                default: break;   // evaluator 等无参数模板项
                }
            }
            vr.valid = vr.errors.empty();
            return vr;
        }

        // 子群侧:档案参数
        static ValidateResult validate(const SubpopulationConfig& sp)
        {
            ValidateResult vr;
            if (!sp.archive_tag.empty())
                checkComponent(Registry<BestArchive>::instance(), "archive", sp.archive_tag, sp.archive_para, vr);
            vr.valid = vr.errors.empty();
            return vr;
        }
    };
}
