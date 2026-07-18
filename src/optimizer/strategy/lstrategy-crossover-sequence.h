//------------------------Description------------------------
// 面向序列/排列的交叉算子:PartialMapped(PMX)/ Cycle(CX)/ Order(OX)/ SubtourExchange / PositionBased(PBX)。
//   兼顾"同解内相同变量"与"两解互不具有的变量"两种场景(1对1/1对多/1对0 匹配,见 buildMap 注释)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <cmath>
#include <cstring>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include "ecflow-constant.h"
#include "ecflow-basicfunc.h"
#include "individual.h"
#include "problem-handle.h"
#include "lstrategy-crossover.h"
#include "registry.h"

namespace ECFlow
{
    // 匹配顺序(序列约束不满足时算子本身工作不佳,不过度追求相似性):
    //   ① 学习对象中元素匹配且未交换的基因位;② 对位元素(退回传统点/均匀交叉);③ 随机未匹配元素。

    // 构建基因位匹配:将 s1 中索引在 map_list 内的基因位与 s2 匹配,结果存 match_list(list_size 项)
    inline void buildMap(double* s1, double* s2, int solution_size, int* map_list, int* match_list, int list_size)
    {
        bool* used = new bool[solution_size];
        for (int i = 0; i < solution_size; i++)
            used[i] = false;

        bool found;
        int s1_index;
        for (int i = 0; i < list_size; i++)
        {
            s1_index = map_list[i];
            found = false;
            int d;
            for (d = 0; d < solution_size; d++)
            {
                if (used[d]) continue;
                if (equal(s1[s1_index], s2[d]))
                {
                    used[d] = true;
                    match_list[i] = d;
                    found = true;
                    break;
                }
            }
            // 无匹配则优先对位,再随机
            d = s1_index;
            while (!found)
            {
                if (!used[d])
                {
                    used[d] = true;
                    match_list[i] = d;
                    found = true;
                    break;
                }
                d = (d + ECFlow::get_int(0, solution_size - 1)) % solution_size;   // 修复:rand()%N
            }
        }
        delete[] used;
    }

    // 重载:匹配 [begin_index, end_index) 段
    inline void buildMap(double* s1, double* s2, int solution_size, int begin_index, int end_index, int* match_list)
    {
        bool* used = new bool[solution_size];
        for (int i = 0; i < solution_size; i++)
            used[i] = false;

        int list_size = end_index - begin_index;
        bool found;
        int s1_index;
        for (int i = 0; i < list_size; i++)
        {
            s1_index = begin_index + i;
            found = false;
            int d;
            for (d = 0; d < solution_size; d++)
            {
                if (used[d]) continue;
                if (equal(s1[s1_index], s2[d]))
                {
                    used[d] = true;
                    match_list[i] = d;
                    found = true;
                    break;
                }
            }
            d = s1_index;
            while (!found)
            {
                if (!used[d])
                {
                    used[d] = true;
                    match_list[i] = d;
                    found = true;
                    break;
                }
                d = (d + ECFlow::get_int(0, solution_size - 1)) % solution_size;   // 修复:rand()%N
            }
        }
        delete[] used;
    }

    // 部分匹配交叉(PMX)
    class PartialMappedCrossover : public Crossover
    {
    private:
        int _buffer_size;
        int begin_index;
        int end_index;
        double* offspring1;
        double* offspring2;

        void buildPartialMap(double* s1, double* s2, int part_size, std::vector<std::pair<double, double>>& partial_map)
        {
            partial_map.clear();
            for (int i = 0; i < part_size; i++)
                partial_map.push_back({ s1[i], s2[i] });

            for (int i = 0; i < (int)partial_map.size(); i++)
            {
                for (int j = i + 1; j < (int)partial_map.size(); j++)
                {
                    if (equal(partial_map[i].second, partial_map[j].first)) // 向后合并
                    {
                        partial_map[i].second = partial_map[j].second;
                        partial_map.erase(partial_map.begin() + j);
                        j = i + 1;
                    }
                    else if (equal(partial_map[i].first, partial_map[j].second)) // 向前合并
                    {
                        partial_map[i].first = partial_map[j].first;
                        partial_map.erase(partial_map.begin() + j);
                        j = i + 1;
                    }
                }
            }
        }

