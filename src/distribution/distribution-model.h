//------------------------Description------------------------
// 常用基础分布模型:支持基于样本的建模、更新与取值(采样)。DistributionModel 基类 + DM_Gaussian/DM_Cauchy/DM_Uniform/DM_Histogram。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <cmath>
#include <vector>
#include "ecflow-rand.h"    // ECFlow::rand01(), ECFlow::rand01_()

namespace ECFlow
{
    // 分布模型的抽象基类
    class DistributionModel
    {
    public:
        DistributionModel() {}
        virtual ~DistributionModel() {}

        virtual void   clear() = 0;                            // 清空分布参数与样本信息
        virtual void   addSample(double sample) = 0;           // 为分布添加样本
        virtual void   build() = 0;                            // 依据已加样本建模
        virtual double getValue() = 0;                         // 基于分布采样一个值
        virtual void   setModel(double* paras) = 0;            // 直接以参数构造模型
        virtual void   iniByDomain(double lo, double hi) = 0;  // 按问题域 [lo,hi] 冷启动构造初值(无样本,供分布初始化)

        // 基于给定样本集一次性构造/初始化模型
        virtual void ini(double* sample_set, size_t size)
        {
            clear();
            for (size_t i = 0; i < size; i++)
                addSample(sample_set[i]);
            build();
        }
    };

    // 高斯分布模型
    class DM_Gaussian final : public DistributionModel
    {
    private:
        double _mean;
        double _stdv;
        double _difference;
        int    _sample_number;

        // Box-Muller:由两个均匀随机数生成标准正态,再线性变换到 (mean, stdv)
        double gaussian(double mean = 0, double stdv = 1)
        {
            return mean + stdv * (
                sqrt((-2) * log(ECFlow::rand01_()))
                * sin(2 * 3.1415926 * ECFlow::rand01()));
        }

    public:
        DM_Gaussian() : DistributionModel() { clear(); }

        void clear() override
        {
            _mean = 0;
            _stdv = 0;
            _difference = 0;
            _sample_number = 0;
        }

        void addSample(double sample) override
        {
            // Welford 在线均值/方差
            _sample_number++;
            double new_mean = _mean + (sample - _mean) / _sample_number;
            _difference = _difference + (sample - _mean) * (sample - new_mean);
            _mean = new_mean;
        }

        void build() override
        {
            if (_sample_number == 1) return;
            _stdv = sqrt(_difference / (_sample_number - 1));
        }

        double getValue() override { return gaussian(_mean, _stdv); }

        void setModel(double* paras) override
        {
            _mean = paras[0];
            _stdv = paras[1];
            _sample_number = int(paras[2]);
            _difference = pow(_stdv, 2) * (_sample_number - 1);
        }

        void iniByDomain(double lo, double hi) override   // 域中心正态,stdv=域宽/6(±3σ≈铺满全域)
        {
            double p[3] = { (lo + hi) / 2.0, (hi - lo) / 6.0, 2 };
            setModel(p);
        }
    };

    // 柯西分布模型:重尾长跳(比高斯更易逃离局部最优)。估计沿用高斯(Welford 均值=位置、标准差≈尺度,无缓冲),仅采样改柯西。
    class DM_Cauchy final : public DistributionModel
    {
    private:
        double _location;   // 位置(中心),样本均值估计
        double _scale;      // 尺度,样本标准差近似
        double _difference;
        int    _sample_number;

        // 逆变换采样:x0 + γ·tan(π·(u-0.5)),u∈(0,1) → 参数 ∈(-π/2,π/2),tan 有限
        double cauchy(double location, double scale)
        {
            return location + scale * tan(3.1415926 * (ECFlow::rand01_() - 0.5));
        }

    public:
        DM_Cauchy() : DistributionModel() { clear(); }

        void clear() override
        {
            _location = 0;
            _scale = 0;
            _difference = 0;
            _sample_number = 0;
        }

        void addSample(double sample) override
        {
            // Welford 在线均值/方差(同高斯)
            _sample_number++;
            double new_mean = _location + (sample - _location) / _sample_number;
            _difference = _difference + (sample - _location) * (sample - new_mean);
            _location = new_mean;
        }

