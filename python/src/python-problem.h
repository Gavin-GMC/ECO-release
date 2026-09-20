// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <memory>
#include <string>

#include <pybind11/pybind11.h>

namespace EvoFlow
{
    class Problem;
}

namespace EvoFlowPython
{
    pybind11::object adoptProblem(
        std::unique_ptr<EvoFlow::Problem> problem,
        const std::string& sourceType,
        const std::string& logicalName = std::string());
}
