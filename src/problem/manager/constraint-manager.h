//------------------------Description------------------------
// ConstraintManager:ConstrainDefine::compile() 的产物——约束域运行期引擎(原扁平句柄的约束切片)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <vector>
#include <cstring>
#include "ecflow-constrain.h"
#include "ecflow-accumulate.h"   // ConstrainCapacity + accumulate constraints
#include "ecflow-constant.h"        // EMPTYVALUE
#include "variable-manager.h"

namespace ECFlow
{
    struct Con4ElePair { std::vector<Constrain*> constrains; };

    class ConstraintManager
    {
    public:
        int variable_number          = 0;
        int decision_variable_number = 0;
        int object_number            = 0;

        Con4ElePair*  constrain_pairs          = nullptr;  // [variable_number]
        int           constrain_number         = 0;
        Constrain**   constrains               = nullptr;  // [constrain_number] (owned)
        int*          constrain_variable_index = nullptr;  // [constrain_number] -> variable id
        bool          constraint_check         = true;

        int*  objective_penalty_number = nullptr;          // [object_number]
        int** objective_penalty_index  = nullptr;          // [object_number][..] -> constraint id

        std::vector<interval_set> feasible_regions_ini;    // [decision_variable_number]
        std::vector<interval_set> feasible_regions_cur;
        interval_set              feasible_regions_dem;
        interval_set*             feasible_regions_dem_ptr = nullptr;
        int                       feasible_regions_dem_index = -1;
        bool*                     no_dem_reduction_region  = nullptr; // [decision_variable_number]

        ConstraintManager() = default;
        ~ConstraintManager() { destroy(); }
        ConstraintManager(ConstraintManager&& s) noexcept { steal(s); }
        ConstraintManager& operator=(ConstraintManager&& s) noexcept { if (this != &s) { destroy(); steal(s); } return *this; }
        ConstraintManager(const ConstraintManager& s) { cloneFrom(s); }
        ConstraintManager& operator=(const ConstraintManager& s) { if (this != &s) { destroy(); cloneFrom(s); } return *this; }

        // ================= single-domain runtime ops (param: VariableManager) =================

        void constrainReset()
        {
            for (int vid = 0; vid < variable_number; vid++)
                for (size_t i = 0; i < constrain_pairs[vid].constrains.size(); i++)
                    constrain_pairs[vid].constrains[i]->ini();
            for (int i = 0; i < decision_variable_number; i++)
                feasible_regions_cur[i] = feasible_regions_ini[i];
            feasible_regions_dem_index = -1;
        }

        bool constrainCheck(const VariableManager& var, int demensionId, double value)
        {
            if (is_empty(value)) return true;   // 未决定的维跳过检查(修复:原 == EMPTYVALUE 对 NaN 恒假 → 守卫从不触发)
            int vid = var.solution_belong_variable[demensionId];
            int did = var.getWithinVariableId(demensionId);
            for (size_t i = 0; i < constrain_pairs[vid].constrains.size(); i++)
                if (!constrain_pairs[vid].constrains[i]->meet(did, value)) return false;
            return true;
        }

        void constrainChange(const VariableManager& var, int demensionId, double value)
        {
            int vid = var.solution_belong_variable[demensionId];
            int dvi = var.variable_map_index[vid];
            int did = demensionId - var.decision_variable_offset[dvi];
            domain_view dv(feasible_regions_cur[dvi],
                           var.variables[vid].getLowbound(), var.variables[vid].getAccuracy(),
                           viewModeOf(var, vid));
            for (int i = 0; i < (int)constrain_pairs[vid].constrains.size(); i++)
            {
                constrain_pairs[vid].constrains[i]->update(did, value);
                if (constrain_pairs[vid].constrains[i]->getConstrainLevel() == constrains_variable)
                    constrain_pairs[vid].constrains[i]->regionReduction(did, dv);
            }
            feasible_regions_dem_index = -1;
        }

        // 长度提醒：getChoiceNumber = 真实可行点数；getFeasibleList 受 FEASIBLE_LIST_LIMIT
        // 降采样，超限时返回长度 < getChoiceNumber。二者长度不必一致。
        int    getChoiceNumber(const VariableManager& var, int dim) { ensureDem(var, dim); return demView(var, dim).count(); }
        double* getFeasibleList(const VariableManager& var, int dim) { ensureDem(var, dim); int n; return demView(var, dim).enumerate_alloc_capped(FEASIBLE_LIST_LIMIT, n); }
        double getBoundaryChoice(const VariableManager& var, int dim, bool left = true) { ensureDem(var, dim); return demView(var, dim).boundary(left); }
        double getCloseChoice(const VariableManager& var, int dim, double v) { ensureDem(var, dim); return demView(var, dim).closest(v); }
        double getRandomChoiceInspace(const VariableManager& var, int dim) { ensureDem(var, dim); return demView(var, dim).random(); }
        double getRandomChoice(const VariableManager& var, int dim)
        {
            int vid = var.solution_belong_variable[dim]; int dvi = var.variable_map_index[vid];
            return domain_view(feasible_regions_ini[dvi],
                               var.variables[vid].getLowbound(), var.variables[vid].getAccuracy(),
                               viewModeOf(var, vid)).random();
        }

