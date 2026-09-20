// SPDX-License-Identifier: BSD-3-Clause

#include "bindings.h"

#include <algorithm>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <pybind11/stl.h>

#include "app-facade.h"
#include "evoflow.h"
#include "python-callable.h"
#include "python-problem.h"
#include "python-run.h"

namespace py = pybind11;
using namespace EvoFlow;

namespace
{
    std::string require_name(const std::string& name, const char* kind)
    {
        if (name.empty()) throw py::value_error(std::string(kind) + " name cannot be empty");
        if (name.find(',') != std::string::npos)
            throw py::value_error(std::string(kind) + " name cannot contain ','");
        return name;
    }

    std::vector<std::string> normalize_names(const py::handle& value, const char* argument,
                                             bool allow_empty = false)
    {
        std::vector<std::string> names;
        if (value.is_none())
        {
            if (allow_empty) return names;
            throw py::type_error(std::string(argument) + " cannot be None");
        }
        if (py::isinstance<py::str>(value))
        {
            std::string one = py::cast<std::string>(value);
            if (one.empty() && allow_empty) return names;
            names.push_back(require_name(one, argument));
        }
        else
        {
            try
            {
                for (const py::handle item : py::reinterpret_borrow<py::iterable>(value))
                    names.push_back(require_name(py::cast<std::string>(item), argument));
            }
            catch (const py::cast_error&)
            {
                throw py::type_error(std::string(argument) + " must be a name or iterable of names");
            }
        }
        if (names.empty() && !allow_empty)
            throw py::value_error(std::string(argument) + " cannot be empty");
        return names;
    }

    std::string join_names(const std::vector<std::string>& names)
    {
        std::string result;
        for (std::size_t i = 0; i < names.size(); ++i)
        {
            if (i) result += ',';
            result += names[i];
        }
        return result;
    }

    std::vector<int> flatten_edges(const py::handle& value)
    {
        try
        {
            std::vector<int> flat = py::cast<std::vector<int>>(value);
            if (flat.size() % 2 != 0)
                throw py::value_error("edges must contain an even number of endpoints");
            return flat;
        }
        catch (const py::cast_error&)
        {
            std::vector<std::vector<int>> pairs;
            try { pairs = py::cast<std::vector<std::vector<int>>>(value); }
            catch (const py::cast_error&)
            {
                throw py::type_error("edges must be a flat endpoint list or a sequence of pairs");
            }
            std::vector<int> flat;
            flat.reserve(pairs.size() * 2);
            for (const auto& pair : pairs)
            {
                if (pair.size() != 2) throw py::value_error("every edge must contain two endpoints");
                flat.push_back(pair[0]);
                flat.push_back(pair[1]);
            }
            return flat;
        }
    }

    std::pair<std::vector<double>, std::pair<int, int>> flatten_matrix(
        const std::vector<std::vector<double>>& matrix, const char* argument)
    {
        if (matrix.empty() || matrix.front().empty())
            throw py::value_error(std::string(argument) + " cannot be empty");
        const int rows = static_cast<int>(matrix.size());
        const int cols = static_cast<int>(matrix.front().size());
        std::vector<double> flat;
        flat.reserve(static_cast<std::size_t>(rows) * cols);
        for (const auto& row : matrix)
        {
            if (static_cast<int>(row.size()) != cols)
                throw py::value_error(std::string(argument) + " must be rectangular");
            flat.insert(flat.end(), row.begin(), row.end());
        }
        return { std::move(flat), { rows, cols } };
    }

    App::ProblemDescription make_description(
        const ProblemDefinitionInfo& source,
        const std::string& source_type = "CUSTOM")
    {
        App::ProblemDescription result;
        result.ok = true;
        result.type = source_type;
        result.name = source.name;
        result.dimension = source.dimension;
        for (const ProblemVariableInfo& item : source.variables)
        {
            App::VariableInfo value;
            value.name = item.name;
            value.kind = item.kind;
            value.valueType = item.valueType;
            value.length = item.length;
            value.lowerBound = item.lowerBound;
            value.upperBound = item.upperBound;
            value.hasBounds = item.hasBounds;
            result.variables.push_back(std::move(value));
        }
        for (const ProblemObjectiveInfo& item : source.objectives)
        {
            App::ObjectiveInfo value;
            value.name = item.name;
            value.variables = item.variables;
            value.minimize = item.minimize;
            value.priority = item.priority;
            result.objectives.push_back(std::move(value));
        }
        for (const ProblemConstraintInfo& item : source.constraints)
        {
            App::ConstraintInfo value;
            value.type = item.type;
            value.variables = item.variables;
            value.penalty = item.penalty;
            value.objectives = item.objectives;
            result.constraints.push_back(std::move(value));
        }
        return result;
    }

    struct ProblemState
    {
        explicit ProblemState(const std::string& value)
            : name(value), problem(std::make_unique<Problem>(value)) {}

        ProblemState(std::unique_ptr<Problem> value,
                     const std::string& source_type,
                     const std::string& logical_name)
            : type(source_type.empty() ? "CUSTOM" : source_type),
              problem(std::move(value))
        {
            if (!problem) throw std::invalid_argument("problem cannot be null");
            const ProblemDefinitionInfo info = problem->describeDefinition();
            name = logical_name.empty() ? info.name : logical_name;
            if (name.empty()) name = "problem";
            if (!logical_name.empty()) problem->setName(logical_name);
            for (const ProblemVariableInfo& item : info.variables)
                variable_lengths[item.name] = item.length;
        }

