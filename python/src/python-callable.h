// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <pybind11/pybind11.h>

#include "evoflow-functor.hpp"

namespace EvoFlowPython
{
    namespace py = pybind11;

    struct PythonCallableState
    {
        PythonCallableState(py::function source, std::vector<int> lengths,
                            std::string callable_kind, std::string callable_name,
                            int scalar_prefix = 0)
            : callback(std::move(source)), input_lengths(std::move(lengths)),
              kind(std::move(callable_kind)), name(std::move(callable_name)),
              prefix_scalars(scalar_prefix) {}

        ~PythonCallableState()
        {
            if (!callback) return;
            if (Py_IsInitialized())
            {
                py::gil_scoped_acquire acquire;
                callback = py::function();
            }
            else
            {
                callback.release();
            }
        }

        py::function callback;
        std::vector<int> input_lengths;
        std::string kind;
        std::string name;
        int prefix_scalars = 0;
    };

    class PythonCalculationFunctor final : public EvoFlow::eccalcul_functor
    {
    public:
        explicit PythonCalculationFunctor(std::shared_ptr<PythonCallableState> state)
            : _state(std::move(state)) {}

        double operator()(double** input) const override
        {
            double result = 0.0;
            std::string error;
            {
                py::gil_scoped_acquire acquire;
                try
                {
                    const std::size_t argc = static_cast<std::size_t>(_state->prefix_scalars)
                        + _state->input_lengths.size();
                    py::tuple args(argc);
                    for (int i = 0; i < _state->prefix_scalars; ++i)
                        args[static_cast<py::ssize_t>(i)] = py::float_(input[i][0]);
                    for (std::size_t i = 0; i < _state->input_lengths.size(); ++i)
                    {
                        const int length = _state->input_lengths[i];
                        py::tuple values(length);
                        double* source = input[i + static_cast<std::size_t>(_state->prefix_scalars)];
                        for (int k = 0; k < length; ++k)
                            values[k] = py::float_(source[k]);
                        args[static_cast<py::ssize_t>(i + _state->prefix_scalars)] = std::move(values);
                    }
                    PyObject* raw = PyObject_CallObject(_state->callback.ptr(), args.ptr());
                    if (!raw) throw py::error_already_set();
                    py::object value = py::reinterpret_steal<py::object>(raw);
                    result = py::cast<double>(value);
                }
                catch (py::error_already_set& source)
                {
                    error = source.what();
                    source.restore();
                    PyErr_Clear();
                }
                catch (const py::cast_error& source)
                {
                    error = source.what();
                }
            }
            if (!error.empty())
                throw std::runtime_error(
                    "Python " + _state->kind + " callback '" + _state->name
                    + "' failed: " + error);
            return result;
        }

        EvoFlow::eccalcul_functor* copy() override
        {
            return new PythonCalculationFunctor(_state);
        }

    private:
        std::shared_ptr<PythonCallableState> _state;
    };
}
