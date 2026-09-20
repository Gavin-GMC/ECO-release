// SPDX-License-Identifier: BSD-3-Clause

#include <pybind11/pybind11.h>

#include "bindings.h"
#include "evoflow-version.h"

namespace py = pybind11;

PYBIND11_MODULE(_core, module)
{
    module.doc() = "EvoFlow " EVOFLOW_VERSION_STRING " Python bindings";
    module.attr("__version__") = EVOFLOW_VERSION_STRING;

#ifdef EVOFLOW_PARALLEL_SUBPOP
    module.attr("parallel_subpop_build") = true;
#else
    module.attr("parallel_subpop_build") = false;
#endif

    bind_app(module);
    bind_problem(module);
    bind_template(module);
    bind_config(module);
    bind_run(module);
    bind_analysis(module);
    bind_callback_probe(module);
}