        std::vector<std::shared_ptr<std::vector<double>>> retained_gap_vectors;
        std::unordered_map<std::string, int> variable_lengths;
        std::string name;
        std::string type = "CUSTOM";
        std::size_t generation = 0;
        std::unique_ptr<Problem> problem;
    };

    void require_variable(const ProblemState& state, const std::string& name)
    {
        if (state.variable_lengths.find(name) == state.variable_lengths.end())
            throw py::value_error("unknown variable: " + name);
    }

    std::vector<int> input_lengths(const ProblemState& state,
                                   const std::vector<std::string>& names)
    {
        std::vector<int> lengths;
        lengths.reserve(names.size());
        for (const std::string& name : names)
        {
            auto found = state.variable_lengths.find(name);
            if (found == state.variable_lengths.end())
                throw py::value_error("unknown callback input variable: " + name);
            lengths.push_back(found->second);
        }
        return lengths;
    }

    py::function require_callable(const py::object& value, const char* argument)
    {
        if (!PyCallable_Check(value.ptr()))
            throw py::type_error(std::string(argument) + " must be callable");
        return py::reinterpret_borrow<py::function>(value);
    }

    class PyAccumulateConstraint
    {
    public:
        PyAccumulateConstraint(std::shared_ptr<ProblemState> state,
                               ConstrainAccumulate* value)
            : _state(std::move(state)), _value(value), _generation(_state->generation) {}

        void add_capacity_track(std::vector<double> delta, double lower,
                                double upper, double initial)
        {
            checked()->addCapacityTrack(std::move(delta), lower, upper, initial);
        }

        void add_time_track(int node_count,
                            std::vector<std::vector<int>> predecessors,
                            std::vector<std::vector<double>> execution,
                            std::vector<std::vector<double>> communication_data,
                            std::vector<std::vector<double>> communication_rate,
                            std::vector<double> deadlines)
        {
            CommModel communication{ std::move(communication_data), std::move(communication_rate) };
            checked()->addTimeTrack(node_count, std::move(predecessors), std::move(execution),
                                    std::move(communication), std::move(deadlines));
        }

    private:
        ConstrainAccumulate* checked() const
        {
            if (_generation != _state->generation || !_value)
                throw std::runtime_error("accumulate constraint builder is invalid after Problem.clear()");
            return _value;
        }

        std::shared_ptr<ProblemState> _state;
        ConstrainAccumulate* _value;
        std::size_t _generation;
    };

    class PyCompiledProblem
    {
    public:
        PyCompiledProblem(std::unique_ptr<ProblemHandle> handle,
                          App::ProblemDescription description)
            : _handle(std::move(handle)), _description(std::move(description))
        {
            for (std::size_t i = 0; i < _description.variables.size(); ++i)
                _variable_ids[_description.variables[i].name] = static_cast<int>(i);
            _construction.setDecoder(_handle->getSolutionDecoder());
            reset_construction(py::none());
        }

        std::vector<double> evaluate(const std::vector<double>& values)
        {
            Solution solution = make_solution(values);
            std::exception_ptr failure;
            {
                py::gil_scoped_release release;
                try { _handle->solutionEvaluate(solution); }
                catch (...) { failure = std::current_exception(); }
            }
            _handle->setResult(_construction);
            if (failure) std::rethrow_exception(failure);
            return std::vector<double>(solution.fitness,
                                       solution.fitness + _handle->getObjectNumber());
        }

        bool is_better(const std::vector<double>& first,
                       const std::vector<double>& second)
        {
            require_objectives(first);
            require_objectives(second);
            return _handle->getSolutionComparer()->isBetter(first.data(), second.data());
        }

        std::string format_solution(const std::vector<double>& values,
                                    const std::vector<double>& objectives,
                                    bool full)
        {
            require_objectives(objectives);
            Solution solution = make_solution(values);
            std::copy(objectives.begin(), objectives.end(), solution.fitness);
            return _handle->getSolutionDecoder()->toString(
                solution.result, solution.fitness, full);
        }

        double constraint_violation(const std::vector<double>& values)
        {
            Solution solution = make_solution(values);
            double result;
            try { result = _handle->constraintViolation(solution); }
            catch (...) { _handle->setResult(_construction); throw; }
            _handle->setResult(_construction);
            return result;
        }

        bool is_legal(const std::vector<double>& values)
        {
            Solution solution = make_solution(values);
            bool result;
            try { result = _handle->solutionLegality(solution); }
            catch (...) { _handle->setResult(_construction); throw; }
            _handle->setResult(_construction);
            return result;
        }

        void set_environment(const py::object& variable, const std::vector<double>& values)
        {
            int id = variable_id(variable);
            const int length = _handle->getVariableLength(id);
            if (static_cast<int>(values.size()) != length)
                throw py::value_error("environment value count does not match variable length");
            _handle->changeEnv(values.data(), id);
        }

        void reset_construction(const py::object& initial)
        {
            std::vector<double> values;
            if (initial.is_none()) values.assign(dimension(), EMPTYVALUE);
            else values = py::cast<std::vector<double>>(initial);
            require_values(values);
            std::copy(values.begin(), values.end(), _construction.result);
            _handle->setResult(_construction);
            _handle->constrainReset();
        }

