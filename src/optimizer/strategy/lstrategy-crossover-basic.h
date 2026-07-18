//------------------------Description------------------------
// 基础交叉算子:PointCrossover(点交叉)/ UniformCrossover(均匀交叉)/ SBXCrossover(模拟二进制交叉)/ DifferenceCrossover(差分交叉)。
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
#include "ecflow-constant.h"
#include "ecflow-basicfunc.h"
#include "individual.h"
#include "problem-handle.h"
#include "comparer.hpp"
#include "lstrategy-crossover.h"
#include "registry.h"

namespace ECFlow
{
    // 点交叉:交换交叉点位之间的片段
    class PointCrossover : public Crossover
    {
    private:
        int _point_number;
        int* points;

        bool isRepeat(int index)
        {
            for (int i = 0; i < index; i++)
                if (points[i] == points[index]) return true;
            return false;
        }

    public:
        PointCrossover(int point_number = 1, double cross_rate = 0.9, bool coupled = true)
            : Crossover(cross_rate, coupled)
        {
            points = new int[point_number];
            _point_number = point_number;
        }

        ~PointCrossover() { delete[] points; }

        static void preAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::notLessButNotice)); // 需要 1 个学习目标
            if (paras[2]) // coupled(修正索引:Point 的 coupled 在 paras[2])
                list.add(new Assert(ModuleType::T_learntopology, "coupled", 1, MatchType::anyButNotice));
        }
        static void postAssert(AssertList& list, double* paras) {}

        void preparation(Solution* s) override
        {
            Crossover::preparation(s);
            if (new_pair && is_crossover)
            {
                for (int i = 0; i < _point_number; i++)
                {
                    points[i] = ECFlow::get_int(0, s->getSolutionSize() - 1);
                    while (isRepeat(i))
                        points[i] = ECFlow::get_int(0, s->getSolutionSize() - 1);
                }
                std::sort(points, points + _point_number);
            }
        }

        void getNewIndividual(Individual* child_individual, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle) override
        {
            Solution* child = &child_individual->solution;
            Solution* s1 = &individual->solution;
            Solution** s2 = learning_object;

            preparation(s1);

            if (is_crossover)
            {
                int position = 0;
                int length;
                bool is_s1 = true;
                for (int i = 0; i < _point_number; i++)
                {
                    length = points[i] - position;
                    if (is_s1)
                        memcpy(child->result + position, s1->result + position, length * sizeof(double));
                    else
                        memcpy(child->result + position, s2[0]->result + position, length * sizeof(double));
                    position = points[i];
                    is_s1 = !is_s1;
                }
                // 最后一段
                length = child->getSolutionSize() - position;
                if (is_s1)
                    memcpy(child->result + position, s1->result + position, length * sizeof(double));
                else
                    memcpy(child->result + position, s2[0]->result + position, length * sizeof(double));
            }
            else // 不交叉,直接继承亲本
            {
                memcpy(child->result, s1->result, child->getSolutionSize() * sizeof(double));
            }

            ending();
        }
    };

    // 均匀交叉:逐位随机取自 s1 或 s2
    class UniformCrossover : public Crossover
    {
    private:
        bool* _is_s1;
        int _buffer_size;

    public:
        UniformCrossover(double cross_rate = 0.9, bool coupled = true)
            : Crossover(cross_rate, coupled)
        {
            _buffer_size = 0;
            _is_s1 = nullptr;
        }

        ~UniformCrossover() { delete[] _is_s1; }

        static void preAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::notLessButNotice)); // 需要 1 个学习目标
            if (paras[1]) // coupled(Uniform 的 coupled 在 paras[1])
                list.add(new Assert(ModuleType::T_learntopology, "coupled", 1, MatchType::anyButNotice));
        }
        static void postAssert(AssertList& list, double* paras) {}

        void preparation(Solution* s) override
        {
            Crossover::preparation(s);
            if (new_pair && is_crossover)
            {
                if (_buffer_size != s->getSolutionSize())
                {
                    delete[] _is_s1;
                    _buffer_size = s->getSolutionSize();
                    _is_s1 = new bool[_buffer_size];
                }
                for (int i = 0; i < s->getSolutionSize(); i++)
                    _is_s1[i] = rand01() < 0.5;
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
                for (int i = 0; i < child->getSolutionSize(); i++)
                    child->result[i] = _is_s1[i] ? s1->result[i] : s2[0]->result[i];
            }
            else
            {
                memcpy(child->result, s1->result, child->getSolutionSize() * sizeof(double));
            }

            ending();
        }
    };

    // 模拟二进制交叉(Simulated Binary Crossover)
    class SBXCrossover : public Crossover
    {
    private:
        double _eta;
        double _r;
        double _belta;
        double* _c2_buffer;
        int buffer_size;

    public:
        SBXCrossover(double eta = 20, double cross_rate = 0.9, bool coupled = true)
            : Crossover(cross_rate, coupled)
        {
            _eta = eta;
            buffer_size = 0;
            _r = 0;
            _belta = 0;
            _c2_buffer = nullptr;
        }

        ~SBXCrossover() { delete[] _c2_buffer; }

        static void preAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::notLessButNotice)); // 需要 1 个学习目标
            if (paras[2]) // coupled(SBX 的 coupled 在 paras[2])
                list.add(new Assert(ModuleType::T_learntopology, "coupled", 1, MatchType::anyButNotice));
        }
        static void postAssert(AssertList& list, double* paras) {}

        void preparation(Solution* s) override
        {
            Crossover::preparation(s);
            if (new_pair && is_crossover)
            {
                if (buffer_size != s->getSolutionSize())
                {
                    delete[] _c2_buffer;
                    buffer_size = s->getSolutionSize();
                    _c2_buffer = new double[buffer_size];
                }
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
                if (new_pair) // 新亲本对,计算 c1 与 c2(c2 存缓冲,下次调用返回)
                {
                    for (int i = 0; i < child->getSolutionSize(); i++)
                    {
                        if (rand01() < 0.5) // 该片段不交叉
                        {
                            _c2_buffer[i] = s2[0]->result[i];
                            child->result[i] = s1->result[i];
                        }
                        else
                        {
                            _r = rand01_();
                            if (_r > 0.5)
                                _belta = pow(2 - _r * 2, -1 / (1 + _eta));
                            else
                                _belta = pow(_r * 2, 1 / (1 + _eta));

                            if (rand01() > 0.5)
                                _belta *= -1;

                            child->result[i] = 0.5 * ((1 + _belta) * s1->result[i] + (1 - _belta) * s2[0]->result[i]);
                            _c2_buffer[i] = 0.5 * ((1 - _belta) * s1->result[i] + (1 + _belta) * s2[0]->result[i]);
                        }
                    }
                }
                else // 返回上次算好的 c2
                {
                    memcpy(child->result, _c2_buffer, sizeof(double) * child->getSolutionSize());
                }
            }
            else
            {
                memcpy(child->result, s1->result, child->getSolutionSize() * sizeof(double));
            }

            ending();
        }
    };

    // 差分交叉
    class DifferenceCrossover : public Crossover
    {
    private:
        double _zoom_factor;
        bool adaptive_factor;
        Comparer* _comparer_pointer;

    public:
        DifferenceCrossover(double cross_rate = 0.9, double factor = 0.5, bool coupled = true)
            : Crossover(cross_rate, coupled)
        {
            _zoom_factor = factor;
            adaptive_factor = is_empty(_zoom_factor);   // 修复:原 _zoom_factor==EMPTYVALUE 恒假 → 自适应永不启用
            _comparer_pointer = nullptr;
        }

        ~DifferenceCrossover() {}

        static void preAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 2, MatchType::notLessButNotice)); // 需要 2 个学习目标
        }
        static void postAssert(AssertList& list, double* paras) {}

        void setProblem(ProblemHandle* problem_handle) override
        {
            _comparer_pointer = problem_handle->getSolutionComparer();
        }

        void getNewIndividual(Individual* child_individual, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle) override
        {
            Solution* child = &child_individual->solution;
            Solution* s1 = &individual->solution;
            Solution** s2 = learning_object;

            Crossover::preparation(child);

            if (is_crossover)
            {
                Solution* s_best = s1;
                Solution* s_middle = s2[0];
                Solution* s_worst = s2[1];

                // 按适应度排序 best/middle/worst
                if (_comparer_pointer->isBetter(s_middle->fitness, s_best->fitness))
                    std::swap(s_middle, s_best);
                if (_comparer_pointer->isBetter(s_worst->fitness, s_middle->fitness))
                    std::swap(s_worst, s_middle);
                if (_comparer_pointer->isBetter(s_middle->fitness, s_best->fitness))
                    std::swap(s_middle, s_best);

                if (adaptive_factor) // 缩放因子自适应:Fi = Fl + (Fu-Fl)*(fm-fb)/(fw-fb)
                    _zoom_factor = 0.1 + 0.8 * (s_middle->fitness[0] - s_best->fitness[0]) / (s_worst->fitness[0] - s_best->fitness[0]);

                for (int i = 0; i < child->getSolutionSize(); i++)
                    child->result[i] = s_best->result[i] + _zoom_factor * (s_middle->result[i] - s_worst->result[i]);
            }
            else
            {
                memcpy(child->result, s1->result, child->getSolutionSize() * sizeof(double));
            }

            ending();
        }
    };

    inline Registry<LearningStrategy>::Entry pointCrossoverEntry()
    {
        return { "Point", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"point_number", ParamKind::Int, 1, 0x3f3f3f3f, false, 1, 3}, {"cross_rate", ParamKind::Real, 0.0, 1.0, false, 0.6, 0.95}, {"coupled", ParamKind::Enum, 0, 1, false, 1, 1} } }, sizeof(PointCrossover),
            [](const double* p) -> LearningStrategy* { return p ? new PointCrossover(int(p[0]), p[1], p[2] != 0) : new PointCrossover(); },
            [](AssertList& L, const double* p) { PointCrossover::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { PointCrossover::postAssert(L, const_cast<double*>(p)); } };
    }
    inline Registry<LearningStrategy>::Entry uniformCrossoverEntry()
    {
        return { "Uniform", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"cross_rate", ParamKind::Real, 0.0, 1.0, false, 0.6, 0.95}, {"coupled", ParamKind::Enum, 0, 1, false, 1, 1} } }, sizeof(UniformCrossover),
            [](const double* p) -> LearningStrategy* { return p ? new UniformCrossover(p[0], p[1] != 0) : new UniformCrossover(); },
            [](AssertList& L, const double* p) { UniformCrossover::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { UniformCrossover::postAssert(L, const_cast<double*>(p)); } };
    }
    inline Registry<LearningStrategy>::Entry sbxCrossoverEntry()
    {
        return { "SBX", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"eta", ParamKind::Real, 0.0, 100.0, false, 15, 20}, {"cross_rate", ParamKind::Real, 0.0, 1.0, false, 0.6, 0.95}, {"coupled", ParamKind::Enum, 0, 1, false, 1, 1} } }, sizeof(SBXCrossover),
            [](const double* p) -> LearningStrategy* { return p ? new SBXCrossover(p[0], p[1], p[2] != 0) : new SBXCrossover(); },
            [](AssertList& L, const double* p) { SBXCrossover::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { SBXCrossover::postAssert(L, const_cast<double*>(p)); } };
    }
    inline Registry<LearningStrategy>::Entry differenceCrossoverEntry()
    {
        return { "Difference", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"cross_rate", ParamKind::Real, 0.0, 1.0, false, 0.6, 0.95}, {"factor", ParamKind::Real, 0.0, 10.0, true, 0.4, 0.9}, {"coupled", ParamKind::Enum, 0, 1, false, 1, 1} } }, sizeof(DifferenceCrossover),
            [](const double* p) -> LearningStrategy* { return p ? new DifferenceCrossover(p[0], p[1], p[2] != 0) : new DifferenceCrossover(); },
            [](AssertList& L, const double* p) { DifferenceCrossover::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { DifferenceCrossover::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(lstrat_cross_point,      LearningStrategy, pointCrossoverEntry());
    ECFLOW_REGISTER(lstrat_cross_uniform,    LearningStrategy, uniformCrossoverEntry());
    ECFLOW_REGISTER(lstrat_cross_sbx,        LearningStrategy, sbxCrossoverEntry());
    ECFLOW_REGISTER(lstrat_cross_difference, LearningStrategy, differenceCrossoverEntry());
}
