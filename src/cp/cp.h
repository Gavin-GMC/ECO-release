// SPDX-License-Identifier: BSD-3-Clause

#pragma once
//
// cp/cp.h —— 库总入口（header-only）。包含本头即可使用全部功能。
//
//   ConstraintSystemBuilder b;
//   b.add("a + b <= 10").add("a * c == d", 2.0);
//   EvoFlow::Program prog = b.compile();          // 编译一次，只读可共享
//
//   EvoFlow::State st(prog);                       // 变量域，可廉价切换
//   st.set_domain(prog.slot("a"), EvoFlow::interval_set{{0,5}});
//   st.assign(prog.slot("c"), 3.0);
//
//   EvoFlow::SolveReport r = EvoFlow::solve(st);        // 模式 A：实时可行域
//   const auto& da = st.domain(prog.slot("a"));
//   double v = EvoFlow::violation(st).total;       // 模式 B：违反度（变量皆点值时）
//   const auto& z = EvoFlow::value_domain(st, id); // 模式 C：结果域
//
#include "cp/core/config.h"
#include "cp/core/interval.h"
#include "cp/core/interval_set.h"
#include "cp/compile/program.h"
#include "cp/compile/parser.h"
#include "cp/domain/variable_domain.h"
#include "cp/solver.h"
