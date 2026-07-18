//------------------------Description------------------------
// 配偶选择类学习拓扑三种(GA 常用):
//   Roulette   轮盘赌 / 适应度比例选择。
//   Tournament 锦标赛 / 竞赛选择(原类名 Championship)。
//   Uniform    均匀随机选择。
// 三者均成对产边:选出的一对个体互为学习对象(+ 若干额外对象)。
//-------------------------Reference-------------------------
// [1] D. E. Goldberg and K. Deb, "A Comparative Analysis of Selection Schemes Used in Genetic
//     Algorithms," in Foundations of Genetic Algorithms, G. Rawlins, Ed. San Francisco, CA, USA:
//     Morgan Kaufmann, 1991, pp. 69-93.(涵盖 proportionate/轮盘赌 与 tournament;Uniform 为基础基线)
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "ecflow-constant.h"
#include "ecflow-rand.h"
#include "learning-topology.h"
#include "registry.h"

namespace ECFlow
{
    // 单目标单种群轮盘赌选择
    class Roulette : public LearningTopology
    {
    private:
        double* _dart_board;
        int _board_size;

        int _objects;
        int* _id_list;

        void setSize(int size)
        {
            if (_board_size != size)
            {
                delete[] _dart_board;
                _board_size = size;
                _dart_board = new double[_board_size];
            }
        }

    public:
        Roulette(int object_number = 1) : LearningTopology()
        {
            _objects = object_number;
            _id_list = new int[object_number + 2];
            _dart_board = nullptr;
            _board_size = 0;
        }

        ~Roulette()
        {
            delete[] _dart_board;
            delete[] _id_list;
        }

        static void preAssert(AssertList& list, double* paras) {}

        static void postAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", int(paras[0]), MatchType::postAssert));
            list.add(new Assert(ModuleType::T_learntopology, "coupled", 1, MatchType::postAssert));
            list.add(new Assert(ModuleType::T_learntopology, "graphScale", 100, MatchType::postAssert));
        }

