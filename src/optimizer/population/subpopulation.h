//------------------------Description------------------------
// 子种群 Subpopulation:统辖一组个体全部活动的基本单元——子代生成、环境选择等。
//   持亲代/子代两个 IndividualArray + 一条 O_Workflow(编排流水线) + 档案/终止器。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <memory>

#include "solution.h"
#include "individual-array.h"
#include "best-archive.h"
#include "terminator.h"
#include "optimize-workflow.h"
#include "problem-handle.h"
#include "ecflow-rand.h"
#include "logger.hpp"   // v3.6 LOG:接通每优化器 Logger(共用件 src/logger/)

namespace ECFlow
{

    class Subpopulation
    {
    private:
        std::string _id;
        IndividualArray* _parent;      // 用户对 population 的直接修改可能致 best 不准,故内部持独立亲代
        IndividualArray* _offspring;

        BestArchive* _best_archive;
        Terminator*  _terminator;
        std::unique_ptr<O_Workflow> workflow;

        Comparer* _comparer_pointer;   // 见头注:原代码设而未用

        IndividualArray** _parent_buffer;
        BestArchive**     _archives_buffer;
        int _buffers_size;

        Logger* _logger_point;         // 不拥有

    public:
        Subpopulation(std::string id, IndividualArray* parent, IndividualArray* offspring, Terminator* terminator, BestArchive* best_archive, Logger* logger)
        {
            _id = id;
            _parent = parent;
            _offspring = offspring;
            _best_archive = best_archive;

            _terminator = terminator;

            _comparer_pointer = nullptr;

            _parent_buffer = nullptr;
            _archives_buffer = nullptr;
            _buffers_size = 0;

            _logger_point = logger;
        }

        ~Subpopulation()
        {
            delete _parent;
            delete _offspring;
            delete _best_archive;
            delete _terminator;
            delete[] _parent_buffer;
            delete[] _archives_buffer;
        }

        void setProblem(ProblemHandle* problem_hanlde)
        {
            _parent->setProblem(problem_hanlde);
            _offspring->setProblem(problem_hanlde);
            _best_archive->setProblem(problem_hanlde);
            workflow->setProblem(problem_hanlde);
            _comparer_pointer = problem_hanlde->getSolutionComparer();
        }

        void setWorkflow(std::unique_ptr<O_Workflow> workflow)
        {
            this->workflow = std::move(workflow);

            for (int i = 0; i < _parent->getSize(); i++)
                (*_parent)[i].setInitializer(this->workflow->getSolutionInitializer());

            for (int i = 0; i < _offspring->getSize(); i++)
                (*_offspring)[i].setInitializer(this->workflow->getSolutionInitializer());
        }

        Terminator* getTerminator()
        {
            return _terminator;
        }

        void resetTerminator()
        {
            _terminator->reset();
        }

        bool getBest(Solution*& best_pointer, int& best_size)
        {
            best_pointer = _best_archive->getBest();
            best_size = _best_archive->getBestSize();
            return true;
        }

        int getSize()
        {
            return _parent->getSize();
        }

        std::string getID()
        {
            return _id;
        }

        void ini()
        {
            _best_archive->clear();

            // offspring 是每迭代 scratch:workflow 在迭代末清空,但开跑前也须为空。
            //   若不清,首迭代 generator 的 extend 会从初始 size 之上追加 → 混入未初始化个体(评估垃圾解致崩溃)。
            //   此时 setProblem 已把 buffer 内个体配置好,clear 仅压 size→0,后续 extend 复用已配置个体
            //   (兼避 IARR size-0 extend 的"未配置模板"深层崩溃)。[迁移修复:原代码首迭代处理垃圾 offspring 的潜在 bug]
            _offspring->clear();

            // 初始种群:**逐个体**初始化(含评估)→ 更新档案 → 计 FES,且**每个体前查终止**。
            //   期间不看预算 → **先花后记**:种群 400 而剩余预算 200 时,400 次评估已实际发生,
            //   `termination()` 直到 ini 结束才可能为真 → **超额 100%**。
            //   (对照:Evaluator/workflow 早已逐个体、逐段查终止,唯独 ini 走的是**独立于 Evaluator 的
            //    评估路径**——自己调 Individual::ini(evaluate=true)——于是没有那道检查。)
            //   预算不足时种群**部分初始化**是可接受的:用户取的是**档案中的最优**,未初始化的尾部个体
            //   不参与结果;且此时预算已尽、本就不会再迭代。
            //   注:`_terminator->update(true)` 的硬编码 true 是**有意设计**(初始种群不计入收敛停滞),非 bug,保留。
            for (int i = 0; i < _parent->getSize(); i++)
            {
                if (_terminator->termination()) break;   // ★ 预算耗尽 → 停止初始化剩余个体
                (*_parent)[i].ini(true, true, true);     // 逐个体:初始化解 + 评估 + 初始化特性
                _best_archive->updateBest((*_parent)[i].solution);
                _terminator->update(true);
            }
            workflow->ini();
        }

