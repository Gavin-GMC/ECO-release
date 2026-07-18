//------------------------Description------------------------
// This file defines the solution object. It contains two vectors,
// namely the result vector and the fitness vector.
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference 
// "未确定"
//-----------------------------------------------------------

#pragma once
#include<algorithm>
#include<cstring>
#include<memory>
#include"solution-decoder.h"

namespace ECFlow
{
    /**
    * @class Solution
    * @brief 优化求解结果核心类
    * @details 封装了求解结果的内存管理、数据拷贝、数组访问等功能，
    *          支持深拷贝（独立内存）和浅拷贝（共享内存）两种模式，
    *          提供运算符重载简化数组访问和对象比较。
    */
    class Solution
    {
    protected:
        /**
         * @var _size
         * @brief 结果数组（result）的长度
         * @details 表示优化问题中变量的总维度数，私有保护成员，仅可通过getSolutionSize()访问
         */
        int _size;

        /**
         * @var _object_number
         * @brief 适应度数组（fitness）的长度
         * @details 表示优化问题中目标函数的数量，私有保护成员，仅可通过getObjectNumber()访问
         */
        int _object_number;

    public:
        /**
         * @var decoder_pointer
         * @brief 指向SolutionDecoder的指针
         * @details 用于将当前Solution的数值结果格式化为可读字符串，
         *          支持空指针（空指针时无法调用格式化输出功能）
         */
        std::shared_ptr<SolutionDecoder> decoder_pointer;

        /**
         * @var result
         * @brief 优化求解的数值结果数组
         * @details 动态分配的double类型数组，长度为_size，存储变量的具体取值
         */
        double* result;

        /**
         * @var fitness
         * @brief 目标函数适应度值数组
         * @details 动态分配的double类型数组，长度为_object_number，存储每个目标函数的计算结果
         */
        double* fitness;

        /**
         * @brief 默认构造函数
         * @details 初始化所有成员变量为默认值（0或nullptr），不分配动态内存
         */
        Solution();

        /**
         * @brief 析构函数
         * @details 释放result和fitness指向的动态内存，防止内存泄漏；
         *          不释放decoder_pointer（避免野指针/重复释放，由外部管理）
         */
        ~Solution();

        // Rule of Three/Five(修 SOLUTION-RULE-OF-THREE):有析构释放 result/fitness,故补深拷贝/移动,
        // 否则默认浅拷贝共享裸指针 → 双重释放(INDIV-COMPOSE 值语义特性依赖此)。decoder 为 shared_ptr,安全。
        Solution(const Solution& source);                 // 深拷贝构造
        Solution(Solution&& source) noexcept;             // 移动构造
        Solution& operator=(const Solution& source);      // 深拷贝赋值
        Solution& operator=(Solution&& source) noexcept;  // 移动赋值

        /**
         * @brief 获取结果数组的长度
         * @return int - _size的当前值（非负整数）
         */
        int getSolutionSize();

        /**
         * @brief 获取适应度数组的长度
         * @return int - _object_number的当前值（非负整数）
         */
        int getObjectNumber();

        /**
         * @brief 设置结果数组和适应度数组的长度，并重新分配内存（不推荐使用，建议通过解码器间接设置）
         * @details 仅当传入的长度与当前值不同时，才释放旧内存并分配新内存；
         *          新分配的内存未初始化，需后续赋值。
         * @param size 结果数组的目标长度（非负整数）
         * @param object_number 适应度数组的目标长度（非负整数）
         * @warning 传入负整数会导致动态内存分配异常
         */
        void setSize(int size, int object_number);

        /**
         * @brief 从另一个Solution对象复制数组长度，并重新分配内存（不推荐使用，建议通过解码器间接设置）
         * @details 重载版本，调用setSize(int, int)实现，参数来自源对象的_size和_object_number
         * @param source 源Solution对象（const引用，避免拷贝）
         */
        void setSize(const Solution& source);

