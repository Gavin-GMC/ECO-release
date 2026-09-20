// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <cmath>
#include"evoflow-constant.h"
#include"evoflow-sys.h"
#include"evoflow-math.h"
#include"evoflow-sort.h"
#include"evoflow-rand.h"
#include"evoflow-time.h"
#include"../../thirdparty/stringlib/src/stringlib/stringlib/stringlib.hpp"

namespace EvoFlow
{
    using stringlib::stringSplit;

    // is_empty(v) 已移至 evoflow-constant.h(与 EMPTYVALUE 定义放在一起,使"凡见哨兵即见判定")。
    //   本文件 include 了 evoflow-constant.h,故经本文件可见 is_empty 的调用点全部不受影响。
}
