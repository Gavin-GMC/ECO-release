//------------------------Description------------------------
// This file contents some basic and commonly used function in
// multi-objective optimization.
//-------------------------Reference-------------------------
// [1]Deb, Kalyan & Pratap, Amrit & Agarwal, Sameer & Meyarivan, 
// T.. (2002). A fast and elitist multiobjective genetic algorithm:
//  NSGA-II. Evolutionary Computation, IEEE Transactions on. 6. 
// 182 - 197. 10.1109/4235.996017. 
// [2]Branke, Juergen & Deb, Kalyan & Dierolf, Henning & Osswald,
//  Matthias. (2004). Finding Knees in Multi-objective Optimization.
//  Parallel Problem Solving from Nature - PPSN VIII. 722-731.
//  10.1007/978-3-540-30217-9_73. 
//-------------------------Copyright-------------------------
// Copyright (c) 2022 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFA（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFA" and reference "未确定"
//-----------------------------------------------------------

#pragma once
#include<vector>
#include<cmath>
#include"population.h"
#include"sort-helper.h"

namespace ECFC {
	class MO
	{
	private:
		static sortHelper<int, double>* MOsortbuffer;
		static int MObuffersize;

	public:
		//calculate the dominate rank of individuals in swarm[1]
		static void fastNonDominatedSort(Population& swarm, size_t ss, double* rank_back)
		{
			std::vector<int> n;
			int np, ra;
			std::vector<std::vector<int>> S;
			S.resize(ss);
			std::vector<int>F;
			std::vector<int>Q;

			for (int i = 0; i < ss; i++)
			{
				np = 0;
				for (int j = 0; j < ss; j++)
				{
					if (i != j)
					{
						if (swarm[i] < swarm[j])
							S[i].push_back(j);
						else if (swarm[j] < swarm[i])
							np += 1;
					}
				}
				n.push_back(np);
				if (np == 0)
				{
					rank_back[i] = 1;
					F.push_back(i);
				}
			}

			ra = 1;
			while (!F.empty())
			{
				Q.clear();
				for (int i = 0; i < F.size(); i++)
				{
					for (int j = 0; j < S[F[i]].size(); j++)
					{
						n[S[F[i]][j]] -= 1;
						if (n[S[F[i]][j]] == 0)
						{
							rank_back[S[F[i]][j]] = ra + 1;
							Q.push_back(S[F[i]][j]);
						}
					}
				}
				ra += 1;
				F = Q;
			}
		}

		//Calculate the crowd distance of individuals in swarm[1]
		static void CrowdDistance(Population& swarm, size_t ss, double* distance_back)
		{
			if (MObuffersize < ss)
			{
				delete[] MOsortbuffer;
				MOsortbuffer = new sortHelper<int, double>[ss];
				MObuffersize = ss;
			}

			for (int i = 0; i < ss; i++)
				distance_back[i] = 0;

			int object_number = swarm[0].getObjectNumber();

			for (int o = 0; o < object_number; o++)
			{
				for (int i = 0; i < ss; i++)
				{
					MOsortbuffer[i].id = i;
					MOsortbuffer[i].value = swarm[i].solution.fitness[o];
				}
				std::sort(MOsortbuffer, MOsortbuffer + ss);

				for (int i = 1; i < ss - 1; i++)
				{
					distance_back[MOsortbuffer[i].id] -=
						MOsortbuffer[i + 1].value - MOsortbuffer[i - 1].value;
				}
				distance_back[MOsortbuffer[0].id] = distance_back[MOsortbuffer[ss - 1].id] = -1 * ECFC_MAX;
			}
		}

		//Calculate the crowd distance of individuals in swarm after nomorlization[1]
		static void normalizeCrowdDistance(Population& swarm, size_t ss, double* distance_back)
		{
			if (MObuffersize < ss)
			{
				delete[] MOsortbuffer;
				MOsortbuffer = new sortHelper<int, double>[ss];
				MObuffersize = ss;
			}

			for (int i = 0; i < ss; i++)
				distance_back[i] = 0;

			int object_number = swarm[0].getObjectNumber();

			for (int o = 0; o < object_number; o++)
			{
				for (int i = 0; i < ss; i++)
				{
					MOsortbuffer[i].id = i;
					MOsortbuffer[i].value = swarm[i].solution.fitness[o];
				}
				std::sort(MOsortbuffer, MOsortbuffer + ss);

				for (int i = 1; i < ss - 1; i++)
				{
					distance_back[MOsortbuffer[i].id] -=
						(MOsortbuffer[i + 1].value - MOsortbuffer[i - 1].value) / (MOsortbuffer[ss - 1].value - MOsortbuffer[0].value);
				}
				distance_back[MOsortbuffer[0].id] = distance_back[MOsortbuffer[ss - 1].id] = -1 * ECFC_MAX;
			}
		}

