//------------------------Description------------------------
// ObjectiveManager:ObjectiveDefine::compile() 的产物——目标域运行期引擎(原扁平句柄的目标切片)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <vector>
#include <string>
#include "objective.h"     // Objective
#include "comparer.hpp"    // Comparer
#include "variable-manager.h"           // VariableManager (for getFitness)

namespace ECFlow
{
    class ObjectiveManager
    {
    public:
        Objective* objectives = nullptr;                       // [object_number] (owned)
        int        object_number = 0;
        std::vector<std::vector<int>> objective_variable_index; // [oid] -> input variable ids
        std::vector<double*>          evaluate_buffer;          // scratch (sized to variable_number)
        Comparer*  comparer = nullptr;                          // owned (priority-based)
        bool       full_evaluate = true;                        // evaluation mode: full / partial(prio>=0 only)
        std::vector<int> positive_priority_index;               // participating oids (priority >= 0)

        ObjectiveManager() = default;
        ~ObjectiveManager() { destroy(); }

        ObjectiveManager(ObjectiveManager&& s) noexcept { steal(s); }
        ObjectiveManager& operator=(ObjectiveManager&& s) noexcept { if (this != &s) { destroy(); steal(s); } return *this; }
        ObjectiveManager(const ObjectiveManager& s) { cloneFrom(s); }
        ObjectiveManager& operator=(const ObjectiveManager& s) { if (this != &s) { destroy(); cloneFrom(s); } return *this; }

        // --- single-domain op: fitness of objective oid (reads variable addresses) ---
        double getFitness(int oid, const VariableManager& var)
        {
            const auto& idx = objective_variable_index[oid];
            for (int i = 0; i < (int)idx.size(); i++)
                evaluate_buffer[i] = var.variables[idx[i]].address;
            return objectives[oid].getFitness(evaluate_buffer.data());
        }

        int         objectNumber()      const { return object_number; }
        bool        isMin(int oid)      const { return objectives[oid].IsMin(); }
        std::string objectiveName(int oid) const { return objectives[oid].getName(); }
        Comparer*   getComparer()       const { return comparer; }

        // --- query (2.3) ---
        int getObjectiveId(const std::string& name) const
        {
            for (int i = 0; i < object_number; i++)
                if (objectives[i].getName() == name) return i;
            return -1;
        }
        int getObjectivePriority(int oid) const { return objectives[oid].priority(); }

        // --- runtime priority editing (2.4) ---
        // Rejects (returns false, no mutation) any change that would leave zero
        // participating objectives (all priority < 0) -> preserves the invariant
        // "at least one objective with priority >= 0" at runtime (see docs F.0/F.1).
        bool changePriority(int oid, int new_priority)
        {
            if (oid < 0 || oid >= object_number) return false;
            int participating = 0;
            for (int i = 0; i < object_number; i++)
            {
                int p = (i == oid) ? new_priority : objectives[i].priority();
                if (p >= 0) participating++;
            }
            if (participating == 0) return false;            // would break the invariant
            objectives[oid].setPriority(new_priority);
            delete comparer; comparer = new Comparer(objectives, object_number);
            rebuildPositiveIndex();
            return true;
        }
        bool changePriority(const std::string& name, int new_priority)
        { return changePriority(getObjectiveId(name), new_priority); }

        // --- evaluation mode (2.5) ---
        void setEvaluateMode(bool full) { full_evaluate = full; }
        bool getEvaluateMode() const    { return full_evaluate; }
        const std::vector<int>& participatingObjectives() const { return positive_priority_index; }

        // (re)build the participating set from current priorities; called by
        // ObjectiveDefine::compile() and after every changePriority().
        void rebuildPositiveIndex()
        {
            positive_priority_index.clear();
            for (int i = 0; i < object_number; i++)
                if (objectives[i].priority() >= 0) positive_priority_index.push_back(i);
        }

    private:
        void destroy()
        {
            delete[] objectives; objectives = nullptr;
            delete comparer; comparer = nullptr;
            object_number = 0;
            objective_variable_index.clear();
            evaluate_buffer.clear();
            positive_priority_index.clear();
        }
        void steal(ObjectiveManager& s)
        {
            objectives = s.objectives; object_number = s.object_number;
            objective_variable_index = std::move(s.objective_variable_index);
            evaluate_buffer = std::move(s.evaluate_buffer);
            comparer = s.comparer;
            full_evaluate = s.full_evaluate;
            positive_priority_index = std::move(s.positive_priority_index);
            s.objectives = nullptr; s.comparer = nullptr; s.object_number = 0;
        }
        void cloneFrom(const ObjectiveManager& s)
        {
            object_number = s.object_number;
            objectives = new Objective[object_number];
            for (int i = 0; i < object_number; i++) objectives[i].copy(s.objectives + i);
            objective_variable_index = s.objective_variable_index;
            evaluate_buffer.assign(s.evaluate_buffer.size(), nullptr);
            comparer = new Comparer(objectives, object_number);
            full_evaluate = s.full_evaluate;
            positive_priority_index = s.positive_priority_index;
        }
    };
}