        std::vector<double> construction_values()
        {
            return std::vector<double>(_construction.result,
                                       _construction.result + _construction.getSolutionSize());
        }

        bool check_choice(int dimension_id, double value)
        {
            require_dimension(dimension_id);
            return _handle->constrainCheck(dimension_id, value);
        }

        void commit_choice(int dimension_id, double value)
        {
            require_dimension(dimension_id);
            _construction[dimension_id] = value;
            _handle->constrainChange(dimension_id, value);
        }

        std::vector<double> feasible_choices(int dimension_id)
        {
            require_dimension(dimension_id);
            double* raw = nullptr;
            int size = 0;
            _handle->getFeasibleList(dimension_id, raw, size);
            std::unique_ptr<double[]> values(raw);
            if (size <= 0) return {};
            return std::vector<double>(values.get(), values.get() + size);
        }

        std::vector<double> priori_choices(int dimension_id, int size)
        {
            require_dimension(dimension_id);
            if (size < 0) throw py::value_error("size must be non-negative");
            std::vector<double> result(static_cast<std::size_t>(size));
            if (size)
            {
                py::gil_scoped_release release;
                _handle->getPrioriChoice(dimension_id, size, result.data());
            }
            return result;
        }

        double choice_heuristic(int dimension_id, double choice)
        {
            require_dimension(dimension_id);
            py::gil_scoped_release release;
            return _handle->getChoiceHeuristic(dimension_id, choice);
        }

        double priori_choice(int dimension_id)
        {
            require_dimension(dimension_id);
            py::gil_scoped_release release;
            return _handle->getPrioriChoice(dimension_id);
        }

        std::vector<double> greedy_solution()
        {
            Solution result;
            result.setDecoder(_handle->getSolutionDecoder());
            std::fill(result.result, result.result + dimension(), EMPTYVALUE);
            std::exception_ptr failure;
            {
                py::gil_scoped_release release;
                try { _handle->getGreedyResult(result); }
                catch (...) { failure = std::current_exception(); }
            }
            _handle->setResult(_construction);
            if (failure) std::rethrow_exception(failure);
            return std::vector<double>(result.result, result.result + dimension());
        }

        int variable_id(const py::object& variable) const
        {
            int id;
            if (py::isinstance<py::str>(variable))
            {
                const std::string name = py::cast<std::string>(variable);
                auto found = _variable_ids.find(name);
                if (found == _variable_ids.end()) throw py::value_error("unknown variable: " + name);
                id = found->second;
            }
            else id = py::cast<int>(variable);
            if (id < 0 || id >= variable_count()) throw py::index_error("variable index out of range");
            return id;
        }

        py::tuple dimension_variable(int dimension_id)
        {
            require_dimension(dimension_id);
            return py::make_tuple(_handle->getBelongVariableId(dimension_id),
                                  _handle->getWithinVariableId(dimension_id));
        }

        int variable_offset(const py::object& variable)
        {
            int id = variable_id(variable);
            if (_description.variables[static_cast<std::size_t>(id)].kind != "decision")
                throw py::value_error("constant variables have no decision-vector offset");
            return _handle->getVariableOffset(id);
        }

        int variable_length(const py::object& variable) { return _handle->getVariableLength(variable_id(variable)); }
        VariableType variable_type(const py::object& variable)
        {
            int id = variable_id(variable);
            if (_description.variables[static_cast<std::size_t>(id)].kind != "decision")
                throw py::value_error("constant variables do not have a VariableType");
            return _handle->getVariableType(id);
        }

        py::tuple dimension_bounds(int dimension_id)
        {
            require_dimension(dimension_id);
            return py::make_tuple(_handle->getVariableLowbound(dimension_id),
                                  _handle->getVariableUpbound(dimension_id));
        }

        int objective_priority(const py::object& objective) const
        {
            return _handle->getObjectivePriority(objective_id(objective));
        }

        bool change_priority(const py::object& objective, int priority)
        {
            if (py::isinstance<py::str>(objective))
                return _handle->changePriority(py::cast<std::string>(objective), priority);
            return _handle->changePriority(py::cast<int>(objective), priority);
        }

        int objective_id(const py::object& objective) const
        {
            int id = py::isinstance<py::str>(objective)
                ? _handle->getObjectiveId(py::cast<std::string>(objective))
                : py::cast<int>(objective);
            if (id < 0 || id >= objective_count()) throw py::value_error("unknown objective");
            return id;
        }

        int dimension() const { return _handle->getProblemSize(); }
        int objective_count() const { return _handle->getObjectNumber(); }
        int variable_count() const { return _handle->getVariableNumber(); }
        std::string name() const { return _handle->getName(); }
        const App::ProblemDescription& description() const { return _description; }
        bool full_evaluation() const { return _handle->getEvaluateMode(); }
        void set_full_evaluation(bool value) { _handle->setEvaluateMode(value); }
        std::unique_ptr<ProblemHandle> clone_handle() const
        {
            return std::make_unique<ProblemHandle>(*_handle);
        }

