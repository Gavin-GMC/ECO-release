//------------------------Description------------------------
// This file defines individual object with pbest and set-based 
// velocity parameter, which is used in set-based particle swarm optimzer.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include<vector>
#include"individual-pbest.h"

namespace ECFC
{
	class SetParticle : public PbestIndividual
	{
	public:
		class SetVelocity
		{
		private:
			static constexpr double threshold = 1e-3;
			double _low_bound;
			double _accuracy;

			SetVelocity() // 攻复制函数使用
			{
				name = "";
				pre_base = false;
				bidiagraph = false;
				demension = 0;
				choice = 0;
				velocity = nullptr;
				velocityIndex = nullptr;
				velocityIndex_Size = nullptr;
				_low_bound = 0;
				_accuracy = 0;
			}

		public:
			std::string name;
			int demension;
			int choice;
			double* velocity;
			int* velocityIndex;
			int* velocityIndex_Size;

			bool pre_base;
			bool bidiagraph;

			SetVelocity(const ElementNote& note, int variable_size)
			{
				switch (note._type)
				{
				case VariableType::normal:
				case VariableType::allocation:
				{
					name = note._name;
					pre_base = false;
					bidiagraph = false;
					demension = variable_size;
					// _pheromones[i].choice = problem_handle->getChoiceNumber();
					choice = (note._upbound - note._lowbound) / note._accuracy + 1;
					int size = demension * choice;
					velocity = new double[size];
					velocityIndex = new int[size];
					velocityIndex_Size = new int[demension];
					for (int j = 0; j < size; j++)
					{
						velocity[j] = 0;
					}
					_low_bound = note._lowbound;
					_accuracy = note._accuracy;
					break;
				}					
				case VariableType::sequence_direction:
				case VariableType::sub_module:
				{
					name = note._name;
					pre_base = true;
					bidiagraph = false;
					demension = (note._upbound - note._lowbound) / note._accuracy + 1;
					choice = (note._upbound - note._lowbound) / note._accuracy + 1;
					int size = demension * choice;
					velocity = new double[size];
					velocityIndex = new int[size];
					velocityIndex_Size = new int[demension];
					for (int j = 0; j < size; j++)
					{
						velocity[j] = 0;
					}
					_low_bound = note._lowbound;
					_accuracy = note._accuracy;
					break;
				}					
				case VariableType::sequence_bidiagraph:
				{
					name = note._name;
					pre_base = true;
					bidiagraph = true;
					demension = (note._upbound - note._lowbound) / note._accuracy + 1;
					choice = (note._upbound - note._lowbound) / note._accuracy + 1;
					int size = demension * choice;
					velocity = new double[size];
					velocityIndex = new int[size];
					velocityIndex_Size = new int[demension];
					for (int j = 0; j < size; j++)
					{
						velocity[j] = 0;
					}
					_low_bound = note._lowbound;
					_accuracy = note._accuracy;
					break;
				}					
				default:
					name = "";
					pre_base = false;
					bidiagraph = false;
					demension = 0;
					choice = 0;
					velocity = nullptr;
					velocityIndex = nullptr;
					velocityIndex_Size = nullptr;
					_low_bound = 0;
					_accuracy = 0;
					break;
				}
			}

			~SetVelocity()
			{
				delete[] velocity;
				delete[] velocityIndex;
				delete[] velocityIndex_Size;
			}

			int choice2Cid(double choice)
			{
				return int((choice - _low_bound) / _accuracy);
			}

			double cid2Choice(int cid)
			{
				return _low_bound + _accuracy * cid;
			}

			int trans2Did(int demension, double prechoice)
			{
				if (prechoice)
					return choice2Cid(prechoice);
				else
					return demension;
			}

			void damping(double w)
			{
				for(int i=0; i< demension; i++)
					for (int j = 0; j < velocityIndex_Size[i]; j++)
						velocity[i * choice + velocityIndex[i * choice + j]] *= w;
			}

			void addToVelocity(int demensionId, int choiceid, double rate)
			{
				if (demensionId >= demension || rate <= velocity[demensionId * choice + choiceid] || choiceid >= choice)
					return;
				if (rate > 1)
					rate = 1;
				// 不用考虑小于0因为过不了第一项判断

				velocity[demensionId * choice + choiceid] = rate;
				if (bidiagraph)
				{
					velocity[choiceid * choice + demensionId] = rate;
				}
			}

			void velocityIndexUpdate()
			{
				int offset;
				for (int i = 0; i < demension; i++)
				{
					velocityIndex_Size[i] = 0;
					offset = i * choice;
					for (int j = 0; j < choice; j++)
					{
						if (velocity[offset + j] > threshold)
						{
							velocityIndex[offset + velocityIndex_Size[i]] = j;
							velocityIndex_Size[i]++;
						}
					}
				}
			}

			double getVelocityRate(int demensionId, int choiceId)
			{
				return velocity[demensionId * choice + choiceId];
			}

			void clear()
			{
				int length = demension * choice;
				for (int i = 0; i < length; i++)
				{
					velocity[i] = 0;
				}
			}

