// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <functional>
#include <memory>

#include <pybind11/pybind11.h>

#include "app-facade.h"
#include "problem-handle.h"

namespace EvoFlow
{
    namespace Python
    {
        // Keeps Python callbacks alive while native optimization runs without the
        // GIL. Callback failures are recorded because Optimizer observers are
        // deliberately non-throwing; throwIfFailed() restores Python semantics
        // after control returns to the binding boundary.
        class RunCallbacks
        {
        public:
            RunCallbacks(const pybind11::object& progress,
                         const pybind11::object& cancelled);
            ~RunCallbacks();

            App::RunProgressCallback progressCallback() const;
            App::RunCancellationCallback cancellationCallback() const;
            void finish();
            void throwIfFailed() const;

        private:
            struct State;
            std::shared_ptr<State> _state;
        };

        // PyCompiledProblem intentionally remains private to bind-problem.cpp.
        // This bridge returns an owning native snapshot without exposing that
        // implementation type or a borrowed pointer to another translation unit.
        std::unique_ptr<ProblemHandle> cloneCompiledProblem(
            const pybind11::handle& value);
    }
}