        /**
         * @brief 设置解码器指针
         * @details 将当前对象的decoder_pointer指向指定的SolutionDecoder对象
         * @param pointer SolutionDecoder对象的指针（可为nullptr）
         */
        void setDecoder(std::shared_ptr<SolutionDecoder> pointer);

        /**
         * @brief 设置解码器指针,并进行解映射
         * @details 将当前对象的decoder_pointer指向指定的SolutionDecoder对象，同时将在新编码下依旧存在的变量映射至新解的对应位置。
         * @param pointer SolutionDecoder对象的指针（不可为nullptr）
         * @param map 从原解到新解的映射
         */
        void setDecoder(std::shared_ptr<SolutionDecoder> pointer, const variableMapTable& map);

        /**
         * @brief 从另一个Solution对象复制解码器指针
         * @details 重载版本，调用setDecoder(SolutionDecoder*)实现，参数来自源对象的decoder_pointer
         * @param source 源Solution对象（const引用）
         */
        void setDecoder(const Solution& source);

        /**
         * @brief 深拷贝另一个Solution对象的所有数据
         * @details 1. 先调用setSize分配独立的内存；
         *          2. 使用memcpy拷贝result和fitness的内容（深拷贝，内存独立）；
         *          3. 浅拷贝decoder_pointer（共享解码器对象）。
         * @param copy_source 源Solution对象（const引用）
         * @warning 若源对象的result/fitness为nullptr，或长度不匹配，可能导致memcpy越界
         */
        void copy(const Solution& copy_source);

        /**
         * @brief 深拷贝外部的结果数组和适应度数组
         * @details 直接使用memcpy将外部数组内容拷贝到当前对象的result/fitness中；
         *          要求当前对象已通过setSize分配足够的内存。
         * @param source_result 外部结果数组的指针（非nullptr）
         * @param source_fitness 外部适应度数组的指针（非nullptr）
         * @warning 外部数组长度需与当前_size/_object_number匹配，否则会内存越界
         */
        void copy(const double* source_result, const double* source_fitness);

        /**
         * @brief 浅拷贝另一个Solution对象的所有数据
         * @details 直接赋值所有成员变量（包括指针），不分配新内存；
         *          拷贝后两个对象共享同一块内存，修改一个会影响另一个。
         * @param copy_source 源Solution对象（const引用）
         * @note 浅拷贝后需注意内存释放顺序，避免重复释放
         */
        void shallowCopy(const Solution& copy_source);

        /**
         * @brief 浅清空所有成员变量
         * @details 将所有成员变量重置为默认值（0或nullptr），不释放动态内存；
         *          需外部手动释放内存，避免内存泄漏。
         */
        void shallowClear();

        /**
         * @brief 交换当前对象与另一个Solution对象的所有成员变量
         * @details 使用std::swap交换所有成员（包括指针和整型变量），
         *          交换后两个对象的内存所有权也随之交换。
         * @param copy_source 待交换的Solution对象（非const引用）
         */
        void swap(Solution& copy_source);

        /**
         * @brief 重载[]运算符，访问结果数组的指定元素
         * @details 支持通过下标直接访问result数组，可读写
         * @param index 数组下标（非负整数，需小于_size）
         * @return double& - result[index]的引用
         * @warning 下标越界会导致未定义行为（内存访问错误）
         */
        double& operator[](const int index);

        /**
         * @brief 重载==运算符，比较两个Solution对象的结果数组是否相同
         * @details 使用memcmp比较result数组的内容，仅当数组长度和内容都相同时返回true；
         *          不比较fitness和decoder_pointer。
         * @param a 待比较的Solution对象（const引用）
         * @return bool - 数组内容相同返回true，否则返回false
         * @note 若_size为0，默认返回true；若其中一个result为nullptr，可能导致memcmp异常
         */
        bool operator==(const Solution& a) const;
    };
}
