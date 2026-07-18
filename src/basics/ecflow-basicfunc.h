#pragma once
#include <cmath>
#include"ecflow-constant.h"
#include"ecflow-sys.h"
#include"ecflow-math.h"
#include"ecflow-sort.h"
#include"ecflow-rand.h"
#include"ecflow-time.h"
#include"../../thirdparty/stringlib/src/stringlib/stringlib/stringlib.hpp"

namespace ECFlow
{
    using stringlib::stringSplit;

    // is_empty(v) 已移至 ecflow-constant.h(与 EMPTYVALUE 定义放在一起,使"凡见哨兵即见判定")。
    //   本文件 include 了 ecflow-constant.h,故经本文件可见 is_empty 的调用点全部不受影响。
}