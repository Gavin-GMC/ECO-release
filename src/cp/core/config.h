#pragma once
//
// cp/core/config.h
//
// 求解器运行参数（一处定义，State / propagate / violation 共用）。
//
#include <cstddef>

namespace ECFlow {

struct Config {
    // 单变量域 / 节点缓存允许的最大碎片数；超出则塌缩（填最小乘性比值的同号洞）。
    std::size_t max_intervals = 32;

    // 违反度 / 比较的容忍带，吸收浮点误差；|a-b| <= eps 视为相等。
    double eps = 1e-9;

    // != 被违反（两侧相等）时的固定罚值。
    double ne_penalty = 1.0;

    // 反向传播不动点的终止阈值：边界移动量同时 < delta_abs 且相对 < delta_rel
    // 才视为「无变化」，不再入队，避免连续域上的无限微缩。
    double delta_abs = 1e-9;
    double delta_rel = 1e-12;
};

} // namespace ECFlow
