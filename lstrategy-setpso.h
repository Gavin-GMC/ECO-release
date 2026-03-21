//------------------------Description------------------------
// This file defines the learning strategy of set-based pso, 
// It extends pso to discrete spaces based on set-based operators
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include"ecfc-constant.h"
#include"learning-strategy.h"
#include"sort-helper.h"
#include"individual-setparticle.h"

namespace ECFC
{
	// 基于集合的粒子群优化算法
	class SetPSOStrategy : public LearningStrategy
	{
	private:
		int _object_number;
		double* _c;
		double* _r;
		
		double _w_ini;
		double _w;
		double _w_attenuation;

		bool v_heuristic;
		bool f_heuristic;

		sortHelper<int, double>* sortbuffer;

		void velocityUpdate(SetParticle* particle, Solution** s)
		{
			double va;

			int variable_size = 0;
			int variable_offset = 0;
			
			for (int vid = 0; vid < particle->velocity.size(); vid++)
			{
				particle->velocity[vid]->damping(_w);

				variable_offset += variable_size;
				variable_size = particle->getVlength(vid);

				if (particle->velocity[vid]->pre_base)
				{
					double* variable_pointer;
					double* example = new double[variable_size * 2];

					//learn from s
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

						//difference set build
						variable_pointer = particle->solution.result + variable_offset;
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
						va = _c[i] * _r[i];
						for (int i = 0; i < variable_size; i++)
						{
							if (example[2 * i] != EMPTYVALUE)
							{
								cid1 = particle->velocity[vid]->choice2Cid(example[2 * i]);
								cid2 = particle->velocity[vid]->choice2Cid(example[2 * i + 1]);
								particle->velocity[vid]->addToVelocity(cid1, cid2, va);
							}
						}
					}

					delete[] example;
				}
				else {
					int cid;
					
					//learn from s
					double* solution_pointer = particle->solution.result + variable_offset;
					double* object_pointer;
					for (int i = 0; i < _object_number; i++)
					{
						object_pointer = s[i]->result + variable_offset;
						
						va = _c[i] * _r[i];
						for (int i = 0; i < variable_size; i++)//task
						{
							if (object_pointer[i] != EMPTYVALUE && solution_pointer[i] != object_pointer[i])
							{
								cid = particle->velocity[vid]->choice2Cid(object_pointer[i]);
								if (va > particle->velocity[vid]->getVelocityRate(i, cid))
								{
									particle->velocity[vid]->addToVelocity(i, cid, va);
								}
							}
						}
					}
				}

				particle->velocity[vid]->velocityIndexUpdate();
			}
		}

		double positionUpdate(SetParticle::SetVelocity* velocity,int demension, int decision_base, double* current_position, ProblemHandle* handle)
		{
			int counter;
			int choiceid;
			double back;

			counter = 0;
			//velocity crisp
			for (int i = 0; i <velocity->velocityIndex_Size[decision_base]; i++)
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
						{
							current_decision = current_results[0];
						}
						else
						{
							current_decision = current_results[i + 1];
						}

						break;
					}
				}
			}
			else {
				current_decision = current_position[demension];
			}

			if (current_decision != EMPTYVALUE && handle->constrainCheck(demension, current_decision))
			{
				return current_decision;
			}

			//full crisp
			int f_size = handle->getChoiceNumber(demension);
			if (f_size == 0)
				return EMPTYVALUE;

			if (f_heuristic)
			{
				back = handle->getPrioriChoice(demension);
			}
			else
			{
				back = handle->getRandomChoiceInspace(demension);
			}

			return back;
		}

	public:
		SetPSOStrategy(int object_number = 2, double c = 2, double w_ini = 0.9, bool v_heuristic = true, bool f_heuristic = true, double w_attenuation = EMPTYVALUE)
			:LearningStrategy()
		{
			_object_number = object_number;
			_c = new double[object_number];
			_r = new double[object_number];
			for (int i = 0; i < object_number; i++)
				_c[i] = c;

			_w = 0;
			_w_ini = w_ini;
			_w_attenuation = w_attenuation;

			this->v_heuristic = v_heuristic;
			this->f_heuristic = f_heuristic;
			sortbuffer = nullptr;
		}

		SetPSOStrategy(int object_number, double* c, double w_ini = 0.9, bool v_heuristic = true, bool f_heuristic = true, double w_attenuation = EMPTYVALUE)
			:LearningStrategy()
		{
			_object_number = object_number;
			_c = new double[object_number];
			_r = new double[object_number];
			memcpy(_c, c, sizeof(double) * object_number);

			_w = 0;
			_w_ini = w_ini;
			_w_attenuation = w_attenuation;

			this->v_heuristic = v_heuristic;
			this->f_heuristic = f_heuristic;
			sortbuffer = nullptr;
		}

		~SetPSOStrategy()
		{
			delete[] _c;
			delete[] _r;
			delete[] sortbuffer;
		}

		static void preAssert(AssertList& list, double* paras)
		{
			list.add(new Assert(ModuleType::T_individual, "setvelocity", 1, MatchType::equal));
			list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::notLess));
		}

		static void postAssert(AssertList& list, double* paras)
		{

		}

		void ini(ProblemHandle* problem_handle)
		{
			_w = _w_ini;
		}

		void setProblem(ProblemHandle* problem_handle) 
		{
			delete[] sortbuffer;
			sortbuffer = new sortHelper<int, double>[problem_handle->getProblemSize()];
		}

		void preparation_s(Population& population)
		{
			if (_w_attenuation == EMPTYVALUE)
			{
				_w = rand01_();
			}
			else
			{
				_w -= _w_attenuation;
			}

			for (int i = 0; i < _object_number; i++)
				_r[i] = rand01_();
		}

		void preparation_i(Individual* individual, Solution** learning_object,Individual* child)
		{
			SetParticle* pointer = static_cast<SetParticle*> (child);

			// 速度更新
			child->copy(*individual);
			velocityUpdate(pointer, learning_object);
		}

		void update_i(Individual* child) 
		{
			SetParticle* pointer = static_cast<SetParticle*> (child);

			pointer->pbestUpdate();
		}

		double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual* child)
		{
			SetParticle* pointer = static_cast<SetParticle*>(child);

			// 位置更新
			int vid = problem_handle->getBelongVariableId(decision_d);
			int decision_base;
			SetParticle::SetVelocity* velocity = pointer->velocity[vid];
			if (pointer->velocity[vid]->pre_base)
			{
				// 变量内维度
				int did = problem_handle->getWithinVariableId(decision_d);
				if (did == 0 || child->solution[decision_d - 1] == EMPTYVALUE)
					decision_base = rand() % velocity->choice;
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
}