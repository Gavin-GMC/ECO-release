//------------------------Description------------------------
// OptimizerBuilder:从 config 装配 O_Workflow 与 Subpopulation。O_Workflow 的友元,直填其单表 components。
//   顶层 optimizer/manager/logger/g_archive、配置文件编解码、assert 校验随 v3.5/v3.6。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <memory>
#include <vector>
#include <string>
#include <ctime>
#include <stdexcept>
#include <sstream>
#include "configure-list.h"
#include "registry.h"
#include "assert-matcher.h"
#include "param-matcher.h"
#include "optimize-workflow.h"
#include "component-wrappers.h"
#include "feature-inference.h"
#include "subpopulation.h"
#include "population.h"
#include "optimizer.h"
#include "subpopulation-manager-basic.h"
#include "subpopulation-topology-basic.h"
#include "logger.hpp"
// —— 聚合/注册头:确保各 Registry 的 tag 已自注册 ——
#include "learning-topologies.h"
#include "learning-strategies.h"
#include "solution-repairman.h"
#include "select-basic.h"
#include "select-immune.h"
#include "select-firework.h"
#include "select-rank-crowding.h"
#include "select-scalar-replace.h"
#include "select-kinship.h"
#include "evaluator-basic.h"
#include "evaluator-hash.h"
#include "generator.h"
#include "best-archive.h"
#include "solution-initializer.h"
#include "distribution-type.h"
#include "terminator.h"

// -------------------------------------------------------
// 装配期校验开关(风格沿用 ECFC原代码 ecflow-builder.h 的 ECFLOW_DISABLE_BUILD_VALIDATION)
//   两道校验在 buildSubpopulation 前**依次**执行,各自可独立禁用:
// -------------------------------------------------------

// 打开此宏以禁用装配前的**断言匹配**校验(AssertMatcher:组件间契约,如 strategy 需 topology.objects)
// #define ECFLOW_DISABLE_ASSERT_VALIDATION

// 打开此宏以禁用装配前的**参数模板**校验(ParamMatcher:组件自身参数的元数/留空/范围/整值)
// #define ECFLOW_DISABLE_PARAM_VALIDATION

namespace ECFlow
{
    class OptimizerBuilder
    {
    private:
        std::vector<WorkflowConfig> _workflows;

        // LOG-DETAIL 失败点兜底:Registry::create 未命中(非 workflow-matcher 覆盖的 cooperation/archive tag)→
        //   nullptr,若不拦下游空指针崩。此处 null-check + sys_logger.error 落盘 + 抛清晰错误(BUILD-FAILFAST 补全)。
        template <class T>
        static T* req(T* p, const std::string& what)
        {
            if (p == nullptr)
            {
                sys_logger.error("[build] " + what);
                throw std::runtime_error("[build] " + what);
            }
            return p;
        }

        // 造纯算子 → 包 wrapper(c_type 决定 Registry + wrapper);para 空则传 nullptr(注册项自取缺省)
        O_Component* Cbuild(const ComponentConfig& c)
        {
            const double* p = c.para.empty() ? nullptr : c.para.data();
            switch (c.c_type)
            {
            case ModuleType::T_learntopology:
                return new SetLearningTopology(Registry<LearningTopology>::instance().create(c.tag, p));
            case ModuleType::T_learnstrategy:
                return new SetLearningStrategy(Registry<LearningStrategy>::instance().create(c.tag, p));
            case ModuleType::T_Repair:
                return new SetRepaire(Registry<SolutionRepaireman>::instance().create(c.tag, p));
            case ModuleType::T_selector:
                return new RunSelect(Registry<EnvirSelect>::instance().create(c.tag, p));
            case ModuleType::T_evaluator:
                return new RunEvaluate(Registry<Evaluator>::instance().create(c.tag, p));
            case ModuleType::T_offspringgenerator:
                return new RunGenerate(Registry<OffspringGenerator>::instance().create(c.tag, p));
            default:
                return nullptr;
            }
        }

