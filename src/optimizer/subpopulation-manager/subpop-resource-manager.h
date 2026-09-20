//------------------------Description------------------------
// SubpopResourceManager:归属 SubpopulationManager 的**线程分配器**——把一轮(epoch)内
//   N 个子群的 run() 分派到多线程并等齐。优化器核心层的首个并行设施(此前核心层零并行基础设施,
//   std::thread 仅存于 UI/LLM 层)。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <thread>
#include <vector>
#include <exception>

namespace EvoFlow
{
    class SubpopResourceManager
    {
    public:
        // task(i):执行第 i 个子群的一轮 run()。并行跑 [0,n) 并等齐;任一抛异常则按子群号序重抛第一个。
        template <class Fn>
        void parallelRun(int n, Fn task)
        {
            if (n <= 0) return;
            if (n == 1) { task(0); return; }   // 单子群退化:不起线程

            std::vector<std::exception_ptr> errs(n, nullptr);   // per-子群槽,各写各的
            std::vector<std::thread> workers;
            workers.reserve(n);
            for (int i = 0; i < n; i++)
                workers.emplace_back([i, &task, &errs]()
                {
                    try { task(i); }
                    catch (...) { errs[i] = std::current_exception(); }
                });
            for (auto& w : workers) w.join();

            // 按子群号顺序:第一个非空异常重抛(确定性,不依赖线程完成序)
            for (int i = 0; i < n; i++)
                if (errs[i]) std::rethrow_exception(errs[i]);
        }
    };
}