        int choice_count(int dimension_id)
        {
            require_dimension(dimension_id);
            return _handle->getChoiceNumber(dimension_id);
        }
        double close_choice(int dimension_id, double value)
        {
            require_dimension(dimension_id); return _handle->getCloseChoice(dimension_id, value);
        }
        double boundary_choice(int dimension_id, bool left)
        {
            require_dimension(dimension_id); return _handle->getBoundaryChoice(dimension_id, left);
        }
        double random_choice(int dimension_id)
        {
            require_dimension(dimension_id); return _handle->getRandomChoice(dimension_id);
        }
        double random_choice_inspace(int dimension_id)
        {
            require_dimension(dimension_id); return _handle->getRandomChoiceInspace(dimension_id);
        }
        double snap_choice(int dimension_id, double value)
        {
            require_dimension(dimension_id); return _handle->choiceDiscretized(dimension_id, value);
        }

    private:
        void require_dimension(int id) const
        {
            if (id < 0 || id >= dimension()) throw py::index_error("dimension index out of range");
        }

        void require_values(const std::vector<double>& values) const
        {
            if (static_cast<int>(values.size()) != dimension())
                throw py::value_error("value count does not match problem dimension");
        }

        void require_objectives(const std::vector<double>& objectives) const
        {
            if (static_cast<int>(objectives.size()) != objective_count())
                throw py::value_error("objective count does not match problem objective count");
        }

        Solution make_solution(const std::vector<double>& values) const
        {
            require_values(values);
            Solution result;
            result.setDecoder(_handle->getSolutionDecoder());
            std::copy(values.begin(), values.end(), result.result);
            return result;
        }

        std::unique_ptr<ProblemHandle> _handle;
        App::ProblemDescription _description;
        std::unordered_map<std::string, int> _variable_ids;
        Solution _construction;
    };

    class PyProblem
    {
    public:
        explicit PyProblem(const std::string& name)
            : _state(std::make_shared<ProblemState>(require_name(name, "problem"))) {}

        PyProblem(std::unique_ptr<Problem> problem,
                  const std::string& source_type,
                  const std::string& logical_name)
            : _state(std::make_shared<ProblemState>(
                  std::move(problem), source_type,
                  logical_name.empty()
                      ? std::string()
                      : require_name(logical_name, "problem"))) {}

        std::string name() const { return _state->name; }
        void set_name(const std::string& value)
        {
            _state->name = require_name(value, "problem");
            _state->problem->setName(_state->name);
        }

        int add_constant(const std::string& name, std::vector<double> values, int height)
        {
            require_name(name, "constant");
            if (height <= 0 || values.empty() || values.size() % static_cast<std::size_t>(height) != 0)
                throw py::value_error("constant values must be non-empty and divisible by height");
            int length = static_cast<int>(values.size() / static_cast<std::size_t>(height));
            int id = _state->problem->addConstant(name, values.data(), length, height);
            _state->variable_lengths[name] = static_cast<int>(values.size());
            return id;
        }

        int add_variable(const std::string& name, double lower, double upper,
                         double accuracy, int length, int height, VariableType type)
        {
            require_name(name, "variable");
            if (length <= 0 || height <= 0) throw py::value_error("length and height must be positive");
            if (upper < lower) throw py::value_error("upper bound must be >= lower bound");
            if (accuracy <= 0) throw py::value_error("accuracy must be positive");
            int id = _state->problem->addVariable(name, lower, upper, accuracy, length, height, type);
            _state->variable_lengths[name] = length * height;
            return id;
        }

        void add_objective(const std::string& name, const py::object& inputs,
                           const py::object& callback, int priority, bool minimize)
        {
            require_name(name, "objective");
            auto names = normalize_names(inputs, "inputs");
            auto state = std::make_shared<EvoFlowPython::PythonCallableState>(
                require_callable(callback, "callback"), input_lengths(*_state, names),
                "objective", name);
            EvoFlowPython::PythonCalculationFunctor functor(state);
            _state->problem->addObjective(name, priority, minimize, join_names(names), &functor);
        }

        void add_objective_formula(const std::string& name, const py::object& inputs,
                                   const std::string& formula, int priority, bool minimize)
        {
            require_name(name, "objective");
            auto names = normalize_names(inputs, "inputs");
            input_lengths(*_state, names);
            _state->problem->addObjective(name, priority, minimize, join_names(names), formula);
        }

        void add_range_constraint(const std::string& variable, const py::object& lower,
                                  const py::object& upper, double penalty, const py::object& objectives)
        {
            require_variable(*_state, variable);
            const double left = lower.is_none() ? EMPTYVALUE : py::cast<double>(lower);
            const double right = upper.is_none() ? EMPTYVALUE : py::cast<double>(upper);
            _state->problem->addConstrainRange(variable, left, right, penalty,
                                              objective_names(objectives));
        }

        void add_compatibility_constraint(const std::string& variable,
                                          std::vector<double> values, double penalty,
                                          const py::object& objectives)
        {
            require_variable(*_state, variable);
            if (values.empty()) throw py::value_error("values cannot be empty");
            _state->problem->addConstrainCompatibility(variable, values.data(),
                static_cast<int>(values.size()), penalty, objective_names(objectives));
        }

        void add_eligible_constraint(const std::string& variable,
                                     std::vector<std::vector<double>> allowed,
                                     double penalty, const py::object& objectives)
        {
            require_variable(*_state, variable);
            if (static_cast<int>(allowed.size()) != _state->variable_lengths.at(variable))
                throw py::value_error("allowed must contain one value list per variable element");
            _state->problem->addConstrainEligible(variable, std::move(allowed), penalty,
                                                 objective_names(objectives));
        }

