//------------------------Description------------------------
// 集合式速度驱动学习策略 SetVelocityDrivenStrategy:基于集合算子把粒子群扩展到离散/组合空间——
//   速度=选择对的集合(SetVelocity),位置更新经速度"清晰化"(概率选择 + 排序 + 约束/启发兜底)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <algorithm>
#include <functional>
#include "ecflow-constant.h"
#include "ecflow-basicfunc.h"
#include "individual.h"
#include "feature-setvelocity.h"   // INDIV-COMPOSE S4:setvelocity 特性(取代 SetParticle 成员)
#include "learning-strategy.h"
#include "registry.h"

namespace ECFlow
{
    class SetVelocityDrivenStrategy : public LearningStrategy
    {
    private:
        int _object_number;
        double* _c;

        double _w_ini;
        double _w;
        double _w_attenuation;

        bool v_heuristic;
        bool f_heuristic;

        sortHelper<int, double>* sortbuffer;
        std::string _setvel_key;   // INDIV-COMPOSE:setvelocity 特性身份键(装配期解析,ini 缓存)

        void velocityUpdate(SetVelocityFeature* svf, const Solution& pos, Solution** s)
        {
            double va;

            int variable_size = 0;
            int variable_offset = 0;

            for (int vid = 0; vid < svf->velocity.size(); vid++)
            {
                svf->velocity[vid]->damping(_w);

                variable_offset += variable_size;
                variable_size = svf->getVlength(vid);

                if (svf->velocity[vid]->pre_base)
                {
                    double* variable_pointer;
                    double* example = new double[variable_size * 2];

                    // learn from s
                    for (int i = 0; i < _object_number; i++)
                    {
                        variable_pointer = s[i]->result + variable_offset;
                        for (int i = 1; i < variable_size; i++)
                        {
                            example[2 * i] = variable_pointer[i - 1];
                            example[2 * i + 1] = variable_pointer[i];
                        }
                        example[0] = variable_pointer[variable_size - 1];
                        example[1] = variable_pointer[0];

                        // difference set build
                        variable_pointer = pos.result + variable_offset;
                        for (int i = 0; i < variable_size; i++)
                        {
                            if (example[2 * i] == variable_pointer[variable_size - 1] && example[2 * i + 1] == variable_pointer[0]) // 元素相同
                            {
                                example[2 * i] = EMPTYVALUE;
                                continue;
                            }
                            for (int j = 1; j < variable_size; j++)
                            {
                                if (example[2 * i] == variable_pointer[j - 1] && example[2 * i + 1] == variable_pointer[j]) // 元素相同
                                {
                                    example[2 * i] = EMPTYVALUE;
                                    break;
                                }
                            }
                        }

                        int cid1, cid2;
                        va = _c[i] * rand01_();
                        for (int i = 0; i < variable_size; i++)
                        {
                            if (!is_empty(example[2 * i]))   // 修复:原 != EMPTYVALUE(NaN)恒真
                            {
                                cid1 = svf->velocity[vid]->choice2Cid(example[2 * i]);
                                cid2 = svf->velocity[vid]->choice2Cid(example[2 * i + 1]);
                                svf->velocity[vid]->addToVelocity(cid1, cid2, va);
                            }
                        }
                    }

                    delete[] example;
                }
                else
                {
                    int cid;

                    // learn from s
                    double* solution_pointer = pos.result + variable_offset;
                    double* object_pointer;
                    for (int i = 0; i < _object_number; i++)
                    {
                        object_pointer = s[i]->result + variable_offset;

                        va = _c[i] * rand01_();
                        for (int i = 0; i < variable_size; i++) // task
                        {
                            if (!is_empty(object_pointer[i]) && solution_pointer[i] != object_pointer[i])   // 修复:原 != EMPTYVALUE
                            {
                                cid = svf->velocity[vid]->choice2Cid(object_pointer[i]);
                                if (va > svf->velocity[vid]->getVelocityRate(i, cid))
                                {
                                    svf->velocity[vid]->addToVelocity(i, cid, va);
                                }
                            }
                        }
                    }
                }

                svf->velocity[vid]->velocityIndexUpdate();
            }
        }

