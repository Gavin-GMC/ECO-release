//------------------------Description------------------------
// 哈希评估器 HashEvaluator:按决策向量缓存评估结果,相同解不重复调真实评估(省评估预算/FES)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <cstdint>
#include <vector>
#include <unordered_map>
#include "evaluator.h"
#include "registry.h"

namespace ECFlow
{
    class HashEvaluator final : public Evaluator
    {
    private:
        struct Entry
        {
            std::vector<double> result;
            std::vector<double> fitness;
        };
        std::unordered_map<uint64_t, std::vector<Entry>> _cache;

        // FNV-1a over the raw bytes of the decision vector
        static uint64_t hashResult(const double* data, int size)
        {
            const unsigned char* p = reinterpret_cast<const unsigned char*>(data);
            uint64_t h = 1469598103934665603ULL;             // FNV offset basis
            int bytes = static_cast<int>(sizeof(double)) * size;
            for (int i = 0; i < bytes; i++)
            {
                h ^= p[i];
                h *= 1099511628211ULL;                        // FNV prime
            }
            return h;
        }

        static bool sameResult(const std::vector<double>& cached, const double* result, int size)
        {
            for (int i = 0; i < size; i++)
                if (cached[i] != result[i]) return false;
            return true;
        }

    public:
        HashEvaluator() {}
        ~HashEvaluator() {}

        void setProblem(ProblemHandle* problem_handle) override
        {
            Evaluator::setProblem(problem_handle);
            _cache.clear();
        }

        void evaluate(Terminator* terminator_pointer, BestArchive* archive_pointer, IndividualArray& offspring) override
        {
            for (int i = 0; i < offspring.getSize(); i++)
            {
                Individual& ind = offspring[i];
                if (ind.has_evaluted)
                    continue;
                if (terminator_pointer->termination())
                    break;

                int size = ind.getSolutionSize();
                int obj = ind.getObjectNumber();
                uint64_t key = hashResult(ind.solution.result, size);

                const Entry* hit = nullptr;
                auto it = _cache.find(key);
                if (it != _cache.end())
                {
                    for (const Entry& e : it->second)
                        if (sameResult(e.result, ind.solution.result, size)) { hit = &e; break; }
                }

                if (hit)   // 命中:回填 fitness,不计 FES、不重复入档
                {
                    for (int j = 0; j < obj; j++)
                        ind.solution.fitness[j] = hit->fitness[j];
                    ind.has_evaluted = true;
                }
                else       // 未命中:真实评估 + 入缓存 + 计 FES + 入档
                {
                    _problem->solutionEvaluate(ind.solution);
                    ind.has_evaluted = true;

                    Entry e;
                    e.result.assign(ind.solution.result, ind.solution.result + size);
                    e.fitness.assign(ind.solution.fitness, ind.solution.fitness + obj);
                    _cache[key].push_back(std::move(e));

                    bool best_solution_updated = archive_pointer->updateBest(ind.solution);
                    terminator_pointer->update(best_solution_updated);
                }
                ind.afterEvaluate();   // 命中/未命中均已置 fitness → 特性自更新(pbest 择优保留)
            }
        }
    };

    inline Registry<Evaluator>::Entry hashEvaluatorEntry()
    {
        return { "Hash", ModuleType::T_evaluator, ParameterTemplate{}, sizeof(HashEvaluator),
            [](const double*) -> Evaluator* { return new HashEvaluator(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    ECFLOW_REGISTER(eval_hash, Evaluator, hashEvaluatorEntry());
}