        // initializer 工厂(D3:含参;仿稳定版覆盖 Random/Greedy/No,并补 R_G/Distribution)
        SolutionInitializer* Ibuild(const std::string& tag, const std::vector<double>& para)
        {
            if (tag == "Random")       return new RandomInitializer();
            if (tag == "Greedy")       return new GreedyInitializer();
            if (tag == "No")           return new NoInitializer();
            if (tag == "R_G")          return new R_GInitializer(para.empty() ? 0.5 : para[0]);
            if (tag == "Distribution") return new DistributionInitializer(DistributionType(para.empty() ? int(DistributionType::F_Gaussian) : int(para[0])));
            return nullptr;
        }

        Terminator* buildTerminator(const int tc[3])
        {
            Terminator* t = new Terminator();
            int fes = tc[0], convergence = tc[1], sec = tc[2];
            if (fes < 0 && convergence < 0 && sec < 0)
                fes = int(1e5);
            if (fes > 0)         t->setMaxFES(fes);
            if (convergence > 0) t->setMaxConvergence(convergence);
            if (sec > 0)         t->setMaxTime(sec);
            return t;
        }

    public:
        void registerWorkflow(const WorkflowConfig& wf) { _workflows.push_back(wf); }

        // 从 WorkflowConfig 装配单表 O_Workflow(友元直填 private components/initializer)
        std::unique_ptr<O_Workflow> buildWorkflow(const WorkflowConfig& config)
        {
            auto wf = std::make_unique<O_Workflow>();
            for (int i = 0; i < (int)config.components.size(); i++)
                wf->components.push_back(Cbuild(config.components[i]));   // 单表:config 顺序 = 执行序
            wf->initializer = Ibuild(config.ini_tag, config.ini_para);
            return wf;
        }

        // 按 tag 查已注册的 workflow 配置(供装配/校验)
        const WorkflowConfig* findWorkflow(const std::string& tag) const
        {
            for (int i = 0; i < (int)_workflows.size(); i++)
                if (_workflows[i].tag == tag) return &_workflows[i];
            return nullptr;
        }
        // 按 tag 查已注册的 workflow 配置并装配
        std::unique_ptr<O_Workflow> buildWorkflow(const std::string& tag)
        {
            const WorkflowConfig* wf = findWorkflow(tag);
            return wf ? buildWorkflow(*wf) : nullptr;
        }

        // 装配期校验的统一落地:警告 → sys_logger.warning;硬失败 → sys_logger.error 落盘 + 抛。
        //   两道校验(assert / param)共用本函数 → 报错风格一致,用户看到的是同一种错误。
        static void reportOrThrow(const ValidateResult& vr, const std::string& mark, const std::string& subject)
        {
            for (const std::string& w : vr.warnings) sys_logger.warning(mark + " " + w);
            if (vr.valid) return;
            std::ostringstream oss;
            oss << mark << " " << subject << " validation FAILED:";
            for (const std::string& e : vr.errors) { oss << "\n  " << e; sys_logger.error(mark + " " + e); }
            throw std::runtime_error(oss.str());
        }

        // 装配前校验:①assert ②param。**依次**执行,各自可宏禁用。
        //   次序理由:assert 先行——它含 N5(tag 合法);tag 都不认识时,再报参数错只会淹没真正的原因。
        void validateOrThrow(const SubpopulationConfig& pc)
        {
            const WorkflowConfig* wf = findWorkflow(pc.workflow_tag);
            if (wf == nullptr)
                throw std::runtime_error("[assert] subpopulation '" + pc.tag + "': unknown workflow tag '" + pc.workflow_tag + "'");

#ifndef ECFLOW_DISABLE_ASSERT_VALIDATION
            reportOrThrow(AssertMatcher::validate(*wf), "[assert]", "workflow '" + pc.workflow_tag + "'");
#endif

#ifndef ECFLOW_DISABLE_PARAM_VALIDATION
            reportOrThrow(ParamMatcher::validate(*wf), "[param]", "workflow '" + pc.workflow_tag + "'");
            reportOrThrow(ParamMatcher::validate(pc),  "[param]", "subpopulation '" + pc.tag + "'");
#endif
        }