        double positionUpdate(SetVelocity* velocity, int demension, int decision_base, double* current_position, ProblemHandle* handle)
        {
            int counter;
            int choiceid;
            double back;

            counter = 0;
            // velocity crisp
            for (int i = 0; i < velocity->velocityIndex_Size[decision_base]; i++)
            {
                int offset = decision_base * velocity->choice;
                choiceid = velocity->velocityIndex[offset + i];
                if (rand01() < velocity->velocity[offset + choiceid])
                {
                    if (v_heuristic)
                        sortbuffer[counter++] = sortHelper<int, double>(choiceid,
                            handle->getChoiceHeuristic(demension, velocity->cid2Choice(choiceid)));
                    else
                        sortbuffer[counter++] = sortHelper<int, double>(choiceid, rand01());
                }
            }
            std::sort(sortbuffer, sortbuffer + counter, std::greater<sortHelper<int, double>>());

            for (int c = 0; c < counter; c++)
            {
                back = velocity->cid2Choice(sortbuffer[c].id);
                if (handle->constrainCheck(demension, back))
                {
                    return back;
                }
            }

            // position crisp
            double current_decision = EMPTYVALUE;
            if (velocity->pre_base)
            {
                // 获得当前决策
                double pre_decision = velocity->cid2Choice(decision_base);
                int vid = handle->getBelongVariableId(demension);
                int variable_offset = handle->getVariableOffset(vid);
                int variable_size = handle->getVariableLength(vid);

                double* current_results = current_position + variable_offset;
                for (int i = 0; i < variable_size; i++)
                {
                    if (equal(current_results[i], pre_decision))
                    {
                        if (i == variable_size - 1)
                            current_decision = current_results[0];
                        else
                            current_decision = current_results[i + 1];

                        break;
                    }
                }
            }
            else
            {
                current_decision = current_position[demension];
            }

            if (!is_empty(current_decision) && handle->constrainCheck(demension, current_decision))   // 修复:原 != EMPTYVALUE
            {
                return current_decision;
            }

            // full crisp
            int f_size = handle->getChoiceNumber(demension);
            if (f_size == 0)
                return EMPTYVALUE;

            if (f_heuristic)
                back = handle->getPrioriChoice(demension);
            else
                back = handle->getRandomChoiceInspace(demension);

            return back;
        }

    public:
        // setvelocity = 每实例私有的集合速度(每变量一个 SetVelocity 矩阵);pbest **不**在此声明——由拓扑(如 PGBest)负责,afterEvaluate 维护
        std::vector<FeatureDemand> featureDemands() const override
        {
            return { { "setvelocity", "setvelocity", FeatureScope::Private, {} } };
        }
        SetVelocityDrivenStrategy(int object_number = 2, double c = 2, double w_ini = 0.9, bool v_heuristic = true, bool f_heuristic = true, double w_attenuation = EMPTYVALUE)
            : LearningStrategy()
        {
            _object_number = object_number;
            _c = new double[object_number];
            for (int i = 0; i < object_number; i++)
                _c[i] = c;

            _w = 0;
            _w_ini = w_ini;
            _w_attenuation = w_attenuation;
            this->v_heuristic = v_heuristic;
            this->f_heuristic = f_heuristic;
            sortbuffer = nullptr;
        }

        SetVelocityDrivenStrategy(int object_number, double* c, double w_ini = 0.9, bool v_heuristic = true, bool f_heuristic = true, double w_attenuation = EMPTYVALUE)
            : LearningStrategy()
        {
            _object_number = object_number;
            _c = new double[object_number];
            memcpy(_c, c, sizeof(double) * object_number);

            _w = 0;
            _w_ini = w_ini;
            _w_attenuation = w_attenuation;
            this->v_heuristic = v_heuristic;
            this->f_heuristic = f_heuristic;
            sortbuffer = nullptr;
        }