		// 基于与目标点距离的排序
		template <class T = Solution>
		static void referenceDRank(T swarm[], size_t ss, double* referencePoint, int objective_number, int rank_number, double* rank_back)
		{
			if (MObuffersize < ss)
			{
				delete[] MOsortbuffer;
				MOsortbuffer = new sortHelper[ss];
				MObuffersize = ss;
			}

			int rank_size = ss / rank_number;
			if (ss % rank_number != 0)
				rank_size++;

			for (int i = 0; i < ss; i++)
			{
				MOsortbuffer[i].id = i;
				MOsortbuffer[i].value = Eu_distance(swarm[i].fitness, referencePoint, objective_number);
			}

			std::sort(MOsortbuffer, MOsortbuffer + ss);
			for (int i = 0; i < ss; i++)
			{
				rank_back[MOsortbuffer[i].id] = i / rank_size;
			}
		}

		//only for bi-objective optimization, return the rate of slope changing around solutions, which have been normalized[2]
		template <class T = Solution>
		static void CrowdAngle(T swarm[], size_t ss, double* angle_back)
		{
			if (MObuffersize < ss)
			{
				delete[] MOsortbuffer;
				MOsortbuffer = new sortHelper[ss];
				MObuffersize = ss;
			}

			double f1range, f2range;
			int length = 0;

			for (int i = 0; i < ss; i++)
			{
				MOsortbuffer[i].id = i;
				MOsortbuffer[i].value = swarm[i].fitness[0];
			}
			std::sort(MOsortbuffer, MOsortbuffer + ss);

			f1range = fabs(swarm[MOsortbuffer[0].id].fitness[0] - swarm[MOsortbuffer[ss - 1].id].fitness[0]);
			f2range = fabs(swarm[MOsortbuffer[0].id].fitness[1] - swarm[MOsortbuffer[ss - 1].id].fitness[1]);

			//remove the coincident points
			for (int i = 0; i < ss - 1; i++)
			{
				if (MOsortbuffer[i].value == MOsortbuffer[i + 1].value)
				{
					angle_back[MOsortbuffer[i].id] = 1;
					MOsortbuffer[i].id = EMPTYVALUE;
				}
			}
			for (int i = 0; i < ss; i++)
			{
				if (MOsortbuffer[i].id != EMPTYVALUE)
					MOsortbuffer[length++].id = MOsortbuffer[i].id;
			}

			for (int i = 1; i < length - 1; i++)
			{
				angle_back[MOsortbuffer[i].id] = -1 * fabs(
					getSlope(swarm[MOsortbuffer[i - 1].id].fitness[0] / f1range, swarm[MOsortbuffer[i - 1].id].fitness[1] / f2range,
						swarm[MOsortbuffer[i].id].fitness[0] / f1range, swarm[MOsortbuffer[i].id].fitness[1] / f2range)
					- getSlope(swarm[MOsortbuffer[i].id].fitness[0] / f1range, swarm[MOsortbuffer[i].id].fitness[1] / f2range,
						swarm[MOsortbuffer[i + 1].id].fitness[0] / f1range, swarm[MOsortbuffer[i + 1].id].fitness[1] / f2range));
			}
			angle_back[MOsortbuffer[0].id] = angle_back[MOsortbuffer[length - 1].id] = 0;
		}

		// sort the swarm by given indicators(small is better)
		// 返回解索引的排名，指标相同的保序排列
		static void buildPartialOrder(size_t ss, double** indicator, int indicator_number, double* order_back)
		{
			// 插入排序
			bool better;
			for (int i = 0; i < ss; i++)
			{
				order_back[i] = i;
				better = false;
				for (int j = 0; j < i; i++)
				{
					for (int k = 0; k < indicator_number; k++)
					{
						if (indicator[k][i] < indicator[k][int(order_back[j])]) // 好于当前个体
						{
							better = true;
							break;
						}
						if (indicator[k][i] > indicator[k][int(order_back[j])])// 差于当前个体
						{
							break;
						}
					}
					
					if (better)
					{
						// 插入
						for (int d = i; d > j; d--)
						{
							order_back[d] = order_back[d - 1];
						}
						order_back[j] = i;

						break;
					}
				}
			}
		}

