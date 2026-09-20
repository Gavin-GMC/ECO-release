//------------------------Description------------------------
// 子种群 Subpopulation:统辖一组个体全部活动的基本单元——子代生成、环境选择等。
//   持亲代/子代两个 IndividualArray + 一条 O_Workflow(编排流水线) + 档案/终止器。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <memory>

#include "solution.h"
#include "individual-array.h"
#include "best-archive.h"
#include "terminator.h"
#include "optimize-workflow.h"
#include "problem-handle.h"
#include "evoflow-rand.h"
#include "logger.hpp"   // v3.6 LOG:接通每优化器 Logger(共用件 src/logger/)

namespace EvoFlow
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

        // v2.5.3 LOG-PRCS-MULTIPOP:子群级过程日志 sink,**本子群自持**(process-only Logger,
        //   进程记录关时为 nullptr → 不写)。每子群各写自己的 .prcs → 天然并行安全(2.5.4 前置)。
        //   _epoch:本地 epoch 计数,run() 每次入口 +1 并落 #epoch 边界,使逐 epoch 终止器重置致的
        //   fes 锯齿在拼接的子群 trace 中可切段(用户决策3)。
        Logger* _own_logger;           // 拥有(dtor 释放)
        int _epoch;

        // per-子群 RNG(2.5.2):本子群持久引擎 + 序号。
        //   运行期 ini()/run() 顶部用 EngineScope 把当前引擎指向 _engine,
        //   使深在 workflow 里的通用 RNG 调用透明落到本子群引擎(并行时各线程互不干扰)。
        //   _index 仅在播种时用于派生确定性种子(derive_subpop_seed),运行期不用。
        std::mt19937 _engine;
        int _index;

        // per-子群 ProblemHandle 副本(2.5.4.2):setProblem 时从主句柄**深拷一份**,本子群自持。
        //   评估热路径(workflow/arrays/archive/comparer)全走这份副本 → 各子群评估状态独立
        //   (配合 2.5.1 的 CEC clone / functor 深拷)→ 并行时互不写冲突。本子群拥有、dtor 释放。
        ProblemHandle* _own_handle;

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

            _own_logger = logger;   // v2.5.3:自持 process-only Logger(builder 造;进程记录关时为 nullptr)
            _epoch = 0;

            _index = 0;   // 引擎默认构造,种子由 seedEngine 于 exe 时派下(见 SubpopulationManager::seedEngines)

            _own_handle = nullptr;   // v2.5.4.2:setProblem 时深拷主句柄
        }

        ~Subpopulation()
        {
            delete _parent;
            delete _offspring;
            delete _best_archive;
            delete _terminator;
            delete[] _parent_buffer;
            delete[] _archives_buffer;
            delete _own_logger;   // v2.5.3:自持子群级 sink
            delete _own_handle;   // v2.5.4.2:自持句柄副本(在 arrays/workflow 之后删——它们只持指针、不删句柄)
        }

        void setProblem(ProblemHandle* problem_hanlde)
        {
            // v2.5.4.2:深拷一份主句柄,本子群自持;评估热路径全走这份副本 → per-子群评估独立。
            //   (可重入:再次 setProblem 先删旧副本。深拷=同问题、确定性,故串行结果与共享一份逐位一致。)
            delete _own_handle;
            _own_handle = new ProblemHandle(*problem_hanlde);

            _parent->setProblem(_own_handle);
            _offspring->setProblem(_own_handle);
            _best_archive->setProblem(_own_handle);
            workflow->setProblem(_own_handle);
            _comparer_pointer = _own_handle->getSolutionComparer();
            // v2.5.3:子群级 sink 的 .prcs 需落到 _log/<opt>/<prob>/ → 同步问题名
            if (_own_logger) _own_logger->setProblem(_own_handle->getName());
        }

        // v2.5.3 LOG-PRCS-MULTIPOP:开本子群本轮的过程文件(优化器 ini 时逐子群扇出调用)。
        //   header extra 声明这是【子群级】文件 + 本群 id/规模,供读取端/人区分两级。
        void newProcessLog(int exe_counter)
        {
            _epoch = 0;
            if (!_own_logger) return;
            std::string extra = "\tLevel:\tsubpop\tSwarm:\t" + _id
                              + "\tSwarmSize:\t" + std::to_string(_parent->getSize());
            _own_logger->newOptimization(exe_counter, extra);
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

        // 播种本子群引擎(2.5.2):管理器在 exe 播全局种子后调用一次,跨代延续、不逐轮重播。
        //   index==0 → 与全局默认引擎逐位一致(单子群保持改造前行为);index>0 错开各子群流。
        void seedEngine(unsigned int base_seed, int index)
        {
            _index = index;
            seed_engine(_engine, derive_subpop_seed(base_seed, index));
        }

        void ini()
        {
            EngineScope es(&_engine);   // 本子群初始化期间所有 RNG 走自己的引擎

            _best_archive->clear();

            // offspring 是每迭代 scratch:workflow 在迭代末清空,但开跑前也须为空。
            //   若不清,首迭代 generator 的 extend 会从初始 size 之上追加 → 混入未初始化个体(评估垃圾解致崩溃)。
            //   此时 setProblem 已把 buffer 内个体配置好,clear 仅压 size→0,后续 extend 复用已配置个体
            //   (兼避 IARR size-0 extend 的"未配置模板"深层崩溃)。[迁移修复:原代码首迭代处理垃圾 offspring 的潜在 bug]
            _offspring->clear();

            // 初始种群:**逐个体**初始化(含评估)→ 更新档案 → 计 FES,且**每个体前查终止**。
            //   v1.4.8 修:原为 `_parent->ini()` 一次性评估**全部**个体、事后再补计 N 次 FES ——
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
            EngineScope es(&_engine);   // 本子群本轮运行期间所有 RNG 走自己的引擎

            // v2.5.3 决策3:每次 run()=一个 epoch。入口落 #epoch 边界 —— epoch 间 manager 会
            //   resetTerminator() 使 fes 归零,拼接的子群 trace 会呈锯齿;此标记让重置可切段。
            //   走 logProcessMeta(不进 -v 回显);读取端跳过 #(非 #rec)元行。
            if (_own_logger != nullptr && _own_logger->process_print())
                _own_logger->logProcessMeta("#epoch\t" + std::to_string(_epoch));
            _epoch++;

            while (!_terminator->termination())
            {
                nextIteration();

                // v3.6 LOG:过程打印(每代最优;若 swarm_print 再打印整群)
                // v2.2.2.2 LOG-PRCS-FORMAT:每代先落一条【记录头】,再落带角色前缀的数据行 ——
                //   解决旧裸行流"每代行数不定(多目标非支配集/整群)⇒ 消费方无法切分代"。
                //   记录头以 FES 为主键(收敛曲线横轴即 FES),代号由记录序天然给出,
                //   故无需维护"随 run() 每 epoch 重入还要正确复位"的代计数器。
                if (_own_logger != nullptr && _own_logger->process_print())
                {
                    int size = 0;
                    Solution* bests = nullptr;
                    getBest(bests, size);
                    bool swarm = _own_logger->swarm_print();
                    int psize = swarm ? _parent->getSize() : 0;

                    // fes 取本子群自持计数:单子群准确;多子群下各群独立计数,故带 swarm 字段
                    //   供消费方区分来源(整体形态见 TODO LOG-PRCS-MULTIPOP,本轮不做)
                    _own_logger->logProcessMeta(
                        "#rec\tfes\t" + std::to_string(_terminator->getFESTimes()) +
                        "\tswarm\t" + _id +
                        "\tnbest\t" + std::to_string(size) +
                        "\tnpop\t" + std::to_string(psize));

                    // v1.1:数据行前缀取【块内序号】,与 .rslt 的 `i+1:\t<toString>` 完全同构
                    //   ⇒ 一个行解析器可同时服务 .rslt/.prcs。块归属由 #rec 计数与分割线界定,
                    //   不再靠 b/p 字母(分割线已足够显眼,字母是冗余)。
                    for (int i = 0; i < size; i++)
                        _own_logger->logprocess(std::to_string(i + 1) + ":\t" +
                            bests[i].decoder_pointer->toString(
                                bests[i].result, bests[i].fitness, _own_logger->full_process_print()));

                    if (swarm)
                    {
                        // v2.2.2.3(.prcs v1.1):最优解与整群之间加分割线 —— 仅靠 b/p 前缀不够显眼。
                        //   `#` 开头,与 `#rec` 同属元数据族,同走 logProcessMeta ⇒ 不进 -v 回显。
                        _own_logger->logProcessMeta("#---------------- population ----------------");
                        for (int i = 0; i < psize; i++)
                        {
                            // 迁移修复:原用 bests[i].result/fitness + 亲代 decoder(混用且 i 可越 bests 界)→ 改用亲代个体自身解
                            Solution& s = (*_parent)[i].solution;
                            _own_logger->logprocess(std::to_string(i + 1) + ":\t" +
                                s.decoder_pointer->toString(
                                    s.result, s.fitness, _own_logger->full_process_print()));
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
