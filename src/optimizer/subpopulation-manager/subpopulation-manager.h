//------------------------Description------------------------
// SubpopulationManager:顶层 Population 内的子种群管理器基类 —— **定义子种群交互的方式**,并作为
//   **驱动者**调用另外两个种群级组件(构建器 = 个体如何形成子种群;拓扑 = 子种群交互的拓扑)。
//   负责 ini(初始化各子群 + 建拓扑邻居 + 注入)、runEpoch(跑一轮)、全局最优汇集、终止器协作。
// ★ 模板方法:ini/runEpoch 是**非虚的框架固定时序**,终止器协作与预算记账由基类包办;
//   子类只覆写钩子 interact()/iniHook()。setProblem/globalOptimumCollection 保持 virtual
//   (二者尚无框架骨架可言,去虚只会净损可拓展性)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "subpopulation.h"
#include "subpopulation-constructer.h"
#include "subpopulation-topology.h"
#include "best-archive.h"
#include "terminator.h"
#include "parameter-template.h"

namespace ECFlow
{
    class SubpopulationManager
    {
    protected:
        SubpopulationConstructer* _builder;
        SubpopulationTopology*    _model;
        Subpopulation**           _subpopulations;
        Terminator*               _terminator_pointer;
        Terminator**              _subterminator_pointers;
        int                       _swarm_number;

        // —— 子类扩展点(钩子)——
        //   ★ 为何是钩子而非让子类覆写整段时序:原设计里 update() 是纯虚,作者在其中以注释列出三段职责
        //     ——「更新种群结构」「种群交互」「更新终止条件」—— 而**四个子类中只有 SingleSwarm 写了第三段**,
        //     另外三个的终止器协作彻底缺失(实测:NoInteraction 下全局预算完全失效,真实评估 = 子群数 x 每轮预算)。
        //     把不变量交给"开发者记得照做"就是这个结果;现改为框架包办、子类碰不到。
        //   ★ 钩子只有 interact() 一个,因为**三个种群级组件的职责本就只有一处属于 manager**:
        //     ① 构建器 SubpopulationConstructer —— 决定**个体如何形成子种群**
        //     ② 拓扑   SubpopulationTopology    —— 决定**子种群交互的拓扑**(谁与谁交互)
        //     ③ 管理器 SubpopulationManager     —— 定义**子种群交互的方式**,并作为**驱动者**调用 ①②
        //     「个体迁移」与「种群重构」都只是**交互方式的一种**(前者 Immigrant、后者驱动 ①),同属 interact();
        //     而「重建拓扑」是**交互的后果、非并列职责** —— 仅当交互改变了子群数目、或拓扑本身是随机/动态的
        //     才需要重建,故由框架在 interact() 之后统一处理(见 runEpoch)。原注释把三者切成并列的三段,
        //     我据此建的 rebuild() 钩子曾把 ①② 焊成一个不可分的选择,已按上述语义订正。
        virtual void interact() {}   // 「交互方式」:迁移 / 驱动构建器重构 / ... (无交互则不覆写)
        virtual void iniHook()  {}   // 初始化末尾的额外动作

        // 把拓扑当前的邻居集注入各子群。子群持的是**快照**(_parent_buffer),故拓扑一变就必须重注入。
        void injectNeighbors()
        {
            for (int i = 0; i < _swarm_number; i++)
                _subpopulations[i]->setNeibors(_model->neighborhoods[i].getNSet(), _model->neighborhoods[i].getSize());
        }

    public:
        SubpopulationManager(SubpopulationConstructer* builder, SubpopulationTopology* model)
        {
            _builder = builder;
            _model = model;
            _swarm_number = 0;
            _subpopulations = nullptr;
            _terminator_pointer = nullptr;
            _subterminator_pointers = nullptr;
        }

        virtual ~SubpopulationManager()
        {
            delete _builder;
            delete _model;
            for (int i = 0; i < _swarm_number; i++)
                delete _subpopulations[i];
            delete[] _subpopulations;
            delete[] _subterminator_pointers;
        }

        static ParameterTemplate getParameterTemplate() { return ParameterTemplate{}; }

        // Pbuild 注入构建器/拓扑(替换 registry 建的裸管理器的 nullptr;删旧防泄漏)
        void setConstructer(SubpopulationConstructer* builder) { delete _builder; _builder = builder; }
        void setTopology(SubpopulationTopology* model) { delete _model; _model = model; }

        void setSwarmNumber(int number)
        {
            _swarm_number = number;
            _subpopulations = new Subpopulation * [number];
            _subterminator_pointers = new Terminator * [number];
        }

        void setTerminator(Terminator* terminator)
        {
            _terminator_pointer = terminator;
        }

        virtual void globalOptimumCollection(BestArchive* global_archive)
        {
            Solution* bests;
            int size = 0;
            for (int i = 0; i < _swarm_number; i++)
            {
                _subpopulations[i]->getBest(bests, size);
                for (int j = 0; j < size; j++)
                    global_archive->updateBest(bests[j]);
            }
        }

