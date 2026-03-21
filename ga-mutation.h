//------------------------Description------------------------
// This file defines the mutation operator of evolationary algorithms.
// It exploits the neighborhood by changing the fragment of the current
// individual in a specific way.
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
#include"solution.h"
#include"problem-handle.h"
#include <random>

namespace ECFC
{
	class Mutation
	{
	protected:
		double _mutation_rate;

	public:
		Mutation(double mutation_rate)
		{
			_mutation_rate = mutation_rate;
		}

		virtual ~Mutation() {}

		static void preAssert(AssertList& list, double* paras)
		{
			// none
		}

		static void postAssert(AssertList& list, double* paras)
		{
			// none
		}

		virtual void setProblem(ProblemHandle* problem_handle)
		{

		}

		virtual double apply(Solution* solution, int demensionId, ProblemHandle* handle)
		{
			// 报错

			// 退出
			exit(-1);
		}

		virtual void apply(Solution* solution, ProblemHandle* handle)
		{
			for (int i = 0; i < solution->getSolutionSize(); i++)
			{
				if (rand01() < _mutation_rate)
				{
					solution->result[i] = apply(solution, i, handle);
#if defined(DEBUGMODE) || defined(MUTATIONDEBUG)
					sys_logger.debug("AFTER mutation:\t" + std::to_string(solution->result[i]));
#endif
				}
			}
		}
	};

	// 位变异
	class BitMutation : public Mutation
	{
	public:
		BitMutation(double mutation_rate = 0.01)
			:Mutation(mutation_rate)
		{

		}

		~BitMutation()
		{

		}

		static void preAssert(AssertList& list, double* paras)
		{
			Mutation::preAssert(list, paras);
		}

		static void postAssert(AssertList& list, double* paras)
		{
			Mutation::postAssert(list, paras);
		}

		double apply(Solution* solution, int demensionId, ProblemHandle* handle)
		{
			return handle->getRandomChoiceInspace(demensionId);
		}
	};

	// 位翻转变异
	class BitFlipMutation : public Mutation
	{
	public:
		BitFlipMutation(double mutation_rate = 0.01)
			:Mutation(mutation_rate)
		{

		}

		~BitFlipMutation()
		{

		}

		static void preAssert(AssertList& list, double* paras)
		{
			Mutation::preAssert(list, paras);
		}

		static void postAssert(AssertList& list, double* paras)
		{
			Mutation::postAssert(list, paras);
		}

		double apply(Solution* solution, int demensionId, ProblemHandle* handle)
		{
			if (solution->result[demensionId])
				return 0;
			return 1;
		}
	};

	// 反转变异
	class OverturnMutation : public Mutation
	{
	private:
		int _times;

	public:
		OverturnMutation(int times = 1, double mutation_rate = 0.01)
			:Mutation(mutation_rate)
		{
			_times = times;
		}

		~OverturnMutation() {}

		static void preAssert(AssertList& list, double* paras)
		{
			Mutation::preAssert(list, paras);
		}

		static void postAssert(AssertList& list, double* paras)
		{
			Mutation::postAssert(list, paras);
		}

		void apply(Solution* solution, ProblemHandle* handle)
		{
			if (_mutation_rate < rand01()) // 不变异
				return;

			int oid1;
			int oid2;
			int s_size = solution->getSolutionSize();

			// 翻转片段指定次数
			for (int i = 0; i < _times; i++)
			{
				oid1 = rand() % s_size;
				oid2 = rand() % s_size;
				if (oid1 > oid2)
					std::swap(oid1, oid2);
				while (oid1 < oid2)
					std::swap(solution->result[oid1++], solution->result[oid2--]);
			}
		}
	};

	// 交换变异
	class ExchangeMutation : public Mutation
	{
	private:
		int _times;

	public:
		ExchangeMutation(int times = 1, double mutation_rate = 1)
			:Mutation(mutation_rate)
		{
			_times = times;
		}

		~ExchangeMutation() {}

		static void preAssert(AssertList& list, double* paras)
		{
			Mutation::preAssert(list, paras);
		}

		static void postAssert(AssertList& list, double* paras)
		{
			Mutation::postAssert(list, paras);
		}

		void apply(Solution* solution, ProblemHandle* handle)
		{
			if (_mutation_rate < rand01()) // 不变异
				return;

			int eid1;
			int eid2;
			int s_size = solution->getSolutionSize();
			//exparameter is the times of exchange
			for (int i = 0; i < _times; i++)
			{
				eid1 = rand() % s_size;
				eid2 = rand() % s_size;
				std::swap(solution->result[eid1], solution->result[eid2]);
			}
		}
	};

	// 多项式变异
	class PM_Mutation : public Mutation
	{
	private:
		double _eta;
		double _r;
		double _sigma;

	public:
		PM_Mutation(double eta, double mutation_rate = 0.01)
			:Mutation(mutation_rate)
		{
			_eta = eta;
			_r = 0;
			_sigma = 0;
		}

		~PM_Mutation() {}