        // penalty contributed to objective oid (reads bound variable addresses).
        double getPenalty4Object(const VariableManager& var, int oid)
        {
            double back = 0;
            for (int i = 0; i < objective_penalty_number[oid]; i++)
            {
                int cid = objective_penalty_index[oid][i];
                int vid = constrain_variable_index[cid];
                back += constrains[cid]->violation(var.variables[vid].address, var.variables[vid].getLength())
                        * constrains[cid]->getWeight();
            }
            return back;
        }

        // 违反度 = 全部已注册约束的**原始** violation 之和(不乘权重、不看目标绑定)。与原版 ECFlow 逐行等价。
        //   语义详见 ProblemHandle::constraintViolation 的注释(两条约束通路的分工 / 变量域为何计入)。
        double constraintViolation(const VariableManager& var)
        {
            double back = 0;
            for (int vid = 0; vid < variable_number; vid++)
                for (int i = 0; i < (int)constrain_pairs[vid].constrains.size(); i++)
                    back += constrain_pairs[vid].constrains[i]->violation(
                        var.variables[vid].address, var.variables[vid].getLength());
            return back;
        }

        // 合法性 = **或逻辑短路**:撞到第一条违反即返回,不再计算其余约束(与原版 ECFlow 同构)。
        //   不写成 `equal(constraintViolation(var), 0)` —— 那样虽在 violation 恒非负时等价,却**总要算完全部约束**;
        //   非法解常在头几条即可判定,短路省去其余计算(约束越多、越早违反,收益越大)。
        bool solutionLegality(const VariableManager& var)
        {
            for (int vid = 0; vid < variable_number; vid++)
                for (int i = 0; i < (int)constrain_pairs[vid].constrains.size(); i++)
                {
                    double violation = constrain_pairs[vid].constrains[i]->violation(
                        var.variables[vid].address, var.variables[vid].getLength());
                    if (!equal(violation, 0.0)) return false;   // 有一条不符合即检查不通过
                }
            return true;
        }

        // 取某维当前的域视图(须在 ensureDem 之后)。
        // ★ **全平台唯一的"变量类型 → 视图模式"分派点**。视图本身不认识 VariableType,
        //   调用方(优化器)也不感知模式 —— 它们只调 getRandomChoiceInspace / getFeasibleList 等,
        //   由本函数决定这些操作是按**网格**还是按**测度**解释。
        domain_view demView(const VariableManager& var, int dim)
        {
            int vid = var.solution_belong_variable[dim];
            return domain_view(*feasible_regions_dem_ptr,
                               var.variables[vid].getLowbound(),
                               var.variables[vid].getAccuracy(),
                               viewModeOf(var, vid));
        }

        // 变量类型 → 视图模式(转发到 domain_view.h 的自由函数,勿在别处硬写 mode)
        static ViewMode viewModeOf(const VariableManager& var, int vid)
        {
            return ECFlow::viewModeOf(var.getVariableType(vid));
        }

        // 把值吸附到该维的网格/输出精度(供 ProblemHandle::choiceDiscretized;生成器每写一个决策都经此)。
        //   **不调 ensureDem** —— snap 只读 (lowbound, accuracy, mode)、不碰可行域本身,
        //   若走 demView 会白白触发一次区域收缩(逐维生成的内循环里,代价可观且是副作用)。
        //   故借 feasible_regions_ini 建一个廉价视图(四个字段,无计算)只为调用 snap。
        double snapValue(const VariableManager& var, int dim, double value)
        {
            int vid = var.solution_belong_variable[dim];
            int dvi = var.variable_map_index[vid];
            if (dvi < 0) return value;   // 非决策变量:无域可言,原样返回
            return domain_view(feasible_regions_ini[dvi],
                               var.variables[vid].getLowbound(),
                               var.variables[vid].getAccuracy(),
                               viewModeOf(var, vid)).snap(value);
        }
        void ensureDem(const VariableManager& var, int dim) { if (feasible_regions_dem_index != dim) regionReduction(var, dim); }