        void setSwarm(Subpopulation* subpopulation, int swarmid)
        {
            _subpopulations[swarmid] = subpopulation;
            _subterminator_pointers[swarmid] = subpopulation->getTerminator();
        }

        int getSwarmNumber()
        {
            return _swarm_number;
        }

        Subpopulation* getSubswarm(std::string id)
        {
            for (int i = 0; i < _swarm_number; i++)
                if (_subpopulations[i]->getID() == id)
                    return _subpopulations[i];
            return nullptr;
        }

        Subpopulation& operator[](const int index)
        {
            return *_subpopulations[index];
        }

        virtual void setProblem(ProblemHandle* problem_handle)
        {
            for (int i = 0; i < _swarm_number; i++)
                _subpopulations[i]->setProblem(problem_handle);
        }

        // ★ 框架固定时序,**非虚**:预算记账不得被子类改写或遗漏;子类的额外初始化走 iniHook()。
        void ini()
        {
            // 初始种群:**逐子群闭环**(设限 → 初始化 → 立刻并账 → 清零)。
            //   ⑴ 用 cappedBy 而非 limited —— 初始种群是一次性成本、在 epoch 结构之外,
            //      只受**总预算**约束;拿子群的"每轮预算"去限它会留下未初始化个体(见 Terminator::cappedBy)。
            //   ⑵ **必须逐个并账**:若批量设限再批量并账,后面的子群会看到**未扣减的陈旧剩余**
            //      → 每群都按同一个"剩余"各初始化一遍 → **N 倍超额**(3 群 x 种群 400 / 总预算 200 → 真实 600)。
            //   ⑶ 前提:调用方须**先 reset 全局终止器再调本函数**,否则 exe(n) 的第 2 次运行会读到上次的残账
            //      → 剩余算作 0 → 一个个体都不初始化。见 Optimizer::ini()。
            for (int i = 0; i < _swarm_number; i++)
            {
                _subpopulations[i]->resetTerminator();
                _subterminator_pointers[i]->cappedBy(_terminator_pointer);
                _subpopulations[i]->ini();
                _terminator_pointer->merge(_subterminator_pointers[i]);
                _subpopulations[i]->resetTerminator();
            }

            _builder->ini(_subpopulations, _swarm_number);
            _model->ini(_subpopulations, _swarm_number);
            injectNeighbors();

            iniHook();
        }

        // ★ 框架固定时序,**非虚**。一"轮"(epoch)= 交互 → 拓扑维护 → 逐子群(配额 → 跑 → 并账)。
        //   子群终止器装的是**每轮预算**(epoch),全局终止器装的是**总预算**;每轮由 limited 取
        //   min(每轮预算, 全局剩余) 下发配额 —— 故既不会跑超全局,也保持"每 N 次评估交互一次"的节奏。
        //   **逐子群闭环**同 ini() 之⑵:分配与扣除若分成两个批次,中间隔着 N 个子群的消耗 → 必有超额窗口
        //   (3 群 / 每轮 30 / 全局剩 50 → 各拿 30 → 真实 90)。闭环后每个子群看到的都是实时剩余,
        //   连"按子群数均分预算"这类策略都不需要(均分对"停滞代数"条件本就无意义)。
        void runEpoch()
        {
            const int before = _swarm_number;

            interact();

            // —— 拓扑维护:**交互的后果**,不是并列职责 ——
            //   ⑴ 交互改变了子群**数目** → 邻居集尺寸随之失效 → 走 ini(重分配 + 内部含 build)。
            //      (注:三个构建器目前**均不改** swarm_number——签名 `int&` 允许而无人行使——故这条分支
            //       当前不可达;真要行使还需 setSwarmNumber 重分配 _subpopulations/_subterminator_pointers,
            //       见文件头注 SMGR-SETSWARMS。此处按语义写全,不预造未被行使的机制。)
            //   ⑵ 否则重填邻居 → **随机/动态拓扑借此每轮刷新,静态拓扑填回相同内容**(swarm_number 通常 1~10,
            //      O(n²) 相对一轮几十次评估可忽略)。故"拓扑动不动态"由其 build() 是否随机**自动决定**,
            //      无需任何声明位 —— 也就没有"声明了却没人驱动"的余地(原 SubpopulationTopology::update()
            //      即此意图,但全仓 + 母版两版**三处零调用、五个拓扑无一覆写**,且语义与 build 重叠,已删)。
            //   ⑶ **必须重注入**:子群持的是 setNeibors 时**快照下来**的邻居指针(_parent_buffer),
            //      拓扑事后变了它看不见 —— 不重注入则重建不生效。
            if (_swarm_number != before)
                _model->ini(_subpopulations, _swarm_number);
            else
                _model->build(_subpopulations, _swarm_number);
            injectNeighbors();

            for (int i = 0; i < _swarm_number; i++)
            {
                _subterminator_pointers[i]->limited(_terminator_pointer);
                _subpopulations[i]->run();
                _terminator_pointer->merge(_subterminator_pointers[i]);
                _subpopulations[i]->resetTerminator();

                if (_terminator_pointer->termination()) break;   // 总预算耗尽 → 后续子群本轮不再跑
            }
        }
    };
}