    public:
        PartialMappedCrossover(double cross_rate = 0.9, bool coupled = true) : Crossover(cross_rate, coupled)
        {
            _buffer_size = 0;
            offspring1 = nullptr;
            offspring2 = nullptr;
            begin_index = -1;
            end_index = -1;
        }

        ~PartialMappedCrossover() { delete[] offspring1; delete[] offspring2; }

        static void preAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::notLessButNotice));
            if (paras[1]) list.add(new Assert(ModuleType::T_learntopology, "coupled", 1, MatchType::anyButNotice));
        }
        static void postAssert(AssertList& list, double* paras) {}

        void preparation(Solution* s) override
        {
            Crossover::preparation(s);
            if (new_pair && is_crossover)
            {
                if (_buffer_size != s->getSolutionSize())
                {
                    delete[] offspring1; delete[] offspring2;
                    _buffer_size = s->getSolutionSize();
                    offspring1 = new double[_buffer_size];
                    offspring2 = new double[_buffer_size];
                }
                begin_index = ECFlow::get_int(0, s->getSolutionSize() - 1);
                end_index = ECFlow::get_int(0, s->getSolutionSize() - 1);
                if (end_index < begin_index) std::swap(begin_index, end_index);
                end_index++;
            }
        }

        void getNewIndividual(Individual* child_individual, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle) override
        {
            Solution* child = &child_individual->solution;
            Solution* s1 = &individual->solution;
            Solution** s2 = learning_object;
            preparation(child);

            if (is_crossover)
            {
                if (new_pair)
                {
                    int solution_size = child->getSolutionSize();
                    memcpy(offspring1, s1->result, solution_size * sizeof(double));
                    memcpy(offspring2, s2[0]->result, solution_size * sizeof(double));

                    for (int i = begin_index; i < end_index; i++)
                        std::swap(offspring1[i], offspring2[i]);

                    int part_size = end_index - begin_index;
                    std::vector<std::pair<double, double>> partial_map;
                    buildPartialMap(offspring1 + begin_index, offspring2 + begin_index, part_size, partial_map);

                    bool used;
                    for (int m = 0; m < (int)partial_map.size(); m++)   // 对 o1 映射
                    {
                        used = false;
                        for (int i = 0; i < begin_index; i++)
                            if (equal(offspring1[i], partial_map[m].first)) { offspring1[i] = partial_map[m].second; used = true; break; }
                        if (used) continue;
                        for (int i = end_index; i < solution_size; i++)
                            if (equal(offspring1[i], partial_map[m].first)) { offspring1[i] = partial_map[m].second; break; }
                    }
                    for (int m = 0; m < (int)partial_map.size(); m++)   // 对 o2 映射
                    {
                        used = false;
                        for (int i = 0; i < begin_index; i++)
                            if (equal(offspring2[i], partial_map[m].second)) { offspring2[i] = partial_map[m].first; used = true; break; }
                        if (used) continue;
                        for (int i = end_index; i < solution_size; i++)
                            if (equal(offspring2[i], partial_map[m].second)) { offspring2[i] = partial_map[m].first; break; }
                    }

                    memcpy(child->result, offspring1, solution_size * sizeof(double));
                }
                else
                {
                    memcpy(child->result, offspring2, child->getSolutionSize() * sizeof(double));
                }
            }
            else
            {
                memcpy(child->result, s1->result, child->getSolutionSize() * sizeof(double));
            }
            ending();
        }
    };

    // 循环交叉(CX)
    class CycleCrossover : public Crossover
    {
    private:
        int _buffer_size;
        bool* used;
        int begin_index;
        double* offspring1;
        double* offspring2;

    public:
        CycleCrossover(double cross_rate = 0.9, bool coupled = true) : Crossover(cross_rate, coupled)
        {
            _buffer_size = 0;
            used = nullptr;
            offspring1 = nullptr;
            offspring2 = nullptr;
            begin_index = -1;
        }

        ~CycleCrossover() { delete[] used; delete[] offspring1; delete[] offspring2; }

        static void preAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::notLessButNotice));
            if (paras[1]) list.add(new Assert(ModuleType::T_learntopology, "coupled", 1, MatchType::anyButNotice));
        }
        static void postAssert(AssertList& list, double* paras) {}

        void preparation(Solution* s) override
        {
            Crossover::preparation(s);
            if (new_pair && is_crossover)
            {
                if (_buffer_size != s->getSolutionSize())
                {
                    delete[] used; delete[] offspring1; delete[] offspring2;
                    _buffer_size = s->getSolutionSize();
                    used = new bool[_buffer_size];
                    offspring1 = new double[_buffer_size];
                    offspring2 = new double[_buffer_size];
                }
                begin_index = ECFlow::get_int(0, s->getSolutionSize() - 1);
            }
        }

        void getNewIndividual(Individual* child_individual, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle) override
        {
            Solution* child = &child_individual->solution;
            Solution* s1 = &individual->solution;
            Solution** s2 = learning_object;
            preparation(child);

            if (is_crossover)
            {
                if (new_pair)
                {
                    memcpy(offspring1, s1->result, child->getSolutionSize() * sizeof(double));
                    memcpy(offspring2, s2[0]->result, child->getSolutionSize() * sizeof(double));
                    for (int i = 0; i < s1->getSolutionSize(); i++)
                        used[i] = false;

                    int current_index = begin_index;
                    bool found;
                    double corr;
                    while (true)
                    {
                        used[current_index] = true;
                        std::swap(offspring1[current_index], offspring2[current_index]);

                        found = false;
                        corr = s2[0]->result[current_index];
                        for (int i = 0; i < s1->getSolutionSize(); i++)
                        {
                            if (used[i]) continue;
                            if (equal(corr, s1->result[i])) { found = true; current_index = i; break; }
                        }
                        if (!found) break;
                    }
                    memcpy(child->result, offspring1, child->getSolutionSize() * sizeof(double));
                }
                else
                {
                    memcpy(child->result, offspring2, child->getSolutionSize() * sizeof(double));
                }
            }
            else
            {
                memcpy(child->result, s1->result, child->getSolutionSize() * sizeof(double));
            }
            ending();
        }
    };

    // 顺序交叉(OX)
    class OrderCrossover : public Crossover
    {
    private:
        int _buffer_size;
        int* matched;
        int begin_index;
        int end_index;
        double* offspring1;
        double* offspring2;

    public:
        OrderCrossover(double cross_rate = 0.9, bool coupled = true) : Crossover(cross_rate, coupled)
        {
            _buffer_size = 0;
            matched = nullptr;
            offspring1 = nullptr;
            offspring2 = nullptr;
            begin_index = -1;
            end_index = -1;
        }

        ~OrderCrossover() { delete[] matched; delete[] offspring1; delete[] offspring2; }

        static void preAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::notLessButNotice));
            if (paras[1]) list.add(new Assert(ModuleType::T_learntopology, "coupled", 1, MatchType::anyButNotice));
        }
        static void postAssert(AssertList& list, double* paras) {}

        void preparation(Solution* s) override
        {
            Crossover::preparation(s);
            if (new_pair && is_crossover)
            {
                if (_buffer_size != s->getSolutionSize())
                {
                    delete[] matched; delete[] offspring1; delete[] offspring2;
                    _buffer_size = s->getSolutionSize();
                    matched = new int[_buffer_size];
                    offspring1 = new double[_buffer_size];
                    offspring2 = new double[_buffer_size];
                }
                // 修复:原误写 `int begin_index=…`(局部遮蔽成员)→ 去 int,赋成员
                begin_index = ECFlow::get_int(0, s->getSolutionSize() - 1);
                end_index = ECFlow::get_int(0, s->getSolutionSize() - 1);
                if (end_index < begin_index) std::swap(begin_index, end_index);
                end_index++;
            }
        }

        void getNewIndividual(Individual* child_individual, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle) override
        {
            Solution* child = &child_individual->solution;
            Solution* s1 = &individual->solution;
            Solution** s2 = learning_object;
            preparation(child);

            if (is_crossover)
            {
                if (new_pair)
                {
                    int solution_size = child->getSolutionSize();
                    memcpy(offspring1, s1->result, solution_size * sizeof(double));
                    memcpy(offspring2, s2[0]->result, solution_size * sizeof(double));

                    buildMap(s1->result, s2[0]->result, solution_size, begin_index, end_index, matched);
                    int list_size = end_index - begin_index;
                    std::sort(matched, matched + list_size);

                    int o1_index = 0;
                    int o2_index = 0;
                    int match_index = 0;
                    while (true)
                    {
                        if (o1_index == begin_index) o1_index = end_index;
                        if (o1_index >= solution_size) break;

                        while (match_index < list_size && o2_index == matched[match_index]) { o2_index++; match_index++; }

                        std::swap(offspring1[o1_index], offspring2[o2_index]);
                        o1_index++;
                        o2_index++;
                    }
                    memcpy(child->result, offspring1, solution_size * sizeof(double));
                }
                else
                {
                    memcpy(child->result, offspring2, child->getSolutionSize() * sizeof(double));
                }
            }
            else
            {
                memcpy(child->result, s1->result, child->getSolutionSize() * sizeof(double));
            }
            ending();
        }
    };

    // 子路径交叉(SubtourExchange)
    class SubtourExchangeCrossover : public Crossover
    {
    private:
        int _buffer_size;
        int* matched;
        int begin_index;
        int end_index;
        double* offspring1;
        double* offspring2;

    public:
        SubtourExchangeCrossover(double cross_rate = 0.9, bool coupled = true) : Crossover(cross_rate, coupled)
        {
            _buffer_size = 0;
            matched = nullptr;
            offspring1 = nullptr;
            offspring2 = nullptr;
            begin_index = -1;
            end_index = -1;
        }

        ~SubtourExchangeCrossover() { delete[] matched; delete[] offspring1; delete[] offspring2; }

        static void preAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::notLessButNotice));
            if (paras[1]) list.add(new Assert(ModuleType::T_learntopology, "coupled", 1, MatchType::anyButNotice));
        }
        static void postAssert(AssertList& list, double* paras) {}

        void preparation(Solution* s) override
        {
            Crossover::preparation(s);
            if (new_pair && is_crossover)
            {
                if (_buffer_size != s->getSolutionSize())
                {
                    delete[] offspring1; delete[] offspring2; delete[] matched;
                    _buffer_size = s->getSolutionSize();
                    matched = new int[_buffer_size];
                    offspring1 = new double[_buffer_size];
                    offspring2 = new double[_buffer_size];
                }
                begin_index = ECFlow::get_int(0, s->getSolutionSize() - 1);
                end_index = ECFlow::get_int(0, s->getSolutionSize() - 1);
                if (end_index < begin_index) std::swap(begin_index, end_index);
                end_index++;
            }
        }

        void getNewIndividual(Individual* child_individual, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle) override
        {
            Solution* child = &child_individual->solution;
            Solution* s1 = &individual->solution;
            Solution** s2 = learning_object;
            preparation(child);

            if (is_crossover)
            {
                if (new_pair)
                {
                    int solution_size = child->getSolutionSize();
                    memcpy(offspring1, s1->result, solution_size * sizeof(double));
                    memcpy(offspring2, s2[0]->result, solution_size * sizeof(double));

                    buildMap(s1->result, s2[0]->result, solution_size, begin_index, end_index, matched);
                    int list_size = end_index - begin_index;
                    std::sort(matched, matched + list_size);

                    for (int i = 0; i < list_size; i++)
                        std::swap(offspring1[i + begin_index], offspring2[matched[i]]);

                    memcpy(child->result, offspring1, solution_size * sizeof(double));
                }
                else
                {
                    memcpy(child->result, offspring2, child->getSolutionSize() * sizeof(double));
                }
            }
            else
            {
                memcpy(child->result, s1->result, child->getSolutionSize() * sizeof(double));
            }
            ending();
        }
    };

    // 基于位置的交叉(PBX)
    class PositionBasedCrossover : public Crossover
    {
    private:
        int _buffer_size;
        int* genes;
        int* matched;
        int list_size;
        double* offspring1;
        double* offspring2;
        double _proportion;

    public:
        PositionBasedCrossover(double cross_rate = 0.9, double select_gene_proportion = 0.5, bool coupled = true)
            : Crossover(cross_rate, coupled)
        {
            _buffer_size = 0;
            _proportion = select_gene_proportion;
            genes = nullptr;
            matched = nullptr;
            list_size = 0;
            offspring1 = nullptr;
            offspring2 = nullptr;
        }

        ~PositionBasedCrossover() { delete[] genes; delete[] matched; delete[] offspring1; delete[] offspring2; }

        static void preAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::notLessButNotice));
            if (paras[2]) list.add(new Assert(ModuleType::T_learntopology, "coupled", 1, MatchType::anyButNotice)); // 修正索引:coupled 在 paras[2]
        }
        static void postAssert(AssertList& list, double* paras) {}

        void preparation(Solution* s) override
        {
            Crossover::preparation(s);
            if (new_pair && is_crossover)
            {
                if (_buffer_size != s->getSolutionSize())
                {
                    delete[] genes; delete[] matched; delete[] offspring1; delete[] offspring2;
                    _buffer_size = s->getSolutionSize();
                    genes = new int[_buffer_size];
                    matched = new int[_buffer_size];
                    offspring1 = new double[_buffer_size];
                    offspring2 = new double[_buffer_size];
                }
                list_size = 0;
                for (int i = 0; i < s->getSolutionSize(); i++)
                    if (rand01() < _proportion) { genes[list_size] = i; list_size++; }
            }
        }

        void getNewIndividual(Individual* child_individual, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle) override
        {
            Solution* child = &child_individual->solution;
            Solution* s1 = &individual->solution;
            Solution** s2 = learning_object;
            preparation(child);

            if (is_crossover)
            {
                if (new_pair)
                {
                    int solution_size = child->getSolutionSize();
                    memcpy(offspring1, s1->result, solution_size * sizeof(double));
                    memcpy(offspring2, s2[0]->result, solution_size * sizeof(double));

                    buildMap(s1->result, s2[0]->result, solution_size, genes, matched, list_size);
                    std::sort(matched, matched + list_size);

                    int gene_index = 0;
                    for (int i = 0; i < solution_size && gene_index < list_size; i++)   // list_size==0 时不进循环(守卫未初始化 genes/matched)
                    {
                        if (i == genes[gene_index])
                        {
                            std::swap(offspring1[i], offspring2[matched[gene_index]]);
                            gene_index++;
                        }
                    }
                    memcpy(child->result, offspring1, solution_size * sizeof(double));
                }
                else
                {
                    memcpy(child->result, offspring2, child->getSolutionSize() * sizeof(double));
                }
            }
            else
            {
                memcpy(child->result, s1->result, child->getSolutionSize() * sizeof(double));
            }
            ending();
        }
    };

    // 自注册:5 个序列交叉。coupled 参数在末位(bool,以 Enum 0/1 表)。
    inline Registry<LearningStrategy>::Entry partialMappedCrossoverEntry()
    {
        return { "PartialMapped", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"cross_rate", ParamKind::Real, 0.0, 1.0, false, 0.6, 0.95}, {"coupled", ParamKind::Enum, 0, 1, false, 1, 1} } }, sizeof(PartialMappedCrossover),
            [](const double* p) -> LearningStrategy* { return p ? new PartialMappedCrossover(p[0], p[1] != 0) : new PartialMappedCrossover(); },
            [](AssertList& L, const double* p) { PartialMappedCrossover::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { PartialMappedCrossover::postAssert(L, const_cast<double*>(p)); } };
    }
    inline Registry<LearningStrategy>::Entry cycleCrossoverEntry()
    {
        return { "Cycle", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"cross_rate", ParamKind::Real, 0.0, 1.0, false, 0.6, 0.95}, {"coupled", ParamKind::Enum, 0, 1, false, 1, 1} } }, sizeof(CycleCrossover),
            [](const double* p) -> LearningStrategy* { return p ? new CycleCrossover(p[0], p[1] != 0) : new CycleCrossover(); },
            [](AssertList& L, const double* p) { CycleCrossover::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { CycleCrossover::postAssert(L, const_cast<double*>(p)); } };
    }
    inline Registry<LearningStrategy>::Entry orderCrossoverEntry()
    {
        return { "Order", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"cross_rate", ParamKind::Real, 0.0, 1.0, false, 0.6, 0.95}, {"coupled", ParamKind::Enum, 0, 1, false, 1, 1} } }, sizeof(OrderCrossover),
            [](const double* p) -> LearningStrategy* { return p ? new OrderCrossover(p[0], p[1] != 0) : new OrderCrossover(); },
            [](AssertList& L, const double* p) { OrderCrossover::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { OrderCrossover::postAssert(L, const_cast<double*>(p)); } };
    }
    inline Registry<LearningStrategy>::Entry subtourExchangeCrossoverEntry()
    {
        return { "SubtourExchange", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"cross_rate", ParamKind::Real, 0.0, 1.0, false, 0.6, 0.95}, {"coupled", ParamKind::Enum, 0, 1, false, 1, 1} } }, sizeof(SubtourExchangeCrossover),
            [](const double* p) -> LearningStrategy* { return p ? new SubtourExchangeCrossover(p[0], p[1] != 0) : new SubtourExchangeCrossover(); },
            [](AssertList& L, const double* p) { SubtourExchangeCrossover::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { SubtourExchangeCrossover::postAssert(L, const_cast<double*>(p)); } };
    }
    inline Registry<LearningStrategy>::Entry positionBasedCrossoverEntry()
    {
        return { "PositionBased", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"cross_rate", ParamKind::Real, 0.0, 1.0, false, 0.6, 0.95}, {"proportion", ParamKind::Real, 0.0, 1.0, false, 0.3, 0.6}, {"coupled", ParamKind::Enum, 0, 1, false, 1, 1} } }, sizeof(PositionBasedCrossover),
            [](const double* p) -> LearningStrategy* { return p ? new PositionBasedCrossover(p[0], p[1], p[2] != 0) : new PositionBasedCrossover(); },
            [](AssertList& L, const double* p) { PositionBasedCrossover::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { PositionBasedCrossover::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(lstrat_cross_partialmapped,   LearningStrategy, partialMappedCrossoverEntry());
    ECFLOW_REGISTER(lstrat_cross_cycle,           LearningStrategy, cycleCrossoverEntry());
    ECFLOW_REGISTER(lstrat_cross_order,           LearningStrategy, orderCrossoverEntry());
    ECFLOW_REGISTER(lstrat_cross_subtourexchange, LearningStrategy, subtourExchangeCrossoverEntry());
    ECFLOW_REGISTER(lstrat_cross_positionbased,   LearningStrategy, positionBasedCrossoverEntry());
}