        void nextIteration()
        {
            workflow->run(_terminator, _offspring, _parent_buffer, _archives_buffer, 1);
        }

        void setNeibors(Subpopulation** neighbors, const int swarm_number)
        {
            if (_buffers_size != swarm_number + 1)
            {
                delete[] _parent_buffer;
                delete[] _archives_buffer;
                _buffers_size = swarm_number + 1;
                _parent_buffer = new IndividualArray * [_buffers_size + 3];
                _archives_buffer = new BestArchive * [_buffers_size + 3];
                _parent_buffer[0] = _parent;
                _archives_buffer[0] = _best_archive;
            }

            for (int i = 0; i < swarm_number; i++)
            {
                _parent_buffer[i + 1] = neighbors[i]->_parent;
                _archives_buffer[i + 1] = neighbors[i]->_best_archive;
            }

            _parent_buffer[swarm_number + 1] = nullptr;
            _archives_buffer[swarm_number + 1] = nullptr;
        }

        void run()
        {
            while (!_terminator->termination())
            {
                nextIteration();

                if (_logger_point != nullptr && _logger_point->process_print())
                {
                    int size = 0;
                    Solution* bests = nullptr;
                    getBest(bests, size);
                    for (int i = 0; i < size; i++)
                        _logger_point->logprocess(bests[i].decoder_pointer->toString(
                            bests[i].result, bests[i].fitness, _logger_point->full_process_print()));

                    if (_logger_point->swarm_print())
                    {
                        int psize = _parent->getSize();
                        for (int i = 0; i < psize; i++)
                        {
                            // 迁移修复:原用 bests[i].result/fitness + 亲代 decoder(混用且 i 可越 bests 界)→ 改用亲代个体自身解
                            Solution& s = (*_parent)[i].solution;
                            _logger_point->logprocess(s.decoder_pointer->toString(
                                s.result, s.fitness, _logger_point->full_process_print()));
                        }
                    }
                }
            }
        }

        // 替换随机个体
        void replaceIndividualR(Individual* individual)
        {
            int id = get_int(0, _parent->getSize() - 1);
            _parent[0][id].copy(*individual);
        }

        // 替换最优个体
        void replaceIndividualB(Individual* individual)
        {
            Individual* s = &(_parent[0][0]);
            for (int i = 1; i < _parent->getSize(); i++)
            {
                if (_parent[0][i] < *s)
                    s = &(_parent[0][i]);
            }
            s->copy(*individual);
        }

        // 替换最差个体
        void replaceIndividualW(Individual* individual)
        {
            Individual* s = &(_parent[0][0]);
            for (int i = 1; i < _parent->getSize(); i++)
            {
                if (*s < _parent[0][i])
                    s = &(_parent[0][i]);
            }
            s->copy(*individual);
        }

        // 替换随机个体(解)
        void replaceIndividualR(Solution* solution)
        {
            int id = get_int(0, _parent->getSize() - 1);
            _parent[0][id].solution.copy(*solution);
        }

        // 替换最优个体(解)
        void replaceIndividualB(Solution* solution)
        {
            Individual* s = &(_parent[0][0]);
            for (int i = 1; i < _parent->getSize(); i++)
            {
                if (_parent[0][i] < *s)
                    s = &(_parent[0][i]);
            }
            s->solution.copy(*solution);
        }

        // 替换最差个体(解)
        void replaceIndividualW(Solution* solution)
        {
            Individual* s = &(_parent[0][0]);
            for (int i = 1; i < _parent->getSize(); i++)
            {
                if (*s < _parent[0][i])
                    s = &(_parent[0][i]);
            }
            s->solution.copy(*solution);
        }

        // 获取当前种群中的最优个体
        Individual* getBestIndividualInSwarm()
        {
            Individual* s = &(_parent[0][0]);
            for (int i = 1; i < _parent->getSize(); i++)
            {
                if (_parent[0][i] < *s)
                    s = &(_parent[0][i]);
            }
            return s;
        }

        void addIndividual(Individual* individuals, const int number)
        {
            _parent->append(individuals, number);
        }

        void removeIndividual(Individual* individuals, const int number)
        {
            _parent->remove(individuals, number);
        }

        void removeIndividual(int* individual_index, const int number)
        {
            _parent->remove(individual_index, number);
        }

        Individual& operator[](int i)
        {
            return (*_parent)[i];
        }
    };
}
