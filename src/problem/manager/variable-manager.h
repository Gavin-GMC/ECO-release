//------------------------Description------------------------
// VariableManager:VariableDefine::compile() 的产物——变量域运行期引擎(原扁平句柄的变量切片)。
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
#include "variable.h"   // ElementNote
#include "solution.h"   // Solution

namespace ECFlow
{
    // Runtime variable: note + bound address (decision vars alias the solution vector).
    struct RTVariable
    {
        ElementNote note;
        double*     address = nullptr;
        int    getLength()   const { return note._length; }
        double getUpbound()  const { return note._upbound; }
        double getLowbound() const { return note._lowbound; }
        double getAccuracy() const { return note._accuracy; }
    };

    class VariableManager
    {
    public:
        // --- compiled-immutable layout ---
        int problem_size             = 0;
        int variable_number          = 0;
        int decision_variable_number = 0;

        std::vector<RTVariable> variables;              // all variables (const + decision)
        std::vector<int> decision_variable_index;       // decision var ids
        std::vector<int> decision_variable_offset;      // each decision var's offset in solution
        std::vector<int> solution_belong_variable;      // solution dim -> variable id
        std::vector<int> variable_map_index;            // variable id -> index within its sub-group
        std::vector<std::vector<double>> owned_values;  // constant buffers; empty for decision

        VariableManager() = default;
        VariableManager(VariableManager&&) noexcept = default;
        VariableManager& operator=(VariableManager&&) noexcept = default;

        // Deep copy: duplicate constant buffers, re-point addresses into THIS copy.
        VariableManager(const VariableManager& s)
            : problem_size(s.problem_size), variable_number(s.variable_number),
              decision_variable_number(s.decision_variable_number),
              variables(s.variables),
              decision_variable_index(s.decision_variable_index),
              decision_variable_offset(s.decision_variable_offset),
              solution_belong_variable(s.solution_belong_variable),
              variable_map_index(s.variable_map_index),
              owned_values(s.owned_values)
        {
            rebindOwned();
        }
        VariableManager& operator=(const VariableManager& s)
        {
            if (this != &s) { VariableManager t(s); *this = std::move(t); }
            return *this;
        }

        void rebindOwned()
        {
            for (int i = 0; i < (int)variables.size(); i++)
                variables[i].address = owned_values[i].empty() ? nullptr : owned_values[i].data();
        }

        // --- runtime ops (single-domain) ---
        // Bind decision variables to a solution vector (decision vars alias it).
        void setResult(const Solution& solution)
        {
            for (int i = 0; i < decision_variable_number; i++)
                variables[decision_variable_index[i]].address = solution.result + decision_variable_offset[i];
        }
        void setResult(double* solution)
        {
            for (int i = 0; i < decision_variable_number; i++)
                variables[decision_variable_index[i]].address = solution + decision_variable_offset[i];
        }

        // --- queries ---
        int getProblemSize()            const { return problem_size; }
        int getVariableNumber()         const { return variable_number; }
        int getDecisionVariableNumber() const { return decision_variable_number; }

        int getVariableId(const std::string& name) const
        {
            for (int i = 0; i < (int)variables.size(); i++)
                if (variables[i].note._name == name) return i;
            return -1;
        }
        int getBelongVariableId(int demensionId) const { return solution_belong_variable[demensionId]; }
        int getWithinVariableId(int demensionId) const
        {
            int vid = solution_belong_variable[demensionId];
            return demensionId - decision_variable_offset[variable_map_index[vid]];
        }
        int getVariableOffset(int vid)  const { return decision_variable_offset[variable_map_index[vid]]; }
        int getVariableLength(int vid)  const { return variables[vid].getLength(); }
        VariableType getVariableType(int vid) const { return variables[vid].note._type; }   // carried from addVariable (original ECElement::getType parity)
        double getVariableUpbound(int demensionId)  const { return variables[solution_belong_variable[demensionId]].getUpbound(); }
        double getVariableLowbound(int demensionId) const { return variables[solution_belong_variable[demensionId]].getLowbound(); }

        // Decision-variable notes in solution-layout order (for the SolutionDecoder).
        std::vector<ElementNote> getDecisionVariableNotes() const
        {
            std::vector<ElementNote> notes;
            notes.reserve(decision_variable_number);
            for (int k = 0; k < decision_variable_number; k++)
                notes.push_back(variables[decision_variable_index[k]].note);
            return notes;
        }
    };
}
