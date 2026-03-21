//------------------------Description------------------------
// This file defines some optional swarm topology components,
// which are basic and commonly used.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include"subswarm-topology.h"

namespace ECFC
{
	class ConnectedModel : public TopologyModel
	{
	public:
		ConnectedModel()
			:TopologyModel()
		{

		}

		~ConnectedModel()
		{

		}

		static void preAssert(AssertList& list, double* paras)
		{
			// none
		}

		static void postAssert(AssertList& list, double* paras)
		{
			// none
		}

		void build(Subswarm** subswarms, int& swarm_number)
		{
			int counter;
			for (int i = 0; i < swarm_number; i++)
			{
				counter = 0;
				for (int j = 0; j < swarm_number; j++)
				{
					if (i != j)
					{
						neighborhoods[i][counter++] = subswarms[j];
					}
				}
			}
		}

		void ini(Subswarm** subswarms, int& swarm_number)
		{
			delete[] neighborhoods;
			neighborhoods = new NeighborSwarm[swarm_number];
			for (int i = 0; i < swarm_number; i++)
			{
				neighborhoods[i].setSize(swarm_number - 1);
			}

			build(subswarms, swarm_number);
		}
	};

	class StarModel : public TopologyModel
	{
	public:
		StarModel()
			:TopologyModel()
		{

		}

		~StarModel()
		{

		}

		static void preAssert(AssertList& list, double* paras)
		{
			// none
		}

		static void postAssert(AssertList& list, double* paras)
		{
			// none
		}

		void build(Subswarm** subswarms, int& swarm_number)
		{
			for (int i = 1; i < swarm_number; i++)
			{
				neighborhoods[0][i] = subswarms[i];
				neighborhoods[i][0] = subswarms[0];
			}
		}

		void ini(Subswarm** subswarms, int& swarm_number)
		{
			delete[] neighborhoods;
			neighborhoods = new NeighborSwarm[swarm_number];

			neighborhoods[0].setSize(swarm_number - 1);
			for (int i = 1; i < swarm_number; i++)
			{
				neighborhoods[i].setSize(1);
			}

			build(subswarms, swarm_number);
		}
	};

	class ToroidalModel final : public TopologyModel
	{
	public:
		ToroidalModel()
			:TopologyModel()
		{

		}

		~ToroidalModel()
		{

		}

		static void preAssert(AssertList& list, double* paras)
		{
			// none
		}

		static void postAssert(AssertList& list, double* paras)
		{
			// none
		}

		void build(Subswarm** subswarms, int& swarm_number)
		{
			neighborhoods[0][0] = subswarms[swarm_number - 1];
			for (int i = 1; i < swarm_number; i++)
			{
				neighborhoods[i][0] = subswarms[i - 1];
			}

			for (int i = 1; i < swarm_number; i++)
			{
				neighborhoods[i - 1][1] = subswarms[i];
			}
			neighborhoods[swarm_number - 1][1] = subswarms[0];

		}

		void ini(Subswarm** subswarms, int& swarm_number)
		{
			delete[] neighborhoods;
			neighborhoods = new NeighborSwarm[swarm_number];
			for (int i = 0; i < swarm_number; i++)
			{
				neighborhoods[i].setSize(2);
			}

			build(subswarms, swarm_number);
		}
	};

	class PreswarmModel : public TopologyModel
	{
	public:
		PreswarmModel()
			:TopologyModel()
		{

		}

		~PreswarmModel()
		{

		}

		static void preAssert(AssertList& list, double* paras)
		{
			// none
		}

		static void postAssert(AssertList& list, double* paras)
		{
			// none
		}

		void build(Subswarm** subswarms, int& swarm_number)
		{
			for (int i = 0; i < swarm_number; i++)
			{
				for (int j = 0; j < i; j++)
				{
					neighborhoods[i][j] = subswarms[j];
				}
			}
		}

		void ini(Subswarm** subswarms, int& swarm_number)
		{
			delete[] neighborhoods;
			neighborhoods = new NeighborSwarm[swarm_number];
			for (int i = 0; i < swarm_number; i++)
			{
				neighborhoods[i].setSize(i);
			}

			build(subswarms, swarm_number);
		}
	};

	class NoConnectModel : public TopologyModel
	{
	public:
		NoConnectModel()
			:TopologyModel()
		{

		}

		~NoConnectModel()
		{

		}

		static void preAssert(AssertList& list, double* paras)
		{
			// none
		}

		static void postAssert(AssertList& list, double* paras)
		{
			// none
		}

		void build(Subswarm** subswarms, int& swarm_number)
		{

		}

		void ini(Subswarm** subswarms, int& swarm_number)
		{
			delete[] neighborhoods;
			neighborhoods = new NeighborSwarm[swarm_number];
			for (int i = 0; i < swarm_number; i++)
			{
				neighborhoods[i].setSize(0);
			}
		}
	};

	class CellModel final : public TopologyModel
	{
	private:
		int _degree;
	public:
		CellModel(int degree)
			:TopologyModel()
		{
			_degree = degree;
		}

		~CellModel()
		{

		}

		static void preAssert(AssertList& list)
		{

		}

		static void postAssert(AssertList& list)
		{

		}

		void build(Subswarm** subswarms, int& swarm_number)
		{
			// 如何将细胞模型拓展到任意度
		}

		void ini(Subswarm** subswarms, int& swarm_number)
		{
			delete[] neighborhoods;
			neighborhoods = new NeighborSwarm[swarm_number];
			for (int i = 0; i < swarm_number; i++)
			{
				neighborhoods[i].setSize(_degree);
			}

			build(subswarms, swarm_number);
		}
	};

	class RandomModel final : public TopologyModel
	{
	private:
		int _degree;
	public:
		RandomModel(int degree)
			:TopologyModel()
		{
			_degree = degree;
		}

		~RandomModel()
		{

		}

		static void preAssert(AssertList& list)
		{

		}

		static void postAssert(AssertList& list)
		{

		}


		void build(Subswarm** subswarms, int& swarm_number)
		{
			//生成一个随机的0-1对称矩阵，要求主对角线为0，且每行1的个数相同

		}

		void ini(Subswarm** subswarms, int& swarm_number)
		{
			delete[] neighborhoods;
			neighborhoods = new NeighborSwarm[swarm_number];
			for (int i = 0; i < swarm_number; i++)
			{
				neighborhoods[i].setSize(_degree);
			}

			build(subswarms, swarm_number);
		}
	};

	class GivenModel final : public TopologyModel
	{
	private:
		int* _neibor_size_pointer;
		int* _neibor_id_pointer;
	public:
		GivenModel(int* neibor_size, int* neigbor_id)
			:TopologyModel()
		{
			_neibor_size_pointer = neibor_size;
			_neibor_id_pointer = neigbor_id;
		}

		~GivenModel()
		{

		}

		static void preAssert(AssertList& list)
		{

		}

		static void postAssert(AssertList& list)
		{

		}

		void build(Subswarm** subswarms, int& swarm_number)
		{
			int counter = 0;
			for (int i = 0; i < swarm_number; i++)
			{
				for (int j = 0; j < _neibor_size_pointer[i]; j++)
				{
					neighborhoods[i][j] = subswarms[_neibor_id_pointer[counter]];
					counter++;
				}
			}
		}

		void ini(Subswarm** subswarms, int& swarm_number)
		{
			delete[] neighborhoods;
			neighborhoods = new NeighborSwarm[swarm_number];
			for (int i = 0; i < swarm_number; i++)
			{
				neighborhoods[i].setSize(_neibor_size_pointer[i]);
			}

			build(subswarms, swarm_number);
		}
	};
}