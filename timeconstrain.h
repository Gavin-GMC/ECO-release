//------------------------Description------------------------
// This file defines the time variable, which supports the 
// definition of cumulative functions and constraints for
// time-dependent problems.
//-------------------------Reference-------------------------
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFC（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFC" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include<map>
#include<unordered_set>
#include"sort-helper.h"
#include"feasible-region.h"
#include"basicfunc.h"
#include"variable.h"
#include"constrain.h"

namespace ECFC
{
	// 时间变量属于一种特殊的计算量，会根据决策情况进行计算更新
	// 时间变量可以对外提供若干监测函数，如时间跨度，平均占用时间，空闲率，资源使用标准差等函数，用于目标定义
	// 同时，部分约束只适用于时间变量，顺序约束，起始时间约束，截至时间约束等
	// 其检测时间变量但作用于决策变量

	// 问题：
	// 如何在时间变量和决策变量间构造关联-链接到对应的决策量（其余计算量上的约束也可以参考这一点）
	//

	// 离散约束（时间间隔约束）：
	// 包含顺序约束、起止点约束（产生时间与截止时间）、
	// 顺序约束中，所有任务应只受之前任务的约束而不受后续任务的约束
	
	// 这里写一下中间量（计算量）的思路
	// 中间量即基于其它决策量计算获得的量，可以是多项式也可以是时间量
	// 其基于决策量，约束也作用于决策量，因此构建一个双向表是必要的
	// 先全部考虑单变量约束，多变量参与的约束以后再考虑
	// 但是这种约束的传播怎么进行呢？
	// 框架流程：
	// 1.决策量变化
	// 2.关联计算量变化
	// 3.作用到决策量之上
	// 
	// 
	// 时间量是什么，有什么：
	// 时间量本质是一个对目标系统时间进程的仿真和记录
	// 它应包括各个时间的时间点及各个时间点上各个容器的动作与状态
	// 它会被施加一些约束，如处理顺序约束，截止时间约束
	// 也可能结合到其它约束，如容量约束，来提供动作的触发
	// 
	// 针对这一情况，考虑到目前可能结合到时间量的只有容量约束，
	// 先只针对时间量再定义一个容量约束，后续再做维护
	//

	class ConstrainTimeInterval final : public Constrain
	{
	private:
		int _container_number;
		int _task_number;

		double* _occupy_time; // 任务在各节点占用时间，【taskid】【nodeid】
		double* _task_preperation_time; // 任务准备时间，【taskid】【nodeid】
		double** _task_output_time; // 任务传输时间，【taskid】【destination】【location】
		bool* _task_precedence; // 任务依赖关系，【taskid】【taskid】

		struct Event
		{
		public:
			int taskid;
			int action;
			double timestamp;

			Event(int tskid, int actn, double tm)
			{
				taskid = tskid;
				action = actn;
				timestamp = tm;
			}

			bool operator<(const Event& e) const
			{
				return timestamp < e.timestamp;
			}
		};
		std::vector<Event>* _scheme; // 节点分配任务【nodeid】【taskid】		
		double* ends; // 任务完成时间，【taskid】
		int* allocates; // 任务部署位置，【taskid】

		// 约束相关量
		double* _task_deadline; // 任务截止日期 【taskid】
		int _resource_types; // 资源种类数目
		double* _capacitys; // 资源容量【nodeid】【resourceid】
		double* _requirements; //资源需求【taskid】【resourceid】

		double getEarliestAvailable(int task_id, int container_id)
		{
			if (_resource_types == 0)
				return 0;

			double* resource_buffer = new double[_resource_types];
			double* require_pointer = _requirements + task_id * _resource_types;
			memcpy(resource_buffer, _capacitys + container_id * _resource_types, _resource_types * sizeof(double));
			
			// 需要在指定时间区间内均满足各类资源需求
			double start = 0;
			double hold = _occupy_time[task_id * _container_number + container_id];
			double end = start + hold;
			bool has_meet = true;

			// 最大容量检查
			for (int r = 0; r < _resource_types; r++)
			{
				if (resource_buffer[r] < require_pointer[r])
				{
					has_meet = false;
					break;
				}
			}
			if (!has_meet)
			{
				return ECFC_MAX;
			}

			for (int i = 0; i < _scheme[container_id].size(); i++)
			{
				if (has_meet && _scheme[container_id][i].timestamp >= end)
					return start;
				for (int j = 0; j < _resource_types; j++)
				{
					resource_buffer[j] -= _scheme[container_id][i].action * _requirements[_scheme[container_id][i].taskid * _resource_types + j];
				}
				if (has_meet)
				{
					if (_scheme[container_id][i].action < 0)
						continue;
					else
					{
						for (int r = 0; r < _resource_types; r++)
						{
							if (resource_buffer[r] < require_pointer[r])
							{
								has_meet = false;
								break;
							}
						}
					}
				}
				else
				{
					if (_scheme[container_id][i].action > 0)
						continue;
					else
					{
						bool success = true;
						for (int r = 0; r < _resource_types; r++)
						{
							if (resource_buffer[r] < require_pointer[r])
							{
								success = false;
								break;
							}
						}
						if (success)
						{
							has_meet = true;
							start = _scheme[container_id][i].timestamp;
							end = start + hold;
						}
					}
				}
			}

			return start;
		}