        void add_unique_constraint(const std::string& variable, double penalty,
                                   const py::object& objectives)
        {
            require_variable(*_state, variable);
            _state->problem->addConstrainUnique(variable, penalty, objective_names(objectives));
        }

        void add_distinct_cap_constraint(const std::string& variable, int maximum,
                                         double penalty, const py::object& objectives)
        {
            require_variable(*_state, variable);
            if (maximum < 0) throw py::value_error("maximum must be non-negative");
            _state->problem->addConstrainDistinctCap(variable, maximum, penalty,
                                                    objective_names(objectives));
        }

        void add_min_distance_constraint(const std::string& variable, const py::object& gap,
                                         double penalty, const py::object& objectives)
        {
            require_variable(*_state, variable);
            if (py::isinstance<py::float_>(gap) || py::isinstance<py::int_>(gap))
            {
                _state->problem->addConstrainMinDistance(variable, py::cast<double>(gap), penalty,
                                                        objective_names(objectives));
                return;
            }
            auto values = std::make_shared<std::vector<double>>(py::cast<std::vector<double>>(gap));
            if (static_cast<int>(values->size()) != _state->variable_lengths.at(variable))
                throw py::value_error("gap sequence length must match variable length");
            _state->retained_gap_vectors.push_back(values);
            _state->problem->addConstrainMinDistance(variable, values->data(), penalty,
                                                    objective_names(objectives));
        }

        void add_capacity_constraint(const std::string& variable, std::vector<double> capacities,
                                     std::vector<double> volumes, double penalty,
                                     const py::object& objectives)
        {
            require_variable(*_state, variable);
            if (capacities.empty() || volumes.empty())
                throw py::value_error("capacities and volumes cannot be empty");
            _state->problem->addConstrainCapacity(variable, capacities.data(),
                static_cast<int>(capacities.size()), volumes.data(), static_cast<int>(volumes.size()),
                penalty, objective_names(objectives));
        }

        void add_distributed_constraint(const std::string& variable, std::vector<double> values,
                                        std::vector<int> counts, double penalty,
                                        const py::object& objectives)
        {
            require_variable(*_state, variable);
            if (values.empty() || values.size() != counts.size())
                throw py::value_error("values and counts must be non-empty and have equal length");
            _state->problem->addConstrainDistributed(variable, values.data(),
                static_cast<int>(values.size()), counts.data(), penalty, objective_names(objectives));
        }

        void add_expression_constraint(const std::string& variable, const py::object& inputs,
                                       const std::string& formula, double penalty,
                                       const py::object& objectives)
        {
            require_variable(*_state, variable);
            auto names = normalize_names(inputs, "inputs", true);
            for (const auto& name : names) require_variable(*_state, name);
            _state->problem->addConstrainExpr(variable, join_names(names), formula, penalty,
                                             objective_names(objectives));
        }

        void add_sequence_accumulate_constraint(const std::string& variable,
                                                std::vector<double> delta, double lower,
                                                double upper, double initial, double penalty,
                                                const py::object& objectives)
        {
            require_variable(*_state, variable);
            if (delta.empty()) throw py::value_error("delta cannot be empty");
            _state->problem->addConstrainSequenceAccumulate(variable, std::move(delta), lower,
                upper, initial, penalty, objective_names(objectives));
        }

        void add_schedule_accumulate_constraint(const std::string& variable, int node_count,
                                                std::vector<std::vector<int>> predecessors,
                                                std::vector<std::vector<double>> execution,
                                                std::vector<std::vector<double>> communication_data,
                                                std::vector<std::vector<double>> communication_rate,
                                                std::vector<double> deadlines, double penalty,
                                                const py::object& objectives)
        {
            require_variable(*_state, variable);
            CommModel communication{ std::move(communication_data), std::move(communication_rate) };
            _state->problem->addConstrainScheduleAccumulate(variable, node_count,
                std::move(predecessors), std::move(execution), std::move(communication),
                std::move(deadlines), penalty, objective_names(objectives));
        }

        std::shared_ptr<PyAccumulateConstraint> add_accumulate_constraint(
            const std::string& variable, double penalty, const py::object& objectives)
        {
            require_variable(*_state, variable);
            ConstrainAccumulate* raw = _state->problem->addConstrainAccumulate(
                variable, penalty, objective_names(objectives));
            return std::make_shared<PyAccumulateConstraint>(_state, raw);
        }

        void add_graph_constraint(const std::string& kind, const std::string& variable,
                                  const py::object& edges, double penalty,
                                  const py::object& objectives)
        {
            require_variable(*_state, variable);
            std::vector<int> flat = flatten_edges(edges);
            const std::string objs = objective_names(objectives);
            if (kind == "independent") _state->problem->addConstrainGraphIndependent(variable, flat, penalty, objs);
            else if (kind == "distinct") _state->problem->addConstrainGraphDistinct(variable, flat, penalty, objs);
            else if (kind == "clique") _state->problem->addConstrainGraphClique(variable, flat, penalty, objs);
            else if (kind == "dominating") _state->problem->addConstrainGraphDominating(variable, flat, penalty, objs);
            else if (kind == "vertex_cover") _state->problem->addConstrainGraphVertexCover(variable, flat, penalty, objs);
            else throw py::value_error("unknown safe graph constraint kind: " + kind);
        }