    private:
        void regionReduction(const VariableManager& var, int demensionId)
        {
            int vid = var.solution_belong_variable[demensionId];
            int dvi = var.variable_map_index[vid];
            int did = demensionId - var.decision_variable_offset[dvi];

            if (no_dem_reduction_region[dvi])
            {
                feasible_regions_dem_ptr = &feasible_regions_cur[dvi];
                return;
            }
            feasible_regions_dem = feasible_regions_cur[dvi];
            feasible_regions_dem_ptr = &feasible_regions_dem;
            // 交给各约束的视图同样须带模式(否则约束传播会按网格解释连续变量)
            domain_view dv(feasible_regions_dem,
                           var.variables[vid].getLowbound(), var.variables[vid].getAccuracy(),
                           viewModeOf(var, vid));
            for (int i = 0; i < (int)constrain_pairs[vid].constrains.size(); i++)
                if (constrain_pairs[vid].constrains[i]->getConstrainLevel() == constraints_discrete)
                    constrain_pairs[vid].constrains[i]->regionReduction(did, dv);
            feasible_regions_dem_index = demensionId;
        }

        void destroy()
        {
            delete[] constrain_pairs;
            if (constrains) for (int i = 0; i < constrain_number; i++) delete constrains[i];
            delete[] constrains;
            delete[] constrain_variable_index;
            delete[] objective_penalty_number;
            if (objective_penalty_index) for (int i = 0; i < object_number; i++) delete[] objective_penalty_index[i];
            delete[] objective_penalty_index;
            delete[] no_dem_reduction_region;
            constrain_pairs = nullptr; constrains = nullptr; constrain_variable_index = nullptr;
            objective_penalty_number = nullptr; objective_penalty_index = nullptr; no_dem_reduction_region = nullptr;
            feasible_regions_ini.clear(); feasible_regions_cur.clear();
            feasible_regions_dem = interval_set(); feasible_regions_dem_ptr = nullptr;
            constrain_number = 0; object_number = 0;
        }
        void steal(ConstraintManager& s)
        {
            variable_number = s.variable_number; decision_variable_number = s.decision_variable_number;
            object_number = s.object_number; constraint_check = s.constraint_check;
            constrain_pairs = s.constrain_pairs; constrain_number = s.constrain_number;
            constrains = s.constrains; constrain_variable_index = s.constrain_variable_index;
            objective_penalty_number = s.objective_penalty_number; objective_penalty_index = s.objective_penalty_index;
            feasible_regions_ini = std::move(s.feasible_regions_ini);
            feasible_regions_cur = std::move(s.feasible_regions_cur);
            feasible_regions_dem = std::move(s.feasible_regions_dem);
            feasible_regions_dem_ptr = nullptr; feasible_regions_dem_index = -1;
            no_dem_reduction_region = s.no_dem_reduction_region;
            s.constrain_pairs = nullptr; s.constrains = nullptr; s.constrain_variable_index = nullptr;
            s.objective_penalty_number = nullptr; s.objective_penalty_index = nullptr; s.no_dem_reduction_region = nullptr;
            s.constrain_number = 0; s.object_number = 0;
        }
        void cloneFrom(const ConstraintManager& s)
        {
            variable_number = s.variable_number; decision_variable_number = s.decision_variable_number;
            object_number = s.object_number; constraint_check = s.constraint_check;
            constrain_number = s.constrain_number;
            constrains = new Constrain*[constrain_number];
            for (int i = 0; i < constrain_number; i++) constrains[i] = s.constrains[i]->clone();
            constrain_variable_index = new int[constrain_number];
            std::memcpy(constrain_variable_index, s.constrain_variable_index, constrain_number * sizeof(int));
            constrain_pairs = new Con4ElePair[variable_number];
            for (int cid = 0; cid < constrain_number; cid++)
                constrain_pairs[constrain_variable_index[cid]].constrains.push_back(constrains[cid]);
            objective_penalty_number = new int[object_number];
            std::memcpy(objective_penalty_number, s.objective_penalty_number, object_number * sizeof(int));
            objective_penalty_index = new int*[object_number];
            for (int o = 0; o < object_number; o++)
            {
                objective_penalty_index[o] = new int[objective_penalty_number[o]];
                std::memcpy(objective_penalty_index[o], s.objective_penalty_index[o], objective_penalty_number[o] * sizeof(int));
            }
            feasible_regions_ini = s.feasible_regions_ini;
            feasible_regions_cur = s.feasible_regions_cur;
            feasible_regions_dem = interval_set(); feasible_regions_dem_ptr = nullptr; feasible_regions_dem_index = -1;
            no_dem_reduction_region = new bool[decision_variable_number];
            std::memcpy(no_dem_reduction_region, s.no_dem_reduction_region, decision_variable_number * sizeof(bool));
        }
    };
}
