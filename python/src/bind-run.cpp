// SPDX-License-Identifier: BSD-3-Clause

#include "bindings.h"
#include "python-run.h"

#include <atomic>
#include <cctype>
#include <exception>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>

#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include "evoflow.h"
#include "python-config.h"

namespace py = pybind11;
using namespace EvoFlow;

namespace EvoFlow
{
    namespace Python
    {
        struct RunCallbacks::State
        {
            py::function progress;
            py::function cancelled;
            std::atomic<bool> active{true};
            std::atomic<bool> failed{false};
            std::atomic<bool> cancelRequested{false};
            mutable std::mutex errorMutex;
            std::string error;

            void fail(const char* kind, const std::string& message)
            {
                std::lock_guard<std::mutex> lock(errorMutex);
                if (failed.load()) return;
                error = std::string("Python ") + kind + " callback failed: " + message;
                cancelRequested.store(true);
                failed.store(true);
            }
        };

        namespace
        {
            py::function requireCallable(const py::object& value, const char* argument)
            {
                if (!PyCallable_Check(value.ptr()))
                    throw py::type_error(std::string(argument) + " must be callable or None");
                return py::reinterpret_borrow<py::function>(value);
            }

            std::string consumePythonError(py::error_already_set& source)
            {
                const std::string result = source.what();
                source.restore();
                PyErr_Clear();
                return result;
            }
        }

        RunCallbacks::RunCallbacks(const py::object& progress,
                                   const py::object& cancelled)
            : _state(std::make_shared<State>())
        {
            if (!progress.is_none())
                _state->progress = requireCallable(progress, "progress");
            if (!cancelled.is_none())
                _state->cancelled = requireCallable(cancelled, "cancelled");
        }

        RunCallbacks::~RunCallbacks() = default;

        App::RunProgressCallback RunCallbacks::progressCallback() const
        {
            const std::shared_ptr<State> state = _state;
            if (!state || !state->progress) return App::RunProgressCallback();
            return [state](int completed, int total) {
                if (!state->active.load()) return;
                py::gil_scoped_acquire acquire;
                if (!state->active.load()) return;
                try
                {
                    state->progress(completed, total);
                }
                catch (py::error_already_set& source)
                {
                    state->fail("progress", consumePythonError(source));
                }
                catch (const std::exception& source)
                {
                    state->fail("progress", source.what());
                }
                catch (...)
                {
                    state->fail("progress", "unknown error");
                }
            };
        }

        App::RunCancellationCallback RunCallbacks::cancellationCallback() const
        {
            const std::shared_ptr<State> state = _state;
            if (!state || (!state->cancelled && !state->progress))
                return App::RunCancellationCallback();
            return [state]() {
                if (!state->active.load()) return false;
                if (state->cancelRequested.load()) return true;
                if (!state->cancelled) return false;

                py::gil_scoped_acquire acquire;
                if (!state->active.load()) return false;
                if (state->cancelRequested.load()) return true;
                try
                {
                    const bool requested = py::cast<bool>(state->cancelled());
                    if (requested) state->cancelRequested.store(true);
                    return requested;
                }
                catch (py::error_already_set& source)
                {
                    state->fail("cancellation", consumePythonError(source));
                }
                catch (const std::exception& source)
                {
                    state->fail("cancellation", source.what());
                }
                catch (...)
                {
                    state->fail("cancellation", "unknown error");
                }
                return true;
            };
        }

        void RunCallbacks::finish()
        {
            if (!_state) return;
            _state->active.store(false);
            // finish() is called after gil_scoped_release has ended. Clearing the
            // objects here prevents stale native cancellation functions from
            // owning Python references during a later continuation or teardown.
            _state->progress = py::function();
            _state->cancelled = py::function();
        }

        void RunCallbacks::throwIfFailed() const
        {
            if (!_state || !_state->failed.load()) return;
            std::lock_guard<std::mutex> lock(_state->errorMutex);
            throw std::runtime_error(_state->error);
        }
    }
}