        void build() override
        {
            if (_sample_number <= 1) return;
            _scale = sqrt(_difference / (_sample_number - 1));
        }

        double getValue() override { return (_scale <= 0) ? _location : cauchy(_location, _scale); }

        void setModel(double* paras) override
        {
            _location = paras[0];
            _scale = paras[1];
            _sample_number = int(paras[2]);
            _difference = pow(_scale, 2) * (_sample_number > 1 ? _sample_number - 1 : 0);
        }

        void iniByDomain(double lo, double hi) override   // 域中心 + 重尾,scale=域宽/10
        {
            double p[3] = { (lo + hi) / 2.0, (hi - lo) / 10.0, 2 };
            setModel(p);
        }
    };

    // 区间均匀分布模型:用好解每维 [min,max] 建区间、均匀采样(PBIL-c 风格,最轻量)。
    class DM_Uniform final : public DistributionModel
    {
    private:
        double _min, _max;
        int    _sample_number;

    public:
        DM_Uniform() : DistributionModel() { clear(); }

        void clear() override { _min = 0; _max = 0; _sample_number = 0; }

        void addSample(double sample) override
        {
            if (_sample_number == 0) { _min = _max = sample; }
            else { if (sample < _min) _min = sample; if (sample > _max) _max = sample; }
            _sample_number++;
        }

        void build() override {}   // min/max 已在 addSample 中就绪

        double getValue() override
        {
            if (_sample_number == 0 || _max <= _min) return _min;   // 未建模/退化 → min(fresh=0)
            return ECFlow::rand01() * (_max - _min) + _min;
        }

        void setModel(double* paras) override
        {
            _min = paras[0];
            _max = paras[1];
            _sample_number = int(paras[2]);
        }

        void iniByDomain(double lo, double hi) override   // 域内均匀
        {
            double p[3] = { lo, hi, 2 };
            setModel(p);
        }
    };

    // 边缘直方图模型(UMDA_c):分箱统计好解每维分布,频率轮盘选箱 + 箱内均匀采样。能表达多峰(高斯只能单峰)。
    class DM_Histogram final : public DistributionModel
    {
    private:
        std::vector<double> _samples;
        std::vector<int>    _counts;
        double _min, _max;
        int    _bins;
        int    _total;

    public:
        DM_Histogram(int bins = 10) : DistributionModel(), _bins(bins > 0 ? bins : 10) { clear(); }

        void clear() override { _samples.clear(); _counts.clear(); _min = 0; _max = 0; _total = 0; }

        void addSample(double sample) override { _samples.push_back(sample); }

        void build() override
        {
            _total = 0;
            _counts.assign(_bins, 0);
            if (_samples.empty()) return;
            _min = _max = _samples[0];
            for (double s : _samples) { if (s < _min) _min = s; if (s > _max) _max = s; }
            if (_max <= _min) { _total = int(_samples.size()); return; }   // 全同值 → 退化(getValue 返回 min)
            double w = (_max - _min) / _bins;
            for (double s : _samples)
            {
                int b = int((s - _min) / w);
                if (b < 0) b = 0;
                if (b >= _bins) b = _bins - 1;
                _counts[b]++;
                _total++;
            }
        }

        double getValue() override
        {
            if (_total == 0 || _max <= _min) return _min;   // 未建模/退化 → min(fresh=0)
            // 按频率轮盘选箱
            int r = int(ECFlow::rand01() * _total);
            if (r >= _total) r = _total - 1;
            int b = 0, acc = 0;
            for (; b < _bins; b++) { acc += _counts[b]; if (r < acc) break; }
            if (b >= _bins) b = _bins - 1;
            double w = (_max - _min) / _bins;
            return _min + (b + ECFlow::rand01()) * w;   // 选中箱内均匀
        }

        void setModel(double* paras) override   // [min, max, bins]:建等频均匀直方图(各箱计数=1)
        {
            _min = paras[0];
            _max = paras[1];
            _bins = (paras[2] > 0) ? int(paras[2]) : _bins;
            _counts.assign(_bins, 1);
            _total = _bins;
        }

        void iniByDomain(double lo, double hi) override   // 无数据冷启动 → [lo,hi] 均匀直方图
        {
            double p[3] = { lo, hi, double(_bins) };
            setModel(p);
        }
    };
}
