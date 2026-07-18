//------------------------Description------------------------
// HeuristicManager:HeuristicDefine::compile() 的产物——启发域运行期引擎(原扁平句柄的启发切片)。
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
#include "ecflow-inspiration.h"   // Inspiration, domain_view
#include "variable-manager.h"

namespace ECFlow
{
    class HeuristicManager
    {
    public:
        int           variable_number             = 0;
        Inspiration** inspirations                = nullptr; // [variable_number] (owned; null for non-decision)
        int*          inspiration_variable_number = nullptr; // [variable_number]
        int**         inspiration_variable_index  = nullptr; // [variable_number][..] -> variable ids
        double**      inspirate_buffer            = nullptr; // [variable_number + 2] scratch

        HeuristicManager() = default;
        ~HeuristicManager() { destroy(); }
        HeuristicManager(HeuristicManager&& s) noexcept { steal(s); }
        HeuristicManager& operator=(HeuristicManager&& s) noexcept { if (this != &s) { destroy(); steal(s); } return *this; }
        HeuristicManager(const HeuristicManager& s) { cloneFrom(s); }
        HeuristicManager& operator=(const HeuristicManager& s) { if (this != &s) { destroy(); cloneFrom(s); } return *this; }

        // --- single-domain ops (dv from ConstraintManager; var for input addresses) ---
        double getHeuristic(int vid, domain_view& dv, int did, double choice, const VariableManager& var)
        {
            fillBuffer(vid, var);
            return inspirations[vid]->getHeuristic(dv, did, choice, inspirate_buffer);
        }
        double getPrioriDecision(int vid, domain_view& dv, int did, const VariableManager& var)
        {
            fillBuffer(vid, var);
            return inspirations[vid]->getPrioriDecision(dv, did, inspirate_buffer);
        }
        double* getPrioriOrder(int vid, domain_view& dv, int did, int& order_size, const VariableManager& var)
        {
            fillBuffer(vid, var);
            return inspirations[vid]->getPrioriOrder(dv, did, inspirate_buffer, order_size);
        }

    private:
        void fillBuffer(int vid, const VariableManager& var)
        {
            for (int i = 0; i < inspiration_variable_number[vid]; i++)
                inspirate_buffer[i] = var.variables[inspiration_variable_index[vid][i]].address;
        }
        void destroy()
        {
            if (inspirations) for (int i = 0; i < variable_number; i++) delete inspirations[i];
            delete[] inspirations;
            delete[] inspiration_variable_number;
            if (inspiration_variable_index) for (int i = 0; i < variable_number; i++) delete[] inspiration_variable_index[i];
            delete[] inspiration_variable_index;
            delete[] inspirate_buffer;
            inspirations = nullptr; inspiration_variable_number = nullptr;
            inspiration_variable_index = nullptr; inspirate_buffer = nullptr; variable_number = 0;
        }
        void steal(HeuristicManager& s)
        {
            variable_number = s.variable_number; inspirations = s.inspirations;
            inspiration_variable_number = s.inspiration_variable_number;
            inspiration_variable_index = s.inspiration_variable_index; inspirate_buffer = s.inspirate_buffer;
            s.inspirations = nullptr; s.inspiration_variable_number = nullptr;
            s.inspiration_variable_index = nullptr; s.inspirate_buffer = nullptr; s.variable_number = 0;
        }
        void cloneFrom(const HeuristicManager& s)
        {
            variable_number = s.variable_number;
            inspirations = new Inspiration*[variable_number];
            for (int i = 0; i < variable_number; i++)
                inspirations[i] = s.inspirations[i] ? s.inspirations[i]->clone() : nullptr;
            inspiration_variable_number = new int[variable_number];
            std::memcpy(inspiration_variable_number, s.inspiration_variable_number, variable_number * sizeof(int));
            inspiration_variable_index = new int*[variable_number];
            for (int i = 0; i < variable_number; i++)
            {
                inspiration_variable_index[i] = new int[inspiration_variable_number[i]];
                std::memcpy(inspiration_variable_index[i], s.inspiration_variable_index[i], inspiration_variable_number[i] * sizeof(int));
            }
            inspirate_buffer = new double*[variable_number + 2];
        }

        friend class HeuristicDefine;
    };
}