		static void preAssert(AssertList& list, double* paras)
		{
			Mutation::preAssert(list, paras);
		}

		static void postAssert(AssertList& list, double* paras)
		{
			Mutation::postAssert(list, paras);
		}

		double apply(Solution* solution, int demensionId, ProblemHandle* handle)
		{
			double upb = handle->getVariableUpbound(demensionId);
			double lowb = handle->getVariableLowbound(demensionId);

            if (solution->result[demensionId] > upb)
                solution->result[demensionId] = upb;
            else if (solution->result[demensionId] < lowb)
                solution->result[demensionId] = lowb;

			_r = rand01_();
			if (_r > 0.5)
				_sigma = 1 - pow(2 * (1 - _r) + 2 * (_r - 0.5) * pow(1 - (upb - solution->result[demensionId]) / (upb - lowb), _eta + 1), 1 / (_eta + 1));
			else
				_sigma = pow(2 * _r + (1 - 2 * _r) * pow(1 - (solution->result[demensionId] - lowb) / (upb - lowb), _eta + 1), 1 / (_eta + 1)) - 1;

			return solution->result[demensionId] + _sigma * (upb - lowb);
		}
	};

	// 高斯变异
	class GaussMutation : public Mutation
	{
	private:
		double _sigma;

		double guassion()
		{
			return _sigma * (
				sqrt((-2) * log(ECFC::rand01()))
				* sin(2 * 3.1415926 * ECFC::rand01()));
		}
	public:
		GaussMutation(double sigma, double mutation_rate = 0.01)
			:Mutation(mutation_rate)
		{
			_sigma = sigma;
		}

		~GaussMutation() {}

		static void preAssert(AssertList& list, double* paras)
		{
			Mutation::preAssert(list, paras);
		}

		static void postAssert(AssertList& list, double* paras)
		{
			Mutation::postAssert(list, paras);
		}

		double apply(Solution* solution, int demensionId, ProblemHandle* handle)
		{
			return solution->result[demensionId] + guassion();
		}
	};

	// 不变异
	class NoMutation : public Mutation
	{
	public:
		NoMutation()
			:Mutation(0)
		{

		}

		~NoMutation() {}

		static void preAssert(AssertList& list, double* paras)
		{
			Mutation::preAssert(list, paras);
		}

		static void postAssert(AssertList& list, double* paras)
		{
			Mutation::postAssert(list, paras);
		}

		double apply(Solution* solution, int demensionId, ProblemHandle* handle)
		{
			return solution->result[demensionId];
		}

		void apply(Solution* solution, ProblemHandle* handle)
		{
			return;
		}
	};

	// 插入变异
	class InsertMutation : public Mutation
	{
	private:
		int _times;

	public:
		InsertMutation(int times = 1, double mutation_rate = 0.01)
			:Mutation(mutation_rate)
		{
			_times = times;
		}

		~InsertMutation() {}

		static void preAssert(AssertList& list, double* paras)
		{
			Mutation::preAssert(list, paras);
		}

		static void postAssert(AssertList& list, double* paras)
		{
			Mutation::postAssert(list, paras);
		}

		void apply(Solution* solution, ProblemHandle* handle)
		{
			if (_mutation_rate < rand01()) // 不变异
				return;

			int oid1; // 选择位
			int oid2; // 插入位
			int s_size = solution->getSolutionSize();

			// 执行重插入指定次数
			for (int i = 0; i < _times; i++)
			{
				oid1 = rand() % s_size;
				oid2 = rand() % s_size;

				double buffer = (*solution)[oid1];

				if (oid1 < oid2)
				{
					memmove(solution->result + oid1, solution->result + oid1 + 1, sizeof(double) * (oid2 - oid1));
					(*solution)[oid2] = buffer;
				}
				else
				{
					memmove(solution->result + oid2 + 1, solution->result + oid2, sizeof(double) * (oid1 - oid2));
					(*solution)[oid2] = buffer;
				}
			}
		}
	};

	// 扰乱变异
	class ReorderMutation : public Mutation
	{
	private:
		int _times;

	public:
		ReorderMutation(int times = 1, double mutation_rate = 0.01)
			:Mutation(mutation_rate)
		{
			_times = times;
		}

		~ReorderMutation() {}

		static void preAssert(AssertList& list, double* paras)
		{
			Mutation::preAssert(list, paras);
		}

		static void postAssert(AssertList& list, double* paras)
		{
			Mutation::postAssert(list, paras);
		}

		void apply(Solution* solution, ProblemHandle* handle)
		{
			if (_mutation_rate < rand01()) // 不变异
				return;

			int oid1;
			int oid2;
			int s_size = solution->getSolutionSize();

			// 打乱片段指定次数
			for (int i = 0; i < _times; i++)
			{
				oid1 = rand() % s_size;
				oid2 = rand() % s_size;

				if (oid2 < oid1)
					std::swap(oid1, oid2);

				oid2++; // 打乱时索引小于该值

				std::shuffle(solution->result + oid1, solution->result + oid2, std::random_device());
			}
		}
	};
}