        void add_node_sum_constraint(const std::string& variable, int vertices,
                                     const py::object& edges, bool directed,
                                     std::vector<double> lower, std::vector<double> upper,
                                     std::vector<double> capacity, double penalty,
                                     const py::object& objectives)
        {
            require_variable(*_state, variable);
            _state->problem->addConstrainNodeSum(variable, vertices, flatten_edges(edges), directed,
                lower, upper, capacity, penalty, objective_names(objectives));
        }

        void add_connectivity_constraint(const std::string& variable, int vertices,
                                         const py::object& edges, std::vector<int> terminals,
                                         double penalty, const py::object& objectives)
        {
            require_variable(*_state, variable);
            _state->problem->addConstrainGraphConnectivity(variable, vertices, flatten_edges(edges),
                terminals, penalty, objective_names(objectives));
        }

        void add_random_heuristic(const std::string& variable)
        {
            require_variable(*_state, variable); _state->problem->addInspirationRandom(variable);
        }
        void add_boundary_heuristic(const std::string& variable)
        {
            require_variable(*_state, variable); _state->problem->addInspirationBoundary(variable);
        }

        void add_callable_heuristic(const std::string& variable, const py::object& inputs,
                                    const py::object& callback, bool stable)
        {
            require_variable(*_state, variable);
            auto names = normalize_names(inputs, "inputs", true);
            auto state = std::make_shared<EvoFlowPython::PythonCallableState>(
                require_callable(callback, "callback"), input_lengths(*_state, names),
                "heuristic", variable, 2);
            EvoFlowPython::PythonCalculationFunctor functor(state);
            _state->problem->addInspirationFunc(variable, join_names(names), &functor, stable);
        }

        void add_matrix_heuristic(const std::string& variable,
                                  const std::vector<std::vector<double>>& matrix)
        {
            require_variable(*_state, variable);
            auto flattened = flatten_matrix(matrix, "matrix");
            const int rows = flattened.second.first;
            const int columns = flattened.second.second;
            if (rows != _state->variable_lengths.at(variable))
                throw py::value_error("matrix must contain one row per target variable element");
            _state->problem->addInspirationMatrix(variable, flattened.first.data(), columns, rows);
        }

        void add_formula_heuristic(const std::string& variable, const py::object& inputs,
                                   const std::string& formula, bool stable)
        {
            require_variable(*_state, variable);
            auto names = normalize_names(inputs, "inputs", true);
            input_lengths(*_state, names);
            _state->problem->addInspirationExpr(variable, join_names(names), formula, stable);
        }

        App::ProblemDescription describe() const
        {
            return make_description(_state->problem->describeDefinition(), _state->type);
        }

        std::shared_ptr<PyCompiledProblem> compile() const
        {
            App::ProblemDescription snapshot = describe();
            std::unique_ptr<ProblemHandle> handle(_state->problem->compile());
            if (!handle)
                throw py::value_error("problem definition is incomplete; check variable references");
            return std::make_shared<PyCompiledProblem>(std::move(handle), std::move(snapshot));
        }

        void clear()
        {
            _state->problem->clear();
            _state->retained_gap_vectors.clear();
            _state->variable_lengths.clear();
            _state->type = "CUSTOM";
            ++_state->generation;
            _compile_check = true;
            _constraint_check = true;
        }

        bool compile_check() const { return _compile_check; }
        void set_compile_check(bool value)
        {
            _compile_check = value; _state->problem->setCompileCheck(value);
        }
        bool full_evaluation() const { return _state->problem->getEvaluateMode(); }
        void set_full_evaluation(bool value) { _state->problem->setEvaluateMode(value); }
        bool constraint_check() const { return _constraint_check; }
        void set_constraint_check(bool value)
        {
            _constraint_check = value;
            _state->problem->changeConstrainDeal(!value);
        }

    private:
        std::string objective_names(const py::object& objectives) const
        {
            auto names = normalize_names(objectives, "objectives", true);
            return join_names(names);
        }

        std::shared_ptr<ProblemState> _state;
        bool _compile_check = true;
        bool _constraint_check = true;
    };
}

namespace EvoFlow
{
    namespace Python
    {
        std::unique_ptr<ProblemHandle> cloneCompiledProblem(const py::handle& value)
        {
            try
            {
                const std::shared_ptr<PyCompiledProblem> problem =
                    py::cast<std::shared_ptr<PyCompiledProblem>>(value);
                if (!problem) throw py::type_error("problem must be a CompiledProblem");
                return problem->clone_handle();
            }
            catch (const py::cast_error&)
            {
                throw py::type_error("problem must be a CompiledProblem");
            }
        }
    }
}

namespace EvoFlowPython
{
    py::object adoptProblem(
        std::unique_ptr<Problem> problem,
        const std::string& sourceType,
        const std::string& logicalName)
    {
        if (!problem) throw py::value_error("native problem construction returned no problem");
        return py::cast(std::make_shared<PyProblem>(
            std::move(problem), sourceType, logicalName));
    }
}