		double getEarliestStart(int task_id, int container_id)
		{
			double back = 0;
			if (_task_preperation_time != nullptr)
				back = _task_preperation_time[task_id * _container_number + container_id];

			if (_task_precedence != nullptr)
			{
				bool* precedence = _task_precedence + task_id * _task_number;
				double* trans_time = _task_output_time[task_id] + container_id * _container_number;
				double getdata;
				for (int i = 0; i < task_id; i++)
				{
					if (precedence[i])
					{
						getdata = ends[i] + trans_time[allocates[i]];
						if (getdata > back)
						{
							back = getdata;
						}
					}
				}
			}

			return back;
		}

	public:
		ConstrainTimeInterval(double penalty_w, int task_number, int container_number,
			double* occupy_time = nullptr, double* task_output_time = nullptr,
			double* task_preperation_time = nullptr, int* task_precedence = nullptr,
			double* task_deadline = nullptr, int resource_types = 0, double* capacitys = nullptr)
			:Constrain(penalty_w)
		{

		}

		~ConstrainTimeInterval()
		{

		}

		double systemSpan(double** input)
		{

		}

		double systemBalance(double** input)
		{

		}

		void ini()
		{
			for (int i = 0; i < _container_number; i++)
			{
				_scheme[i].clear();
			}
		}

		bool meet(int demension, double value)
		{
			if (_task_deadline == nullptr)
				return true;

			double start = getEarliestStart(demension, value);
			return (start + _occupy_time[demension * _container_number + int(value)]) < _task_deadline[demension];
		}

		void update(int demension, double value)
		{
			double start = getEarliestStart(demension, value);

			std::vector<Event>* _scheme; // 节点分配任务【nodeid】【taskid】		
			ends[demension] = start + _occupy_time[demension * _container_number + int(value)];
			allocates[demension] = value;

			std::vector<Event>* scheme = _scheme + allocates[demension];
			bool inserted = false;
			for(std::vector<Event>::iterator it = (*scheme).begin(); it!= (*scheme).end(); it++)
			{
				if (it->timestamp > start) //开始执行
				{
					(*scheme).insert(it, Event(demension, 1, start));
					inserted = true;
					break;
				}
			}
			if (!inserted) // 部署到方案末尾
			{
				(*scheme).push_back(Event(demension, 1, start));
				(*scheme).push_back(Event(demension, -1, ends[demension]));
			}
			else
			{
				// 插入终止事件
				inserted = false;
				for (std::vector<Event>::iterator it = (*scheme).begin(); it != (*scheme).end(); it++)
				{
					if (it->timestamp > ends[demension])
					{
						(*scheme).push_back(Event(demension, -1, ends[demension]));
						inserted = true;
						break;
					}
				}
				if (!inserted)
				{
					(*scheme).push_back(Event(demension, -1, ends[demension]));
				}
			}
		}

		double violation(double* solution, int size)
		{
			if (_task_deadline == nullptr)
				return 0;

			ini();
			for (int i = 0; i < size; i++)
			{
				update(i, solution[i]);
			}
			
			double back = 0;
			double over;
			for (int i = 0; i < size; i++)
			{
				over = ends[i] - _task_deadline[i];
				if (over > 0)
					back += over;
			}

			return over;
		}

		void regionReduction(int demensionId, FeasibleLine* region)
		{
			if (_task_deadline == nullptr)
				return;

			for (int cid = 0; cid < _container_number; cid++)
			{
				if (!meet(demensionId, cid))
				{
					region->differ(cid, cid, false, false);
				}
			}
		}
	};
}