		//sort the swarm by the main indicators
		template <class T>
		static void buildPartialOrder(T swarm[], size_t ss, double* indicator, int indicator_number, int main_indacator)
		{
			bool better;
			double indi_buffer;
			T* t_buffer = (T*)malloc(sizeof(T));

			//insert sort
			for (int i = 0; i < ss; i++)
			{
				for (int j = 0; j < i; j++)
				{
					//compare individual
					better = (indicator[main_indacator * ss + i] < indicator[main_indacator * ss + j]);

					//insert individual
					if (better)
					{
						*t_buffer = swarm[i];
						for (int j1 = i; j1 > j; j1--)
							swarm[j1] = swarm[j1 - 1];
						swarm[j] = *t_buffer;

						for (int k = 0; k < indicator_number; k++)
						{
							indi_buffer = indicator[k * ss + i];
							for (int j1 = i; j1 > j; j1--)
								indicator[k * ss + j1] = indicator[k * ss + j1 - 1];
							indicator[k * ss + j] = indi_buffer;
						}
						break;
					}
				}
			}

			free(t_buffer);
		}

		/*
		//update the pareto front archieve
		static bool paretoFrontUpdate(Solution* candidate, Solution pf[], int& pf_size)
		{
			Solution* temp = (Solution*)malloc(sizeof(Solution));
			for (int i = 0; i < pf_size; i++)
			{
				if (pf[i] < *candidate || pf[i] == *candidate)
					return false;
				if (*candidate < pf[i])
				{
					*temp = std::move(pf[i]);
					pf[i] = std::move(pf[pf_size - 1]);
					pf[pf_size - 1] = std::move(*temp);

					i--;
					pf_size--;
				}
			}
			free(temp);
			if (pf_size == (PFSIZE - 1))
				return false;

			pf[pf_size++].copy(*candidate);
			return true;
		}

		//estimate the hypervolume by MonteCarlo estimation
		template<class T = Solution>
		static void estimateHyperVolume(T swarm[], size_t ss, double* referencePoint, int objective_number, double& HV_back)
		{
			double* o_max = new double[objective_number];
			double* o_min = new double[objective_number];

			//build max and min vector
			for (int i = 0; i < objective_number; i++)
			{
				o_max[i] = referencePoint[i];
				o_min[i] = referencePoint[i];
			}

			for (int i = 0; i < ss; i++)
			{
				for (int j = 0; j < objective_number; j++)
				{
					if (swarm[i].fitness[j] < o_min[j])
						o_min[j] = swarm[i].fitness[j];
					else if (swarm[i].fitness[j] > o_max[j])
						o_max[j] = swarm[i].fitness[j];

				}
			}

			int sample_number = 1e6;
			int counter = 0;
			double totalVolume = 1;
			for (int i = 0; i < objective_number; i++)
				totalVolume *= (o_max[i] - o_min[i]);

			double* left_value = new double[ss * objective_number];
			double* right_value = new double[ss * objective_number];
			for (int i = 0; i < ss; i++)
			{
				for (int j = 0; j < objective_number; j++)
				{
					if (swarm[i].fitness[j] < referencePoint[j])
					{
						left_value[i * objective_number + j] = swarm[i].fitness[j];
						right_value[i * objective_number + j] = referencePoint[j];
					}
					else {
						left_value[i * objective_number + j] = referencePoint[j];
						right_value[i * objective_number + j] = swarm[i].fitness[j];
					}
				}
			}


			bool inCube;
			double* sample_point = new double[objective_number];

			for (int i = 0; i < sample_number; i++)
			{
				//sample a point in sampling room
				for (int j = 0; j < objective_number; j++)
					sample_point[j] = rand01_() * (o_max[j] - o_min[j]) + o_min[j];

				//judge whether the point is in the hyperCube
				for (int j = 0; j < ss; j++)
				{
					inCube = true;
					for (int k = 0; k < objective_number; k++)
					{
						//not in the hyperCube
						if (sample_point[k] < left_value[j * ss + k] || sample_point[k] > right_value[j * ss + k])
						{
							inCube = false;
							break;
						}
					}

					if (inCube)
					{
						counter++;
						break;
					}
				}
			}

			HV_back = double(counter) / sample_number * totalVolume;

			delete[] left_value;
			delete[] right_value;
			delete[] sample_point;

			delete[] o_max;
			delete[] o_min;
		}

		//calculate the exact hypervolume
		template <class T = Solution>
		static void hyperVolume(T swarm[], size_t ss, double* referencePoint, int objective_number, double& HV_back)
		{
			if (MObuffersize < ss)
			{
				delete[] MOsortbuffer;
				MOsortbuffer = new sortHelper[ss];
				MObuffersize = ss;
			}

			if (objective_number == 2)
			{
				HV_back = 0;
				int buffersize;
				double pre_y;

				//the point in quadrant 1
				{
					buffersize = 0;
					pre_y = referencePoint[1];
					for (int j = 0; j < ss; j++)
						if (swarm[j].fitness[0] > referencePoint[0] && swarm[j].fitness[1] > referencePoint[1])
						{
							MOsortbuffer[buffersize].id = j;
							MOsortbuffer[buffersize].value = swarm[j].fitness[0];
							buffersize++;
						}
					std::sort(MOsortbuffer, MOsortbuffer + buffersize, std::greater<sortHelper>());
					for (int i = 0; i < buffersize; i++)
					{
						HV_back += (swarm[MOsortbuffer[i].id].fitness[0] - referencePoint[0]) * (swarm[MOsortbuffer[i].id].fitness[1] - pre_y);
						pre_y = swarm[MOsortbuffer[i].id].fitness[1];
					}
				}

				//the point in quadrant 2
				{
					buffersize = 0;
					pre_y = referencePoint[1];
					for (int j = 0; j < ss; j++)
						if (swarm[j].fitness[0] < referencePoint[0] && swarm[j].fitness[1] > referencePoint[1])
						{
							MOsortbuffer[buffersize].id = j;
							MOsortbuffer[buffersize].value = swarm[j].fitness[0];
							buffersize++;
						}
					std::sort(MOsortbuffer, MOsortbuffer + buffersize, std::less<sortHelper>());
					for (int i = 0; i < buffersize; i++)
					{
						HV_back += (referencePoint[0] - swarm[MOsortbuffer[i].id].fitness[0]) * (swarm[MOsortbuffer[i].id].fitness[1] - pre_y);
						pre_y = swarm[MOsortbuffer[i].id].fitness[1];
					}
				}

				//the point in quadrant 3
				{
					buffersize = 0;
					pre_y = referencePoint[1];
					for (int j = 0; j < ss; j++)
						if (swarm[j].fitness[0] < referencePoint[0] && swarm[j].fitness[1] < referencePoint[1])
						{
							MOsortbuffer[buffersize].id = j;
							MOsortbuffer[buffersize].value = swarm[j].fitness[0];
							buffersize++;
						}
					std::sort(MOsortbuffer, MOsortbuffer + buffersize, std::less<sortHelper>());
					for (int i = 0; i < buffersize; i++)
					{
						HV_back += (referencePoint[0] - swarm[MOsortbuffer[i].id].fitness[0]) * (pre_y - swarm[MOsortbuffer[i].id].fitness[1]);
						pre_y = swarm[MOsortbuffer[i].id].fitness[1];
					}
				}

				//the point in quadrant 4
				{
					buffersize = 0;
					pre_y = referencePoint[1];
					for (int j = 0; j < ss; j++)
						if (swarm[j].fitness[0] > referencePoint[0] && swarm[j].fitness[1] < referencePoint[1])
						{
							MOsortbuffer[buffersize].id = j;
							MOsortbuffer[buffersize].value = swarm[j].fitness[0];
							buffersize++;
						}
					std::sort(MOsortbuffer, MOsortbuffer + buffersize, std::greater<sortHelper>());
					for (int i = 0; i < buffersize; i++)
					{
						HV_back += (swarm[MOsortbuffer[i].id].fitness[0] - referencePoint[0]) * (pre_y - swarm[MOsortbuffer[i].id].fitness[1]);
						pre_y = swarm[MOsortbuffer[i].id].fitness[1];
					}
				}
			}
			else if (objective_number == 3)
			{
				//it should be re-impletement in appropriate way later
				estimateHyperVolume(swarm, ss, referencePoint, objective_number, HV_back);
			}
			else {
				//it should be re-impletement in appropriate way later
				estimateHyperVolume(swarm, ss, referencePoint, objective_number, HV_back);
			}
		}*/
	};

	sortHelper<int, double>* MO::MOsortbuffer = nullptr;
	int MO::MObuffersize = 0;
}