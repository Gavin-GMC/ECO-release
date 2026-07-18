//------------------------Description------------------------
// Population(顶层聚合,旧名 Swarm):持子群管理器(SubpopulationManager)+ 全局最优档案。薄壳,方法委派管理器。
//   nextIteration → manager.runEpoch():跑一"轮"(epoch)——结构更新 → 群间交互 → 逐子群(配额 → 跑 → 并账)。
//   → 与 SingleSwarm::update 内的并账**重复计入**(全局账面 2 倍速膨胀 → 用户实得约半数预算)。
//   现整轮时序收归 manager.runEpoch(基类固定时序),updateTerminator 随之删除。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "subpopulation.h"
#include "best-archive.h"
#include "subpopulation-manager.h"

namespace ECFlow
{
    class Population
    {
    private:
        SubpopulationManager* _subpopulations;
        BestArchive*          _bestholder;   // 全局最优档案

    public:
        Population(SubpopulationManager* manager, BestArchive* archive)
        {
            _subpopulations = manager;
            _bestholder = archive;
        }

        ~Population()
        {
            delete _subpopulations;
            delete _bestholder;
        }

        void setProblem(ProblemHandle* problem_handle)
        {
            _subpopulations->setProblem(problem_handle);
            _bestholder->setProblem(problem_handle);
        }

        void setTerminator(Terminator* terminator)
        {
            _subpopulations->setTerminator(terminator);
        }

        void ini()
        {
            _subpopulations->ini();
            _bestholder->clear();
        }

        void nextIteration()
        {
            _subpopulations->runEpoch();
        }

        void globalOptimumCollection()
        {
            _subpopulations->globalOptimumCollection(_bestholder);
        }

        bool getBest(Solution*& best_pointer, int& best_size)
        {
            best_size = _bestholder->getBestSize();
            best_pointer = _bestholder->getBest();
            return true;
        }

        Subpopulation* getSubswarm(std::string id)
        {
            return _subpopulations->getSubswarm(id);
        }
    };
}