        void ini() override {}

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** best_holder,
                                   const int swarm_number, IndividualArray* offspring, LearningGraph* last_graph) override
        {
            IndividualArray* cswarm = subswarm[0];
            int graph_size = cswarm->getSize();
            setSize(graph_size);
            LearningGraph* back = new LearningGraph(graph_size, _objects);

            // 统计最大最小 fitness[0](内建"最小化"假设,见 ROULETTE-DIR)
            double max = -1;
            double min = ECFLOW_MAX;
            for (int i = 0; i < graph_size; i++)
            {
                if (max < cswarm[0][i].solution.fitness[0])
                    max = cswarm[0][i].solution.fitness[0];
                if (min > cswarm[0][i].solution.fitness[0])
                    min = cswarm[0][i].solution.fitness[0];
            }
            max *= 1.001; // 保证归一化后最差个体非 0

            // 归一化(B 护栏:range<=0 → 均匀权重,防除零 NaN)
            double range = max - min;
            double total = 0;
            if (range <= 0)
            {
                for (int i = 0; i < graph_size; i++) { _dart_board[i] = 1.0; total += 1.0; }
            }
            else
            {
                for (int i = 0; i < graph_size; i++)
                {
                    _dart_board[i] = (max - cswarm[0][i].solution.fitness[0]) / range;
                    total += _dart_board[i];
                }
            }
            for (int i = 0; i < graph_size; i++)
                _dart_board[i] /= total;

            // 轮盘赌(rand01 = ECFlow 引擎,可复现)
            double p0 = rand01();
            int id = 0;
            int counter = 0;
            Individual* s1;
            Solution* s2;
            while (counter < graph_size)
            {
                for (int i = 0; i < _objects + 1; i++)
                {
                    while (p0 > 0)
                    {
                        id++;
                        if (id == graph_size)
                            id = 0;
                        p0 -= _dart_board[id];
                    }
                    _id_list[i] = id;
                    p0 += rand01();
                }

                // 插入拓扑图
                s1 = &cswarm[0][_id_list[0]];
                back->addStart(s1);
                s2 = &cswarm[0][_id_list[1]].solution;
                back->addEnd(s2);
                for (int i = 1; i < _objects; i++)
                {
                    s2 = &cswarm[0][_id_list[i + 1]].solution;
                    back->addEnd(s2);
                }
                counter++;

                if (counter == graph_size)
                    break;

                // 插入对称对象
                s1 = &cswarm[0][_id_list[1]];
                back->addStart(s1);
                s2 = &cswarm[0][_id_list[0]].solution;
                back->addEnd(s2);
                for (int i = 1; i < _objects; i++)
                {
                    s2 = &cswarm[0][_id_list[i + 1]].solution;
                    back->addEnd(s2);
                }
                counter++;
            }

            return back;
        }
    };

    // 锦标赛 / 竞赛选择(原 Championship)
    class Tournament : public LearningTopology
    {
    private:
        int _competition_scale;
        int _objects_number;
        int* _id_list;

    public:
        Tournament(int n = 2, int object_number = 1) : LearningTopology()
        {
            _competition_scale = n;
            _objects_number = object_number;
            _id_list = new int[object_number + 2];
        }

        ~Tournament() { delete[] _id_list; }

        static void preAssert(AssertList& list, double* paras) {}

        static void postAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", int(paras[1]), MatchType::postAssert));
            list.add(new Assert(ModuleType::T_learntopology, "coupled", 1, MatchType::postAssert));
            list.add(new Assert(ModuleType::T_learntopology, "graphScale", 100, MatchType::postAssert));
        }

        void ini() override {}

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** best_holder,
                                   const int swarm_number, IndividualArray* offspring, LearningGraph* last_graph) override
        {
            IndividualArray* cswarm = subswarm[0]; // 当前种群
            int graph_size = cswarm->getSize();
            LearningGraph* back = new LearningGraph(graph_size, _objects_number);

            int counter = 0;
            int winner, challenger;
            Individual* s1;
            Solution* s2;
            while (counter < graph_size)
            {
                // 锦标赛
                for (int i = 0; i < _objects_number + 1; i++)
                {
                    winner = ECFlow::get_int(0, graph_size - 1);
                    for (int j = 1; j < _competition_scale; j++)   // 内层改名 j(原遮蔽外层 i)
                    {
                        challenger = ECFlow::get_int(0, graph_size - 1);
                        if ((*cswarm)[challenger] < (*cswarm)[winner])
                            winner = challenger;
                    }
                    _id_list[i] = winner;
                }

                // 插入拓扑图
                s1 = &(*cswarm)[_id_list[0]];
                back->addStart(s1);
                s2 = &(*cswarm)[_id_list[1]].solution;
                back->addEnd(s2);
                for (int i = 1; i < _objects_number; i++)
                {
                    s2 = &cswarm[0][_id_list[i + 1]].solution;
                    back->addEnd(s2);
                }
                counter++;

                if (counter == graph_size)
                    break;

                // 插入对称对象
                s1 = &(*cswarm)[_id_list[1]];
                back->addStart(s1);
                s2 = &(*cswarm)[_id_list[0]].solution;
                back->addEnd(s2);
                for (int i = 1; i < _objects_number; i++)
                {
                    s2 = &cswarm[0][_id_list[i + 1]].solution;
                    back->addEnd(s2);
                }
                counter++;
            }

            return back;
        }
    };

    // 均匀随机选择
    class Uniform : public LearningTopology
    {
    private:
        int _objects_number;
        int* _id_list;

    public:
        Uniform(int object_number = 1) : LearningTopology()
        {
            _objects_number = object_number;
            _id_list = new int[object_number + 2];
        }

        ~Uniform() { delete[] _id_list; }

        static void preAssert(AssertList& list, double* paras) {}

        static void postAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", int(paras[0]), MatchType::postAssert));
            list.add(new Assert(ModuleType::T_learntopology, "coupled", 1, MatchType::postAssert));
            list.add(new Assert(ModuleType::T_learntopology, "graphScale", 100, MatchType::postAssert));
        }

        void ini() override {}

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** best_holder,
                                   const int swarm_number, IndividualArray* offspring, LearningGraph* last_graph) override
        {
            IndividualArray* cswarm = subswarm[0];
            int graph_size = cswarm->getSize();
            LearningGraph* back = new LearningGraph(graph_size, _objects_number);

            int counter = 0;
            Individual* s1;
            Solution* s2;
            while (counter < graph_size)
            {
                // 均匀选取
                for (int i = 0; i < _objects_number + 1; i++)
                    _id_list[i] = ECFlow::get_int(0, graph_size - 1);

                // 插入拓扑图
                s1 = &cswarm[0][_id_list[0]];
                back->addStart(s1);
                s2 = &cswarm[0][_id_list[1]].solution;
                back->addEnd(s2);
                for (int i = 1; i < _objects_number; i++)
                {
                    s2 = &cswarm[0][_id_list[i + 1]].solution;
                    back->addEnd(s2);
                }
                counter++;

                if (counter == graph_size)
                    break;

                // 插入对称对象
                s1 = &cswarm[0][_id_list[1]];
                back->addStart(s1);
                s2 = &cswarm[0][_id_list[0]].solution;
                back->addEnd(s2);
                for (int i = 1; i < _objects_number; i++)
                {
                    s2 = &cswarm[0][_id_list[i + 1]].solution;
                    back->addEnd(s2);
                }
                counter++;
            }

            return back;
        }
    };

    inline Registry<LearningTopology>::Entry rouletteTopologyEntry()
    {
        return { "Roulette", ModuleType::T_learntopology,
            ParameterTemplate{ { {"object_number", ParamKind::Int, 1, 0x3f3f3f3f, false, 1, 3} } }, sizeof(Roulette),
            [](const double* p) -> LearningTopology* { return new Roulette(p ? int(p[0]) : 1); },
            [](AssertList& L, const double* p) { Roulette::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { Roulette::postAssert(L, const_cast<double*>(p)); } };
    }
    inline Registry<LearningTopology>::Entry tournamentTopologyEntry()
    {
        return { "Tournament", ModuleType::T_learntopology,
            ParameterTemplate{ { {"competition_scale", ParamKind::Int, 1, 0x3f3f3f3f, false, 2, 5},
                                 {"object_number", ParamKind::Int, 1, 0x3f3f3f3f, false, 1, 3} } }, sizeof(Tournament),
            [](const double* p) -> LearningTopology* { return new Tournament(p ? int(p[0]) : 2, p ? int(p[1]) : 1); },
            [](AssertList& L, const double* p) { Tournament::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { Tournament::postAssert(L, const_cast<double*>(p)); } };
    }
    inline Registry<LearningTopology>::Entry uniformTopologyEntry()
    {
        return { "Uniform", ModuleType::T_learntopology,
            ParameterTemplate{ { {"object_number", ParamKind::Int, 1, 0x3f3f3f3f, false, 1, 3} } }, sizeof(Uniform),
            [](const double* p) -> LearningTopology* { return new Uniform(p ? int(p[0]) : 1); },
            [](AssertList& L, const double* p) { Uniform::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { Uniform::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(ltopo_roulette,   LearningTopology, rouletteTopologyEntry());
    ECFLOW_REGISTER(ltopo_tournament, LearningTopology, tournamentTopologyEntry());
    ECFLOW_REGISTER(ltopo_uniform,    LearningTopology, uniformTopologyEntry());
}