void bind_problem(py::module_& module)
{
    py::enum_<VariableType>(module, "VariableType")
        .value("DISCRETE", VariableType::discrete)
        .value("ALLOCATION", VariableType::allocation)
        .value("SEQUENCE_DIRECTION", VariableType::sequence_direction)
        .value("SEQUENCE_BIDIAGRAPH", VariableType::sequence_bidiagraph)
        .value("SUB_MODULE", VariableType::sub_module)
        .value("CONTINUOUS", VariableType::continuous);

    py::class_<PyAccumulateConstraint, std::shared_ptr<PyAccumulateConstraint>>(
        module, "AccumulateConstraint")
        .def("add_capacity_track", &PyAccumulateConstraint::add_capacity_track,
             py::arg("delta"), py::arg("lower"), py::arg("upper"), py::arg("initial") = 0.0)
        .def("add_time_track", &PyAccumulateConstraint::add_time_track,
             py::arg("node_count"), py::arg("predecessors"), py::arg("execution"),
             py::arg("communication_data"), py::arg("communication_rate"), py::arg("deadlines"));

    py::class_<PyCompiledProblem, std::shared_ptr<PyCompiledProblem>>(module, "CompiledProblem")
        .def_property_readonly("name", &PyCompiledProblem::name)
        .def_property_readonly("dimension", &PyCompiledProblem::dimension)
        .def_property_readonly("objective_count", &PyCompiledProblem::objective_count)
        .def_property_readonly("variable_count", &PyCompiledProblem::variable_count)
        .def_property_readonly("description", &PyCompiledProblem::description,
                               py::return_value_policy::reference_internal)
        .def_property("full_evaluation", &PyCompiledProblem::full_evaluation,
                      &PyCompiledProblem::set_full_evaluation)
        .def("evaluate", &PyCompiledProblem::evaluate, py::arg("values"))
        .def("is_better", &PyCompiledProblem::is_better,
             py::arg("first"), py::arg("second"))
        .def("format_solution", &PyCompiledProblem::format_solution,
             py::arg("values"), py::arg("objectives"), py::arg("full") = false)
        .def("constraint_violation", &PyCompiledProblem::constraint_violation, py::arg("values"))
        .def("is_legal", &PyCompiledProblem::is_legal, py::arg("values"))
        .def("objective_id", &PyCompiledProblem::objective_id, py::arg("objective"))
        .def("objective_priority", &PyCompiledProblem::objective_priority, py::arg("objective"))
        .def("change_priority", &PyCompiledProblem::change_priority,
             py::arg("objective"), py::arg("priority"))
        .def("variable_id", &PyCompiledProblem::variable_id, py::arg("variable"))
        .def("dimension_variable", &PyCompiledProblem::dimension_variable, py::arg("dimension"))
        .def("variable_offset", &PyCompiledProblem::variable_offset, py::arg("variable"))
        .def("variable_length", &PyCompiledProblem::variable_length, py::arg("variable"))
        .def("variable_type", &PyCompiledProblem::variable_type, py::arg("variable"))
        .def("dimension_bounds", &PyCompiledProblem::dimension_bounds, py::arg("dimension"))
        .def("set_environment", &PyCompiledProblem::set_environment,
             py::arg("variable"), py::arg("values"))
        .def("reset_construction", &PyCompiledProblem::reset_construction,
             py::arg("initial") = py::none())
        .def_property_readonly("construction_values", &PyCompiledProblem::construction_values)
        .def("check_choice", &PyCompiledProblem::check_choice,
             py::arg("dimension"), py::arg("value"))
        .def("commit_choice", &PyCompiledProblem::commit_choice,
             py::arg("dimension"), py::arg("value"))
        .def("choice_count", &PyCompiledProblem::choice_count, py::arg("dimension"))
        .def("feasible_choices", &PyCompiledProblem::feasible_choices, py::arg("dimension"))
        .def("close_choice", &PyCompiledProblem::close_choice,
             py::arg("dimension"), py::arg("value"))
        .def("boundary_choice", &PyCompiledProblem::boundary_choice,
             py::arg("dimension"), py::arg("left") = true)
        .def("random_choice", &PyCompiledProblem::random_choice, py::arg("dimension"))
        .def("random_choice_inspace", &PyCompiledProblem::random_choice_inspace, py::arg("dimension"))
        .def("snap_choice", &PyCompiledProblem::snap_choice,
             py::arg("dimension"), py::arg("value"))
        .def("choice_heuristic", &PyCompiledProblem::choice_heuristic,
             py::arg("dimension"), py::arg("choice"))
        .def("priori_choice", &PyCompiledProblem::priori_choice, py::arg("dimension"))
        .def("priori_choices", &PyCompiledProblem::priori_choices,
             py::arg("dimension"), py::arg("size"))
        .def("greedy_solution", &PyCompiledProblem::greedy_solution);

    py::class_<PyProblem, std::shared_ptr<PyProblem>>(module, "Problem")
        .def(py::init<const std::string&>(), py::arg("name"))
        .def_property("name", &PyProblem::name, &PyProblem::set_name)
        .def_property("compile_check", &PyProblem::compile_check, &PyProblem::set_compile_check)
        .def_property("constraint_check", &PyProblem::constraint_check, &PyProblem::set_constraint_check)
        .def_property("full_evaluation", &PyProblem::full_evaluation, &PyProblem::set_full_evaluation)
        .def("clear", &PyProblem::clear)
        .def("describe", &PyProblem::describe)
        .def("compile", &PyProblem::compile)
        .def("add_constant", &PyProblem::add_constant,
             py::arg("name"), py::arg("values"), py::arg("height") = 1)
        .def("add_variable", &PyProblem::add_variable,
             py::arg("name"), py::arg("lower"), py::arg("upper"), py::arg("accuracy"),
             py::arg("length"), py::arg("height") = 1,
             py::arg("type") = VariableType::discrete)
        .def("add_objective", &PyProblem::add_objective,
             py::arg("name"), py::arg("inputs"), py::arg("callback"),
             py::arg("priority") = 0, py::arg("minimize") = true)
        .def("add_objective_formula", &PyProblem::add_objective_formula,
             py::arg("name"), py::arg("inputs"), py::arg("formula"),
             py::arg("priority") = 0, py::arg("minimize") = true)
        .def("add_range_constraint", &PyProblem::add_range_constraint,
             py::arg("variable"), py::arg("lower") = py::none(), py::arg("upper") = py::none(),
             py::arg("penalty") = 1.0, py::arg("objectives") = py::none())
        .def("add_compatibility_constraint", &PyProblem::add_compatibility_constraint,
             py::arg("variable"), py::arg("values"), py::arg("penalty") = 1.0,
             py::arg("objectives") = py::none())
        .def("add_eligible_constraint", &PyProblem::add_eligible_constraint,
             py::arg("variable"), py::arg("allowed"), py::arg("penalty") = 1.0,
             py::arg("objectives") = py::none())
        .def("add_unique_constraint", &PyProblem::add_unique_constraint,
             py::arg("variable"), py::arg("penalty") = 1.0,
             py::arg("objectives") = py::none())
        .def("add_distinct_cap_constraint", &PyProblem::add_distinct_cap_constraint,
             py::arg("variable"), py::arg("maximum"), py::arg("penalty") = 1.0,
             py::arg("objectives") = py::none())
        .def("add_min_distance_constraint", &PyProblem::add_min_distance_constraint,
             py::arg("variable"), py::arg("gap"), py::arg("penalty") = 1.0,
             py::arg("objectives") = py::none())
        .def("add_capacity_constraint", &PyProblem::add_capacity_constraint,
             py::arg("variable"), py::arg("capacities"), py::arg("volumes"),
             py::arg("penalty") = 1.0, py::arg("objectives") = py::none())
        .def("add_distributed_constraint", &PyProblem::add_distributed_constraint,
             py::arg("variable"), py::arg("values"), py::arg("counts"),
             py::arg("penalty") = 1.0, py::arg("objectives") = py::none())
        .def("add_expression_constraint", &PyProblem::add_expression_constraint,
             py::arg("variable"), py::arg("inputs"), py::arg("formula"),
             py::arg("penalty") = 1.0, py::arg("objectives") = py::none())
        .def("add_sequence_accumulate_constraint", &PyProblem::add_sequence_accumulate_constraint,
             py::arg("variable"), py::arg("delta"), py::arg("lower"), py::arg("upper"),
             py::arg("initial") = 0.0, py::arg("penalty") = 1.0,
             py::arg("objectives") = py::none())
        .def("add_schedule_accumulate_constraint", &PyProblem::add_schedule_accumulate_constraint,
             py::arg("variable"), py::arg("node_count"), py::arg("predecessors"),
             py::arg("execution"), py::arg("communication_data"),
             py::arg("communication_rate"), py::arg("deadlines"),
             py::arg("penalty") = 1.0, py::arg("objectives") = py::none())
        .def("add_accumulate_constraint", &PyProblem::add_accumulate_constraint,
             py::arg("variable"), py::arg("penalty") = 1.0,
             py::arg("objectives") = py::none())
        .def("add_graph_constraint", &PyProblem::add_graph_constraint,
             py::arg("kind"), py::arg("variable"), py::arg("edges"),
             py::arg("penalty") = 1.0, py::arg("objectives") = py::none())
        .def("add_node_sum_constraint", &PyProblem::add_node_sum_constraint,
             py::arg("variable"), py::arg("vertices"), py::arg("edges"),
             py::arg("directed"), py::arg("lower"), py::arg("upper"),
             py::arg("capacity"), py::arg("penalty") = 1.0,
             py::arg("objectives") = py::none())
        .def("add_connectivity_constraint", &PyProblem::add_connectivity_constraint,
             py::arg("variable"), py::arg("vertices"), py::arg("edges"),
             py::arg("terminals") = std::vector<int>(), py::arg("penalty") = 1.0,
             py::arg("objectives") = py::none())
        .def("add_random_heuristic", &PyProblem::add_random_heuristic, py::arg("variable"))
        .def("add_boundary_heuristic", &PyProblem::add_boundary_heuristic, py::arg("variable"))
        .def("add_callable_heuristic", &PyProblem::add_callable_heuristic,
             py::arg("variable"), py::arg("inputs"), py::arg("callback"),
             py::arg("stable") = false)
        .def("add_matrix_heuristic", &PyProblem::add_matrix_heuristic,
             py::arg("variable"), py::arg("matrix"))
        .def("add_formula_heuristic", &PyProblem::add_formula_heuristic,
             py::arg("variable"), py::arg("inputs"), py::arg("formula"),
             py::arg("stable") = false);
}
