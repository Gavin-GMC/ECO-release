//------------------------Description------------------------
// This file defines the terminating condition of the optimizer
// or population iteration. When at least one of the conditions
// is met, the optimization process will stop and output the result.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include<ctime>
#include<vector>

namespace ECFC 
{
	class Terminator
	{
	private:
		int _conditions[3]; // FES, Convergence, Time
		int _states[3]; // FES, Convergence, Time
		time_t _start_time;

	public:
		Terminator()
		{
			for (int i = 0; i < 3; i++)
			{
				_conditions[i] = ECFC_MAX;
			}
		}

		void setMaxFES(int fes)
		{
			_conditions[0] = fes;
		}

		void setMaxConvergence(int fes)
		{
			_conditions[1] = fes;
		}

		void setMaxTime(int sec)
		{
			_conditions[2] = sec;
		}

		bool termination()
		{
			for (int i = 0; i < 3; i++)
			{
				if (_states[i] >= _conditions[i])
					return true;
			}
			return false;
		}

		void reset()
		{
			_states[0] = 0;
			_states[1] = 0;
			_states[2] = 0;
			_start_time = time(NULL);
		}

		void update(bool best_update)
		{
			_states[0]++;

			if (best_update)
				_states[1] = 0;
			else
				_states[1]++;

			_states[2] = time(NULL) - _start_time;
		}

		int getFESTimes()
		{
			return _states[0];
		}

		void merge(Terminator* subterminator)
		{
			for (int i = 0; i < 3; i++)
			{
				_states[i] += subterminator->_states[i];
			}
		}
	};

	/*
	* // 如果不考虑更复杂的终止条件的话，是否可以直接取消三种条件，全部整合进终止器中
	class TerminateCondition
	{
	private:

	public:
		virtual bool termination() = 0;

		virtual void reset() = 0;

		virtual void update(bool best_update) = 0;

		virtual TerminateCondition* copy() = 0;

		virtual void merge(TerminateCondition* object) = 0;
	};

	class FESTerminator final : public TerminateCondition
	{
	private:
		int _max_fes;
		int _fes;
	public:
		FESTerminator(int max_fes)
		{
			_max_fes = max_fes;
			_fes = 0;
		}

		bool termination()
		{
			return _fes >= _max_fes;
		}

		void reset()
		{
			_fes = 0;
		}

		void update(bool best_update)
		{
			_fes++;
		}

		TerminateCondition* copy()
		{
			return new FESTerminator(_max_fes);
		}

		void merge(TerminateCondition* object)
		{
			FESTerminator* buffer = static_cast<FESTerminator*>(object);
			_fes += buffer->_fes;
		}
	};

	class ConvergeTerminator final : public TerminateCondition
	{
	private:
		int _max_no_best_update;
		int _no_best_update;

	public:
		ConvergeTerminator(int max_no_best_update)
		{
			_max_no_best_update = max_no_best_update;
			_no_best_update = 0;
		}

		bool termination()
		{
			return _no_best_update >= _max_no_best_update;
		}

		void reset()
		{
			_no_best_update = 0;
		}

		void update(bool best_update)
		{
			if (best_update)
				_no_best_update = 0;
			else
				_no_best_update++;
		}

		TerminateCondition* copy()
		{
			return new ConvergeTerminator(_max_no_best_update);
		}

		void merge(TerminateCondition* object)
		{
			
		}
	};

	class TimeTerminator  final : public TerminateCondition
	{
	private:
		time_t _max_time;
		time_t _beginning;
	public:
		TimeTerminator(int max_time)
		{
			_max_time = max_time;
			_beginning = time(NULL);
		}

		bool termination()
		{
			return time(NULL) - _beginning >= _max_time;
		}

		void reset()
		{
			_beginning = time(NULL);
		}

		void update(bool best_update)
		{

		}

		TerminateCondition* copy()
		{
			return new TimeTerminator(_max_time);
		}

		void merge(TerminateCondition* object)
		{

		}
	};

	class Terminator
	{
	private:
		std::vector<TerminateCondition*> _conditions;
		int _fes_times;
	public:
		Terminator()
		{
			_fes_times = 0;
		}

		void addCondition(TerminateCondition* condition)
		{
			_conditions.push_back(condition);
		}

		bool termination()
		{
			for (int i = 0; i < _conditions.size(); i++)
			{
				if (_conditions[i]->termination())
					return true;
			}
			return false;
		}

		void reset()
		{
			_fes_times = 0;
			for (int i = 0; i < _conditions.size(); i++)
			{
				_conditions[i]->reset();
			}
		}

		void update(bool best_update)
		{
			_fes_times++;
			for (int i = 0; i < _conditions.size(); i++)
			{
				_conditions[i]->update(best_update);
			}
		}

		int getFESTimes()
		{
			return _fes_times;
		}

		Terminator* split()
		{
			Terminator* back = new Terminator();
			for (int i = 0; i < _conditions.size(); i++)
			{
				back->addCondition(_conditions[i]->copy());
			}

			return back;
		}

		void merge(Terminator* subterminator)
		{
			for (int i = 0; i < _conditions.size(); i++)
			{
				_conditions[i]->merge(subterminator->_conditions[i]);
			}
		}
	};
*/

	
}