        // 从 SubpopulationConfig 装配 Subpopulation(拥有 parent/offspring/archive/terminator/workflow;logger 不拥有)
        Subpopulation* buildSubpopulation(const SubpopulationConfig& pc, Logger* logger = nullptr)
        {
            validateOrThrow(pc);   // 装配前校验(缓存,一 workflow 多子群只校一次)

            // INDIV-COMPOSE:先建 workflow(得组件实例)→ 装配期推断个体特性集 + 戳组件键。
            const WorkflowConfig* wfc = findWorkflow(pc.workflow_tag);   // validateOrThrow 已保证非空
            std::unique_ptr<O_Workflow> wf = buildWorkflow(*wfc);
            std::vector<FeatureSpec> spec = FeatureInference::infer(wfc->components, wf->components);

            // 有推断特性 → 造基类 + 挂特性;无 → 只造基类
            IndividualArray* parent    = spec.empty() ? new IndividualArray(pc.size)
                                                      : new IndividualArray(spec, pc.size);
            IndividualArray* offspring = spec.empty() ? new IndividualArray(pc.size)
                                                      : new IndividualArray(spec, pc.size);

            const double* ap = pc.archive_para.empty() ? nullptr : pc.archive_para.data();
            Subpopulation* sub = new Subpopulation(
                pc.tag, parent, offspring,
                buildTerminator(pc.terminate_conditions),
                req(Registry<BestArchive>::instance().create(pc.archive_tag, ap),
                    "subpopulation '" + pc.tag + "': unknown archive tag '" + pc.archive_tag + "'"),
                logger);   // v3.6 LOG:接线(独立造子群时缺省 nullptr)
            sub->setWorkflow(std::move(wf));
            return sub;
        }

        // 装配顶层 Population:协作(管理器+注入构建器/拓扑)+ N 子群 + 全局档案。
        //   一个 optimizer 一个 Population,故直接收这三件、不经 PopulationConfig 聚合;v3.6 顶层 ConfigureList 直接持之。
        Population* buildPopulation(const CooperationConfig& c,
                                    const std::vector<SubpopulationConfig>& subpopulations,
                                    const std::string& g_archive_tag = "Basic",
                                    const std::vector<double>& g_archive_para = {},
                                    Logger* logger = nullptr)
        {
            SubpopulationManager* mgr = req(Registry<SubpopulationManager>::instance().create(
                c.manager_tag, c.manager_para.empty() ? nullptr : c.manager_para.data()),
                "unknown manager tag: '" + c.manager_tag + "'");

            // SingleSwarm 构造即自建 Fixed+NoConnect,不注入;其余注入构建器/拓扑(组合装配,同 v3.4.c)
            if (c.manager_tag != "Single")
            {
                mgr->setConstructer(req(Registry<SubpopulationConstructer>::instance().create(
                    c.constructer_tag, c.constructer_para.empty() ? nullptr : c.constructer_para.data()),
                    "unknown constructer tag: '" + c.constructer_tag + "'"));
                mgr->setTopology(req(Registry<SubpopulationTopology>::instance().create(
                    c.topology_tag, c.topology_para.empty() ? nullptr : c.topology_para.data()),
                    "unknown cooperation topology tag: '" + c.topology_tag + "'"));
            }

            int n = (int)subpopulations.size();
            mgr->setSwarmNumber(n);
            for (int i = 0; i < n; i++)
                mgr->setSwarm(buildSubpopulation(subpopulations[i], logger), i);

            const double* gap = g_archive_para.empty() ? nullptr : g_archive_para.data();
            BestArchive* g_archive = req(Registry<BestArchive>::instance().create(g_archive_tag, gap),
                "unknown g_archive tag: '" + g_archive_tag + "'");

            return new Population(mgr, g_archive);
        }

        // 从 OptimizerConfig 装配顶层 Optimizer(Logger + Population + Terminator)
        Optimizer* buildOptimizer(const OptimizerConfig& cfg)
        {
            std::string tag = cfg.tag.empty() ? std::to_string(time(NULL)) : cfg.tag;

            Logger* logger = new Logger(cfg.name, tag,
                cfg.logger_full_result, cfg.logger_process, cfg.logger_full_process, cfg.logger_console_echo);

            Population* pop = buildPopulation(cfg.cooperation, cfg.subpopulations,
                cfg.g_archive_tag, cfg.g_archive_para, logger);

            Terminator* terminator = buildTerminator(cfg.terminate_conditions);

            return new Optimizer(pop, terminator, logger, tag);
        }
    };
}