        ~SetVelocityDrivenStrategy()
        {
            delete[] _c;
            delete[] sortbuffer;
        }

        static void preAssert(AssertList& list, double* paras)
        {
            // setvelocity 需求已移入 featureDemands()(INDIV-COMPOSE),不再经个体断言
            list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::notLess));
        }
        static void postAssert(AssertList& list, double* paras) { list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert)); }

        void ini(ProblemHandle* problem_handle) override { _w = _w_ini; _setvel_key = featureKey("setvelocity"); }   // 缓存 setvelocity 键

        void setProblem(ProblemHandle* problem_handle) override
        {
            delete[] sortbuffer;
            sortbuffer = new sortHelper<int, double>[problem_handle->getProblemSize()];
        }

        void preparation_s(IndividualArray& population, Terminator*) override
        {
            if (is_empty(_w_attenuation))    // 未设 → 随机惯性(修复:原 == EMPTYVALUE 恒假)
                _w = rand01_();
            else
                _w -= _w_attenuation;
        }

        void preparation_i(Individual* individual, Solution** learning_object, Individual* child) override
        {
            // 子代 setvelocity 已由通用 inheritFeaturesFrom 继承亲代矩阵(inheritAtBirth);此处 damp+叠加学习,当前位置读亲代
            SetVelocityFeature* svf = child->feature<SetVelocityFeature>(_setvel_key);
            velocityUpdate(svf, individual->solution, learning_object);
        }

        // update_i(pbestUpdate)已删除:pbest 归拓扑声明(PGBest),评估后由通用 PbestFeature::afterEvaluate 维护(INDIV-COMPOSE)

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual* child) override
        {
            SetVelocityFeature* svf = child->feature<SetVelocityFeature>(_setvel_key);

            int vid = problem_handle->getBelongVariableId(decision_d);
            int decision_base;
            SetVelocity* velocity = svf->velocity[vid];
            if (svf->velocity[vid]->pre_base)
            {
                // 变量内维度
                int did = problem_handle->getWithinVariableId(decision_d);
                if (did == 0 || is_empty(child->solution[decision_d - 1]))   // 修复:原 == EMPTYVALUE
                    decision_base = ECFlow::get_int(0, velocity->choice - 1);  // 修复:原 rand()%choice
                else
                    decision_base = velocity->choice2Cid(child->solution[decision_d - 1]);
            }
            else
            {
                decision_base = problem_handle->getWithinVariableId(decision_d);
            }

            return positionUpdate(velocity, decision_d, decision_base, individual->solution.result, problem_handle);
        }
    };

    inline Registry<LearningStrategy>::Entry setVelocityDrivenStrategyEntry()
    {
        return { "SetVelocityDriven", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"object_number", ParamKind::Int,  1, 0x3f3f3f3f, false, 2, 2},
                                 {"c",             ParamKind::Real, 0.0, 10.0,     false, 1.5, 2.5},
                                 {"w_ini",         ParamKind::Real, 0.0, 1.0,      false, 0.4, 0.9},
                                 {"v_heuristic",   ParamKind::Enum, 0, 1,          false, 0, 1},
                                 {"f_heuristic",   ParamKind::Enum, 0, 1,          false, 0, 1},
                                 {"w_attenuation", ParamKind::Real, 0.0, 1.0,      true,  0.0, 0.1} } }, sizeof(SetVelocityDrivenStrategy),
            [](const double* p) -> LearningStrategy* {
                return p ? new SetVelocityDrivenStrategy(int(p[0]), p[1], p[2], p[3] != 0, p[4] != 0, p[5])
                         : new SetVelocityDrivenStrategy();
            },
            [](AssertList& L, const double* p) { SetVelocityDrivenStrategy::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { SetVelocityDrivenStrategy::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(lstrat_setvelocitydriven, LearningStrategy, setVelocityDrivenStrategyEntry());
}
