//------------------------Description------------------------
// 个体数组 IndividualArray(旧名 Population):管理一组同型候选解个体(容量/大小分离,类 vector)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include "individual.h"
#include "registry.h"

namespace ECFlow
{
    class IndividualArray
    {
    private:
        int          _individual_buffer_size;
        int          _population_size;
        Individual** _individuals;
        //   个体只剩基类、Registry<Individual> 已移除)。留着等于让调用方以为"类型名有用"。
        std::vector<FeatureSpec> _feature_spec;   // INDIV-COMPOSE 路径:非空则造基类+挂推断特性

        // 造 length 个个体:有推断特性集 → 基类 Individual + 挂特性;否则直接造基类 Individual(Registry<Individual> 已移除)
        Individual** newArray(int length)
        {
            if (!_feature_spec.empty())
            {
                Individual** arr = new Individual*[length];
                for (int i = 0; i < length; ++i)
                {
                    Individual* ind = new Individual();
                    for (auto& fs : _feature_spec)
                    {
                        Feature* f = Registry<Feature>::instance().create(
                            fs.kind, fs.params.empty() ? nullptr : fs.params.data());
                        if (f) f->setInitPolicy(fs.init, fs.init_value);   // 组件声明的初始化策略戳入特性
                        ind->addFeature(fs.key, f);
                    }
                    arr[i] = ind;
                }
                return arr;
            }
            // 无推断特性:个体只剩基类 → 直接造 length 个基类 Individual
            Individual** arr = new Individual*[length];
            for (int i = 0; i < length; ++i) arr[i] = new Individual();
            return arr;
        }

    public:
        IndividualArray()
        {
            _individual_buffer_size = 0;
            _population_size = 0;
            _individuals = newArray(0);       // 长度 0 的数组(非 nullptr)
        }

        // 无推断特性:只造基类。,但 type_name 从不被读 → 已删)
        explicit IndividualArray(int size)
        {
            _individual_buffer_size = size;
            _population_size = size;
            _individuals = newArray(size);
        }

        // INDIV-COMPOSE:按推断特性集造(基类 Individual + 挂特性)
        IndividualArray(const std::vector<FeatureSpec>& spec, int size = 1)
        {
            _individual_buffer_size = size;
            _population_size = size;
            _feature_spec = spec;
            _individuals = newArray(size);
        }

        ~IndividualArray()
        {
            for (int i = 0; i < _individual_buffer_size; i++)
                delete _individuals[i];
            delete[] _individuals;
        }

        void ini()
        {
            for (int i = 0; i < _individual_buffer_size; i++)
                _individuals[i]->ini(true, true, true);
        }

        int getSize() { return _population_size; }

        Individual** getIndividuals() { return _individuals; }

        void resize(int target_size)
        {
            if (target_size <= _individual_buffer_size)
            {
                _population_size = target_size;
            }
            else
            {
                int array_size = _population_size;
                if (array_size == 0)                  // 修复 IARR-RESIZE:空数组直接到目标(否则 += 0 死循环)
                    array_size = target_size;
                else
                    while (array_size < target_size)
                        array_size += _population_size;   // 几何增长(按 population_size 倍数累加)

                Individual** individuals_buffer = _individuals;
                _individuals = newArray(array_size);

                for (int i = 0; i < _individual_buffer_size; i++)
                    std::swap(_individuals[i], individuals_buffer[i]);   // 把旧个体换进新数组

                for (int i = 0; i < _individual_buffer_size; i++)
                    delete individuals_buffer[i];
                delete[] individuals_buffer;

                for (int i = _individual_buffer_size; i < array_size; i++)
                    _individuals[i]->setProblem(**_individuals);   // 新个体以第 0 个为模板配置

                _individual_buffer_size = array_size;
                _population_size = target_size;
            }
        }

        void extend(int target_size) { resize(_population_size + target_size); }

        void clear() { _population_size = 0; }

        void append(Individual* individuals, int size = 1)
        {
            if (_population_size + size > _individual_buffer_size)
            {
                resize(_population_size + size);
                _population_size -= size;
            }
            for (int i = 0; i < size; i++)
                _individuals[_population_size++]->copy(individuals[i]);
        }

        void remove(Individual* remove_individuals, int size = 1)
        {
            for (int i = 0; i < size; i++)
                for (int j = 0; j < _population_size; j++)
                    if (remove_individuals[i] == *(_individuals[j]))
                        std::swap(_individuals[j], _individuals[--_population_size]);   // 换到尾部 + 缩 size
        }

        void remove(int* remove_individuals_index, int size = 1)
        {
            std::sort(remove_individuals_index, remove_individuals_index + size, std::greater<int>());
            int last_index = -1;
            for (int i = 0; i < size; i++)
            {
                if (remove_individuals_index[i] < 0) break;
                if (remove_individuals_index[i] >= _population_size || remove_individuals_index[i] == last_index) continue;
                _population_size--;
                std::swap(_individuals[remove_individuals_index[i]], _individuals[_population_size]);
                last_index = remove_individuals_index[i];
            }
        }

        void sort()
        {
            int n = _population_size;
            bool swapped;
            for (int i = 0; i < n - 1; ++i)
            {
                swapped = false;
                for (int j = 0; j < n - 1 - i; ++j)
                    if (*_individuals[j + 1] < *_individuals[j])
                    {
                        _individuals[j]->swap(*_individuals[j + 1]);
                        swapped = true;
                    }
                if (!swapped) break;
            }
        }

        void setProblem(ProblemHandle* problem_handle)
        {
            for (int i = 0; i < _individual_buffer_size; i++)
                _individuals[i]->setProblem(problem_handle);
        }

        Individual& operator[](int i) { return *(_individuals[i]); }
    };
}