			SetVelocity* copy()
			{
				SetVelocity* back = new SetVelocity();
				back->_low_bound = _low_bound;
				back->_accuracy = _accuracy;

				back->name = name;
				back->demension = demension;
				back->choice = choice;
				back->velocity = new double[demension * choice];
				back->velocityIndex = new int[demension * choice];
				back->velocityIndex_Size = new int[demension];

				memcpy(back->velocity, velocity, demension * choice * sizeof(double));
				memcpy(back->velocityIndex, velocityIndex, demension * choice * sizeof(int));
				memcpy(back->velocityIndex_Size, velocityIndex_Size, demension * sizeof(int));

				back->pre_base = pre_base;
				back->bidiagraph = bidiagraph;

				return back;
			}			
		};
	
	private:
		int variable_number;
		int* variable_length;

		void clearVelocity()
		{
			for (int i = 0; i < velocity.size(); i++)
			{
				delete velocity[i];
			}
			velocity.clear();
		}
	public:
		std::vector<SetVelocity*> velocity;

		SetParticle()
			:PbestIndividual()
		{
			pbest = Solution();
			variable_number = 0;
			variable_length = nullptr;
		}

		~SetParticle()
		{
			clearVelocity();
			delete[] variable_length;
		}

		static void preAssert(AssertList& list, double* paras)
		{
			PbestIndividual::preAssert(list, paras);
		}

		static void postAssert(AssertList& list, double* paras)
		{
			PbestIndividual::postAssert(list, paras);
			list.add(new Assert(ModuleType::T_individual, "setvelocity", 1, MatchType::postAssert));
		}

		int getVid(int dimension)
		{
			for (int i = 0; i < variable_number; i++)
			{
				if (dimension < variable_length[i])
					return i;
			}
		}

		int getVlength(int vid)
		{
			if (vid == 0)
			{
				return variable_length[0];
			}
			else
				return variable_length[vid] - variable_length[vid - 1];
		}

		int getDid(int dimension, int vid)
		{
			return dimension - variable_length[vid];
		}

		void setProblem(ProblemHandle* problem_handle)
		{
			PbestIndividual::setProblem(problem_handle);

			// 这里可以考虑改进一下，把仍存在的变量的信息进行保留
			SolutionDecoder* decoder = problem_handle->getSolutionDecoder();
			
			variable_number = decoder->getVariableNumber();
			delete[] variable_length;
			variable_length = new int[variable_number];
			
			clearVelocity();
			for (int i = 0; i < variable_number; i++)
			{
				SetVelocity* sv_buffer = new SetVelocity(decoder->notes[i], decoder->variable_sizes[i]);
				velocity.push_back(sv_buffer);
				variable_length[i] = decoder->variable_sizes[i];
			}

			for (int i = 1; i < variable_number; i++)
			{
				variable_length[i] += variable_length[i - 1];
			}
		}

		void setProblem(const Individual& source)
		{
			PbestIndividual::setProblem(source);

			Individual* p_buffer = const_cast<Individual*>(&source);
			SetParticle* pointer = static_cast<SetParticle*> (p_buffer);

			variable_number = pointer->variable_number;
			delete[] variable_length;
			variable_length = new int[variable_number];
			
			clearVelocity();
			for (int i = 0; i < variable_number; i++)
			{
				// 这里有bug！！！不该完全拷贝的，只拷贝形式就好了？
				velocity.push_back(pointer->velocity[i]->copy());
			}
		}

		void ini(bool ini_solution = true, bool evaluate = true, bool ini_speciality = false)
		{
			PbestIndividual::ini(ini_solution, evaluate, ini_speciality);
			// 速度初始化
			if (ini_speciality)
			{
				// 速度向量随机初始化
				int choice_number;
				for (int vid = 0; vid < variable_number; vid++)
				{
					velocity[vid]->clear();
					choice_number = velocity[vid]->choice;
					for (int i = 0; i < velocity[vid]->demension; i++)
					{
						velocity[vid]->addToVelocity(i, rand() % choice_number, 1);
					}
					velocity[vid]->velocityIndexUpdate();
				}
			}
		}

		void copy(const Individual& copy_source)
		{
			PbestIndividual::copy(copy_source);
			Individual* p_buffer = const_cast<Individual*>(&copy_source);
			SetParticle* pointer = static_cast<SetParticle*> (p_buffer);
			
			variable_number = pointer->variable_number;
			delete[] variable_length;
			variable_length = new int[variable_number];
			memcpy(variable_length, pointer->variable_length, sizeof(int) * variable_number);
			
			clearVelocity();
			for (int i = 0; i < variable_number; i++)
			{
				velocity.push_back(pointer->velocity[i]->copy());
			}
		}

		void copy(const double* source_result, const double* source_fitness)
		{
			PbestIndividual::copy(source_result, source_fitness);
		}

		void shallowCopy(const Individual& copy_source)
		{
			PbestIndividual::shallowCopy(copy_source);
			Individual* p_buffer = const_cast<Individual*>(&copy_source);
			SetParticle* pointer = static_cast<SetParticle*> (p_buffer);

			variable_number = pointer->variable_number;
			variable_length = pointer->variable_length;
			velocity = pointer->velocity;
		}

		void swap(Individual& copy_source)
		{
			PbestIndividual::swap(copy_source);
			SetParticle* pointer = static_cast<SetParticle*> (&copy_source);
			
			std::swap(variable_number, pointer->variable_number);
			std::swap(variable_length, pointer->variable_length);			
			std::swap(velocity, pointer->velocity);
		}
	};
}