namespace
{
    bool portableTag(const std::string& value)
    {
        for (unsigned char ch : value)
            if (ch < 0x20 || std::isspace(ch) || ch == '"' || ch == '\\')
                return false;
        return true;
    }

    std::string tagOverride(const py::object& value, const std::string& fallback)
    {
        std::string result;
        if (value.is_none()) result = fallback;
        else
        {
            try { result = py::cast<std::string>(value); }
            catch (const py::cast_error&)
            {
                throw py::type_error("tag must be a string or None");
            }
        }
        if (result.size() > 128 || !portableTag(result))
            throw py::value_error(
                "tag must be at most 128 characters and contain no whitespace, control characters, quote, or backslash");
        return result;
    }

    std::string optimizerNameOverride(const py::object& value,
                                      const std::string& fallback)
    {
        std::string result = fallback;
        if (!value.is_none())
        {
            try { result = py::cast<std::string>(value); }
            catch (const py::cast_error&)
            {
                throw py::type_error("optimizer_name must be a string or None");
            }
        }
        if (result.empty() || result.size() > 128)
            throw py::value_error("optimizer_name must contain 1 to 128 characters");
        if (result == "." || result == "..")
            throw py::value_error("optimizer_name cannot be a path segment");
        for (unsigned char ch : result)
            if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
                  || (ch >= '0' && ch <= '9') || ch == '_'
                  || ch == '-' || ch == '.' || ch == '+'))
                throw py::value_error(
                    "optimizer_name may contain only ASCII letters, digits, '.', '_', '-' and '+'");
        return result;
    }

    int optionalRunStart(const py::object& value)
    {
        if (value.is_none()) return -1;
        if (PyBool_Check(value.ptr()))
            throw py::type_error("run_start must be a non-negative integer or None");
        int result;
        try { result = py::cast<int>(value); }
        catch (const py::cast_error&)
        {
            throw py::type_error("run_start must be a non-negative integer or None");
        }
        if (result < 0)
            throw py::value_error("run_start must be a non-negative integer or None");
        return result;
    }

    void applyBudget(FullConfig& config, int index, const py::object& value,
                     const char* argument)
    {
        if (value.is_none()) return;
        if (PyBool_Check(value.ptr()))
            throw py::type_error(std::string(argument) + " must be an integer or None");
        int limit;
        try { limit = py::cast<int>(value); }
        catch (const py::cast_error&)
        {
            throw py::type_error(std::string(argument) + " must be an integer or None");
        }
        if (limit < 0)
            throw py::value_error(std::string(argument) + " must be non-negative or None");
        config.optimizer.terminate_conditions[index] = limit;
        for (SubpopulationConfig& subpopulation : config.optimizer.subpopulations)
            subpopulation.terminate_conditions[index] = limit;
    }

    int positiveLimit(const py::object& value, const char* argument)
    {
        if (value.is_none()) return -1;
        if (PyBool_Check(value.ptr()))
            throw py::type_error(
                std::string(argument) + " must be a positive integer or None");
        int limit;
        try { limit = py::cast<int>(value); }
        catch (const py::cast_error&)
        {
            throw py::type_error(std::string(argument) + " must be a positive integer or None");
        }
        if (limit <= 0)
            throw py::value_error(std::string(argument) + " must be a positive integer or None");
        return limit;
    }

    App::RunRoundResult collectRoundResult(EvoFlow::Optimizer* optimizer, int run)
    {
        App::RunRoundResult round;
        round.run = run;
        round.resultIndex = optimizer ? optimizer->getRunIndex() : -1;
        Solution* best = nullptr;
        int bestSize = 0;
        if (!optimizer || !optimizer->getBest(best, bestSize)
            || !best || bestSize <= 0)
            return round;

        for (int i = 0; i < bestSize; ++i)
        {
            App::RunSolution solution;
            solution.fitness = best[i].fitness[0];
            const int objectiveCount = best[i].decoder_pointer->getObjectNumber();
            for (int k = 0; k < objectiveCount; ++k)
                solution.objectives.push_back(best[i].fitness[k]);
            solution.text = best[i].decoder_pointer->toString(
                best[i].result, best[i].fitness, 1);
            round.best.push_back(std::move(solution));
        }
        round.bestFitness = round.best.front().fitness;
        return round;
    }

    void completeResult(App::RunResult& result)
    {
        if (result.rounds.empty() || result.rounds.back().best.empty())
            throw std::runtime_error("no solution produced");
        result.best = result.rounds.back().best;
        result.bestFitness = result.rounds.back().bestFitness;
        result.runStart = result.rounds.front().resultIndex;
        result.ok = true;
        result.error.clear();
    }

    class PyOptimizer
    {
    public:
        PyOptimizer(const py::object& problem, const Python::Config& config,
                    const py::object& tag, const py::object& optimizerName,
                    const py::object& maxFES,
                    const py::object& maxConvergence, const py::object& maxTime,
                    bool verbose)
        {
            FullConfig snapshot = config.snapshot();
            if (snapshot.workflows.empty())
                throw py::value_error("config must contain at least one workflow");
            if (snapshot.optimizer.subpopulations.empty())
                throw py::value_error("config must contain at least one subpopulation");
            snapshot.optimizer.tag = tagOverride(tag, snapshot.optimizer.tag);
            snapshot.optimizer.name = optimizerNameOverride(
                optimizerName, snapshot.optimizer.name);
            applyBudget(snapshot, 0, maxFES, "max_fes");
            applyBudget(snapshot, 1, maxConvergence, "max_convergence");
            applyBudget(snapshot, 2, maxTime, "max_time");
            if (verbose)
            {
                snapshot.optimizer.logger_process = true;
                snapshot.optimizer.logger_console_echo = true;
            }

            std::unique_ptr<ProblemHandle> problemSnapshot =
                Python::cloneCompiledProblem(problem);
            OptimizerBuilder builder;
            for (const WorkflowConfig& workflow : snapshot.workflows)
                builder.registerWorkflow(workflow);
            _optimizer.reset(builder.buildOptimizer(snapshot.optimizer));
            _optimizer->setProblem(problemSnapshot.get());
            _tag = _optimizer->getTag();
            _optimizerName = snapshot.optimizer.name;
        }

        const std::string& tag() const { return _tag; }
        const std::string& optimizerName() const { return _optimizerName; }

        App::RunResult run(unsigned seed, int runs, const py::object& runStart,
                           bool replaceExisting, const py::object& progress,
                           const py::object& cancelled)
        {
            if (runs < 1 || runs > 10000)
                throw py::value_error("runs must be between 1 and 10000");
            if (_busy) throw std::runtime_error("optimizer is already running");
            const int requestedStart = optionalRunStart(runStart);
            if (replaceExisting && requestedStart < 0)
                throw py::value_error(
                    "replace_existing=True requires an explicit run_start");

            Python::RunCallbacks callbacks(progress, cancelled);
            const App::RunProgressCallback userProgress = callbacks.progressCallback();
            const App::RunCancellationCallback cancellation = callbacks.cancellationCallback();
            App::RunResult result;
            result.tag = _tag;
            result.optimizerName = _optimizerName;
            std::exception_ptr failure;
            std::exception_ptr collectionFailure;

            _busy = true;
            _canContinue = false;
            _optimizer->configureRunArtifacts(requestedStart, replaceExisting);
            {
                py::gil_scoped_release release;
                try
                {
                    _optimizer->exe(
                        runs, static_cast<time_t>(seed),
                        [&](int completed, int total) {
                            if (!collectionFailure)
                            {
                                try
                                {
                                    result.rounds.push_back(
                                        collectRoundResult(_optimizer.get(), completed));
                                }
                                catch (...)
                                {
                                    collectionFailure = std::current_exception();
                                }
                            }
                            if (userProgress) userProgress(completed, total);
                        },
                        cancellation);
                }
                catch (const OptimizationCancelled& source)
                {
                    result.cancelled = true;
                    result.error = source.what();
                }
                catch (...)
                {
                    failure = std::current_exception();
                }
            }
            callbacks.finish();
            _busy = false;
            if (result.runStart < 0 && _optimizer->getRunIndex() >= 0)
                result.runStart = result.rounds.empty()
                    ? _optimizer->getRunIndex() : result.rounds.front().resultIndex;

            try
            {
                if (failure) std::rethrow_exception(failure);
                if (collectionFailure) std::rethrow_exception(collectionFailure);
                callbacks.throwIfFailed();
                if (result.cancelled) return result;
                completeResult(result);
                _canContinue = (runs == 1);
                return result;
            }
            catch (...)
            {
                _canContinue = false;
                throw;
            }
        }

        App::RunResult continueRun(const py::object& maxFES,
                                   const py::object& maxConvergence,
                                   const py::object& maxTime)
        {
            if (_busy) throw std::runtime_error("optimizer is already running");
            if (!_canContinue)
                throw std::runtime_error(
                    "continue_run requires a successful, non-cancelled single run");

            const int fes = positiveLimit(maxFES, "max_fes");
            const int convergence = positiveLimit(maxConvergence, "max_convergence");
            const int seconds = positiveLimit(maxTime, "max_time");
            if (fes < 0 && convergence < 0 && seconds < 0)
                throw py::value_error("at least one continuation limit must be provided");

            std::unique_ptr<Terminator> terminator(new Terminator());
            if (fes > 0) terminator->setMaxFES(fes);
            if (convergence > 0) terminator->setMaxConvergence(convergence);
            if (seconds > 0) terminator->setMaxTime(seconds);

            _busy = true;
            std::exception_ptr failure;
            {
                py::gil_scoped_release release;
                try
                {
                    _optimizer->continue_exe(terminator.release());
                }
                catch (...)
                {
                    failure = std::current_exception();
                }
            }
            _busy = false;
            if (failure)
            {
                _canContinue = false;
                std::rethrow_exception(failure);
            }

            App::RunResult result;
            result.tag = _tag;
            result.optimizerName = _optimizerName;
            result.rounds.push_back(collectRoundResult(_optimizer.get(), 1));
            completeResult(result);
            return result;
        }

    private:
        std::unique_ptr<EvoFlow::Optimizer> _optimizer;
        std::string _tag;
        std::string _optimizerName;
        bool _busy = false;
        bool _canContinue = false;
    };
}

void bind_run(py::module_& module)
{
    py::class_<PyOptimizer>(module, "_Optimizer")
        .def(py::init<const py::object&, const Python::Config&,
                      const py::object&, const py::object&, const py::object&,
                      const py::object&, const py::object&, bool>(),
             py::arg("problem"), py::arg("config"), py::arg("tag") = py::none(),
             py::arg("optimizer_name") = py::none(),
             py::arg("max_fes") = py::none(),
             py::arg("max_convergence") = py::none(),
             py::arg("max_time") = py::none(), py::arg("verbose") = false)
        .def_property_readonly("tag", &PyOptimizer::tag)
        .def_property_readonly("optimizer_name", &PyOptimizer::optimizerName)
        .def("run", &PyOptimizer::run, py::arg("seed") = 1,
             py::arg("runs") = 1, py::arg("run_start") = py::none(),
             py::arg("replace_existing") = false,
             py::arg("progress") = py::none(),
             py::arg("cancelled") = py::none())
        .def("continue_run", &PyOptimizer::continueRun,
             py::arg("max_fes") = py::none(),
             py::arg("max_convergence") = py::none(),
             py::arg("max_time") = py::none());
}
