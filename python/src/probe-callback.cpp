// SPDX-License-Identifier: BSD-3-Clause

#include "bindings.h"

#include <atomic>
#include <cmath>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>

#include <pybind11/stl.h>

#include "evoflow.h"

namespace py = pybind11;
using namespace EvoFlow;

namespace
{
    struct PythonObjectiveState
    {
        explicit PythonObjectiveState(py::function source)
            : callback(std::move(source)) {}

        py::function callback;
        std::atomic<int> calls{0};
    };

    class PythonObjectiveFunctor : public eccalcul_functor
    {
        std::shared_ptr<PythonObjectiveState> _state;
        int _size;

    public:
        PythonObjectiveFunctor(
            std::shared_ptr<PythonObjectiveState> state, int size)
            : _state(std::move(state)), _size(size) {}

        double operator()(double** input) const override
        {
            double result = 0.0;
            std::string error;
            {
                py::gil_scoped_acquire acquire;
                try
                {
                    py::list values;
                    for (int i = 0; i < _size; ++i)
                        values.append(input[0][i]);
                    result = py::cast<double>(_state->callback(values));
                    _state->calls.fetch_add(1, std::memory_order_relaxed);
                }
                catch (py::error_already_set& source)
                {
                    error = source.what();
                    source.restore();
                    PyErr_Clear();
                }
            }
            if (!error.empty())
                throw std::runtime_error(
                    "Python objective callback failed: " + error);
            return result;
        }

        eccalcul_functor* copy() override
        {
            return new PythonObjectiveFunctor(_state, _size);
        }
    };

    py::dict probe_python_objective(const py::object& callback)
    {
        if (!PyCallable_Check(callback.ptr()))
            throw py::type_error("callback must be callable");

        const int dimension = 5;
        const int population = 8;
        const int global_fes = 800;
        const int sub_fes = 80;
        const int subpopulations = 4;

        auto state = std::make_shared<PythonObjectiveState>(
            py::reinterpret_borrow<py::function>(callback));
        Problem problem("python_callback_probe");
        problem.addVariable("x", 0.0, 9.0, 0.001, dimension);
        PythonObjectiveFunctor objective(state, dimension);
        problem.addObjective("f", 1, true, "x", &objective);

        ConfigBuilder builder;
        auto workflow = builder.workflow("probe_workflow");
        workflow.initializer("Random");
        workflow.topology.pgBest();
        workflow.repair.boundary();
        workflow.strategy.velocityDriven(
            2, 2.0, 0.6, 0.1);
        workflow.generator.orderedConstruct();
        workflow.evaluator.basic();
        workflow.selector.index(false);

        builder.name("python_callback_probe")
            .tag("python_callback_probe")
            .maxFES(global_fes)
            .manager("NoInteraction")
            .cooperationConstructer("Fixed")
            .cooperationTopology("Connected")
            .gArchive("Basic")
            .logging(false, false, false);
        for (int i = 0; i < subpopulations; ++i)
        {
            builder.subpopulation("s" + std::to_string(i))
                .size(population)
                .workflow("probe_workflow")
                .maxFES(sub_fes)
                .archive("Basic");
        }

        std::unique_ptr<Optimizer> optimizer(builder.buildOptimizer());
        optimizer->setProblem(&problem);

        std::exception_ptr failure;
        {
            py::gil_scoped_release release;
            try
            {
                optimizer->exe(42);
            }
            catch (...)
            {
                failure = std::current_exception();
            }
        }
        if (failure) std::rethrow_exception(failure);

        Solution* best = nullptr;
        int best_size = 0;
        optimizer->getBest(best, best_size);
        if (!best || best_size <= 0)
            throw std::runtime_error("callback probe produced no solution");

        py::dict result;
        result["best_fitness"] = best[0].fitness[0];
        result["calls"] = state->calls.load(std::memory_order_relaxed);
#ifdef EVOFLOW_PARALLEL_SUBPOP
        result["parallel_build"] = true;
#else
        result["parallel_build"] = false;
#endif
        return result;
    }
}

void bind_callback_probe(py::module_& module)
{
    module.def("_probe_python_objective", &probe_python_objective,
               py::arg("callback"));
}
