//------------------------Description------------------------
// 个体基类 Individual:优化器的基本单元,持有一个候选解 + 初始化/比较等操作。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <vector>
#include <string>
#include <memory>
#include <utility>
#include "module-type.h"
#include "solution.h"
#include "comparer.hpp"
#include "problem-handle.h"
#include "solution-initializer.h"
#include "ecflow-assert.h"
#include "logger.hpp"
#include "parameter-template.h"
#include "registry.h"
#include "feature.h"

namespace ECFlow
{
    class Individual
    {
    protected:
        Comparer*            _comparer_pointer;
        SolutionInitializer* _initializer_pointer;

        // 特性袋(INDIV-COMPOSE):key = 装配期解析出的身份键;value = 特性组件(拥有)。
        std::vector<std::pair<std::string, std::unique_ptr<Feature>>> _features;

        FeatureContext _featureCtx()
        {
            // 句柄透传:直接借用个体初始化器持有的句柄(个体不自持),供 RandomInDomain 等取域边界
            ProblemHandle* h = _initializer_pointer ? _initializer_pointer->handle() : nullptr;
            return FeatureContext{ solution.getSolutionSize(), &solution, _comparer_pointer, h };
        }
        void _sizeFeatures()
        {
            FeatureContext c = _featureCtx();
            for (auto& p : _features) p.second->setProblem(c);
        }
        // **就地**把 source 的特性内容复制进本个体的同名特性(不销毁重建 → 保持特性对象地址稳定,
        // 因拓扑等会持有指向特性数据的裸指针,如 &pbestFeature->sol);无同名特性则克隆补上(兜底)。
        void _copyFeaturesFrom(const Individual& s)
        {
            for (auto& sp : s._features)
            {
                Feature* dst = nullptr;
                for (auto& p : _features) if (p.first == sp.first) { dst = p.second.get(); break; }
                if (dst) dst->copyFrom(*sp.second);
                else _features.emplace_back(sp.first, std::unique_ptr<Feature>(sp.second->clone()));
            }
        }

    public:
        Solution solution;
        bool     has_evaluted;

        // ---- 特性袋接口 ----
        void addFeature(const std::string& key, Feature* f)
        {
            _features.emplace_back(key, std::unique_ptr<Feature>(f));
        }
        template<class T> T* feature(const std::string& key)
        {
            for (auto& p : _features)
                if (p.first == key) return static_cast<T*>(p.second.get());
            return nullptr;
        }
        bool hasFeature(const std::string& key) const
        {
            for (auto& p : _features)
                if (p.first == key) return true;
            return false;
        }
        int featureCount() const { return (int)_features.size(); }

        // 生成子代时:把 src 中"随粒子记忆前传"的特性(inheritAtBirth,如 pbest)按各自出生语义并入本体同名特性——
        //   经 birthFrom(默认就地拷贝;血缘类为"派生":记源亲本 id)。velocity 等 inheritAtBirth=false 者不动,随后由策略重算。
        void inheritFeaturesFrom(const Individual& src)
        {
            for (auto& p : _features)
            {
                if (!p.second->inheritAtBirth()) continue;
                for (auto& sp : src._features)
                    if (sp.first == p.first) { p.second->birthFrom(*sp.second); break; }
            }
        }
        // 个体评估完成后:广播给各特性自更新(pbest 据此择优保留;其余默认空操作)。
        void afterEvaluate()
        {
            for (auto& p : _features) p.second->afterEvaluate(solution, _comparer_pointer);
        }

        Individual()
        {
            solution = Solution();
            _comparer_pointer = nullptr;
            _initializer_pointer = nullptr;
            has_evaluted = false;
        }

        virtual ~Individual() {}

        static ParameterTemplate getParameterTemplate() { return ParameterTemplate{}; }
        static void preAssert(AssertList& list, double* paras) {}
        static void postAssert(AssertList& list, double* paras) {}

        int getSolutionSize() { return solution.getSolutionSize(); }
        int getObjectNumber() { return solution.getObjectNumber(); }

        virtual void setProblem(ProblemHandle* problem_handle)
        {
            solution.setSize(problem_handle->getProblemSize(), problem_handle->getObjectNumber());
            solution.setDecoder(problem_handle->getSolutionDecoder());
            _comparer_pointer = problem_handle->getSolutionComparer();
            _sizeFeatures();
        }

        virtual void setProblem(const Individual& source)
        {
            solution.setSize(source.solution);
            solution.setDecoder(source.solution);
            _comparer_pointer = source._comparer_pointer;
            _sizeFeatures();
        }

        void setInitializer(SolutionInitializer* initializer) { _initializer_pointer = initializer; }

        virtual void ini(bool ini_solution = true, bool evaluate = true, bool ini_speciality = false)
        {
            if (ini_solution)
            {
                if (evaluate)
                    _initializer_pointer->ini_solution(solution);
                else
                    _initializer_pointer->ini_solution(solution.result, solution.getSolutionSize());
            }
            if (ini_speciality)
            {
                FeatureContext c = _featureCtx();
                for (auto& p : _features) p.second->ini(c);   // 特性初始化(含 AgeFeature 归零)
            }
        }

        virtual void copy(const Individual& copy_source)
        {
            setProblem(copy_source);
            solution.copy(copy_source.solution);              // 复制决策值与适应度
            setInitializer(copy_source._initializer_pointer); // 沿用初始化器
            _copyFeaturesFrom(copy_source);                   // 就地复制特性内容(保持地址稳定)
        }

        virtual void copy(const double* source_result, const double* source_fitness)
        {
            solution.copy(source_result, source_fitness);
        }

        virtual void shallowCopy(const Individual& copy_source)
        {
            solution.shallowCopy(copy_source.solution);
            _comparer_pointer = copy_source._comparer_pointer;
            _initializer_pointer = copy_source._initializer_pointer;
        }

        virtual void shallowClear()
        {
            solution.shallowClear();
            _comparer_pointer = nullptr;
            _initializer_pointer = nullptr;
        }

        virtual void swap(Individual& copy_source)
        {
            solution.swap(copy_source.solution);
            _features.swap(copy_source._features);   // 交换特性袋(指针交换)
        }

        double& operator[](const int index) { return solution[index]; }

        bool operator<(const Individual& a) const
        {
            return _comparer_pointer->isBetter(solution.fitness, a.solution.fitness);
        }

        bool operator==(const Individual& a) const { return solution == a.solution; }
    };

    // INDIV-COMPOSE 收官:个体只剩基类 Individual、行为全由挂载特性决定 → Registry<Individual>(按名造子类)已无意义,移除。
    //   个体由 IndividualArray 直接 `new Individual()`(老路径)或按推断特性集挂载(特性路径)构造。
}
