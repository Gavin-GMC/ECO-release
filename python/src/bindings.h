// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <pybind11/pybind11.h>

void bind_app(pybind11::module_& module);
void bind_problem(pybind11::module_& module);
void bind_template(pybind11::module_& module);
void bind_config(pybind11::module_& module);
void bind_run(pybind11::module_& module);
void bind_analysis(pybind11::module_& module);
void bind_callback_probe(pybind11::module_& module);
