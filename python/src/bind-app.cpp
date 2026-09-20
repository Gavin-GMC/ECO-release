// SPDX-License-Identifier: BSD-3-Clause

#include "bindings.h"

#include <memory>
#include <stdexcept>
#include <string>

#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include "app-facade.h"
#include "python-run.h"

namespace py = pybind11;
using namespace EvoFlow;

namespace
{
    App::RunResult run_with_callbacks(
        const App::RunOptions& options,
        const py::object& progress,
        const py::object& cancelled)
    {
        Python::RunCallbacks callbacks(progress, cancelled);
        const App::RunProgressCallback progressCallback = callbacks.progressCallback();
        const App::RunCancellationCallback cancellationCallback =
            callbacks.cancellationCallback();
        App::RunResult result;
        std::exception_ptr failure;
        {
            py::gil_scoped_release release;
            try
            {
                result = App::runOptimization(
                    options, progressCallback, cancellationCallback);
            }
            catch (...)
            {
                failure = std::current_exception();
            }
        }
        callbacks.finish();
        if (failure) std::rethrow_exception(failure);
        callbacks.throwIfFailed();
        return result;
    }
}

void bind_app(py::module_& module)
{
    py::class_<App::VariableInfo>(module, "VariableInfo")
        .def_readonly("name", &App::VariableInfo::name)
        .def_readonly("kind", &App::VariableInfo::kind)
        .def_readonly("value_type", &App::VariableInfo::valueType)
        .def_readonly("length", &App::VariableInfo::length)
        .def_readonly("lower_bound", &App::VariableInfo::lowerBound)
        .def_readonly("upper_bound", &App::VariableInfo::upperBound)
        .def_readonly("has_bounds", &App::VariableInfo::hasBounds);

    py::class_<App::ObjectiveInfo>(module, "ObjectiveInfo")
        .def_readonly("name", &App::ObjectiveInfo::name)
        .def_readonly("variables", &App::ObjectiveInfo::variables)
        .def_readonly("minimize", &App::ObjectiveInfo::minimize)
        .def_readonly("priority", &App::ObjectiveInfo::priority);

    py::class_<App::ConstraintInfo>(module, "ConstraintInfo")
        .def_readonly("type", &App::ConstraintInfo::type)
        .def_readonly("variables", &App::ConstraintInfo::variables)
        .def_readonly("penalty", &App::ConstraintInfo::penalty)
        .def_readonly("objectives", &App::ConstraintInfo::objectives);

    py::class_<App::ProblemDescription>(module, "ProblemDescription")
        .def_readonly("ok", &App::ProblemDescription::ok)
        .def_readonly("error", &App::ProblemDescription::error)
        .def_readonly("type", &App::ProblemDescription::type)
        .def_readonly("name", &App::ProblemDescription::name)
        .def_readonly("dimension", &App::ProblemDescription::dimension)
        .def_readonly("variables", &App::ProblemDescription::variables)
        .def_readonly("objectives", &App::ProblemDescription::objectives)
        .def_readonly("constraints", &App::ProblemDescription::constraints);

    py::class_<App::RunOptions>(module, "RunOptions")
        .def(py::init<>())
        .def_readwrite("problem_path", &App::RunOptions::problemPath)
        .def_readwrite("problem_name", &App::RunOptions::problemName)
        .def_readwrite("config_path", &App::RunOptions::configPath)
        .def_readwrite("optimizer_name", &App::RunOptions::optimizerName)
        .def_readwrite("tag", &App::RunOptions::tag)
        .def_readwrite("seed", &App::RunOptions::seed)
        .def_readwrite("runs", &App::RunOptions::runs)
        .def_readwrite("max_fes", &App::RunOptions::maxFES)
        .def_readwrite("max_convergence", &App::RunOptions::maxConv)
        .def_readwrite("max_time", &App::RunOptions::maxTime)
        .def_readwrite("run_start", &App::RunOptions::runStart)
        .def_readwrite("replace_existing", &App::RunOptions::replaceExisting)
        .def_readwrite("verbose", &App::RunOptions::verbose);

    py::class_<App::RunSolution>(module, "RunSolution")
        .def_readonly("fitness", &App::RunSolution::fitness)
        .def_readonly("objectives", &App::RunSolution::objectives)
        .def_readonly("text", &App::RunSolution::text);

    py::class_<App::RunRoundResult>(module, "RunRoundResult")
        .def_readonly("run", &App::RunRoundResult::run)
        .def_readonly("result_index", &App::RunRoundResult::resultIndex)
        .def_readonly("best", &App::RunRoundResult::best)
        .def_readonly("best_fitness", &App::RunRoundResult::bestFitness);

    py::class_<App::RunResult>(module, "RunResult")
        .def_readonly("ok", &App::RunResult::ok)
        .def_readonly("cancelled", &App::RunResult::cancelled)
        .def_readonly("error", &App::RunResult::error)
        .def_readonly("tag", &App::RunResult::tag)
        .def_readonly("optimizer_name", &App::RunResult::optimizerName)
        .def_readonly("run_start", &App::RunResult::runStart)
        .def_readonly("best", &App::RunResult::best)
        .def_readonly("rounds", &App::RunResult::rounds)
        .def_readonly("best_fitness", &App::RunResult::bestFitness);

    module.def("_describe_problem", &App::describeProblem,
               py::arg("path"), py::arg("logical_name") = std::string());
    module.def("_compile_and_describe_problem",
               &App::compileAndDescribeProblem,
               py::arg("path"), py::arg("expected_type"),
               py::arg("logical_name") = std::string());
    module.def("_run_optimization", &run_with_callbacks,
               py::arg("options"), py::arg("progress") = py::none(),
               py::arg("cancelled") = py::none());
}
