//------------------------Description------------------------
// This file defines the object in ECFlow. They support fitness 
// evaluate of candidate solutions.
//-------------------------Copyright-------------------------
// Copyright (c) 2024 ���������ƣ���ȷ�ϣ�, All Rights Reserved.
// You are free to use the ECFlow����ȷ�ϣ� for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference 
// "δȷ��"
//-----------------------------------------------------------

#pragma once
#include<string>

namespace ECFlow
{
    class Calculator;

    /**
     * @brief �Ż�Ŀ���࣬��װ���������е����Ż�Ŀ��ĺ�����������Ϊ
     *
     * ���ඨ�����Ż�Ŀ��Ļ���������Ŀ�����ơ��Ż�������Сֵ����/���ֵ���ţ������ȼ���
     * ������һ��Calculatorʵ������ɺ�ѡ�����Ӧ�ȼ��㣬֧��Ŀ�����Ŀ��������ȼ��Ƚϡ�
     */
    class Objective
    {
    private:
        std::string _name;          ///< Ŀ�����ƣ����ڱ�ʶ��ͬ���Ż�Ŀ�꣨��"�ܺ�"��"����"�ȣ�
        bool _min_is_better;        ///< �Ż������ǣ�true��ʾ��Сֵ���ţ���С��Ŀ�꣩��false��ʾ���ֵ���ţ�����Ŀ�꣩
        int _priority;              ///< Ŀ�����ȼ�����ֵԽ�����ȼ�Խ�ߣ������ڶ�Ŀ���Ż��е�Ŀ������
        Calculator* _calculator;    ///< ָ�������ʵ����ָ�룬����������Ӧ�ȼ����߼�

    public:
        /**
         * @brief Ĭ�Ϲ��캯������ʼ��Ŀ�����ΪĬ��״̬
         *
         * Ĭ��ֵ��
         * - ���ƣ�"none"
         * - ���ȼ���-1����Ч���ȼ���
         * - �Ż����򣺼�С����min_is_better = true��
         * - ����������ָ�루�����ͨ��setCalculator���ã�
         */
        Objective();

        /**
         * @brief ���������캯������ʼ��Ŀ���������
         * @param name Ŀ������
         * @param priority Ŀ�����ȼ�������Ǹ���������ֵԽ�����ȼ�Խ�ߣ�
         * @param min_is_better �Ż�����true=��С����false=����
         * @note ������ָ��Ĭ�ϳ�ʼ��Ϊ�գ���ͨ��setCalculator()������
         */
        Objective(std::string name, int priority, bool min_is_better);

        /**
         * @brief �����������ͷż�����ʵ�����ڴ�
         * @note ȷ��Calculator������ڴ汻��ȷ�ͷţ������ڴ�й©
         */
        ~Objective();

        /**
         * @brief ��ȡ��ǰĿ����Ż�����
         * @return bool��true=��С������Сֵ���ţ���false=���󻯣����ֵ���ţ�
         */
        bool IsMin() const;

        /**
         * @brief ��ȡĿ������
         * @return std::string��Ŀ��������ַ���
         */
        std::string getName() const;

        /**
         * @brief ����Ŀ�����ȼ�
         * @param priority �µ����ȼ���ֵ������Ǹ���������ֵԽ�����ȼ�Խ�ߣ�
         */
        void setPriority(int priority);

        /**
         * @brief ��ȡĿ�����ȼ�
         * @return int����ǰĿ������ȼ���ֵ
         */
        int priority() const;

        /**
         * @brief �󶨼�����ʵ������ǰĿ��
         * @param c_pointer ָ��Calculatorʵ����ָ��
         * @note �÷�����ӹܴ���ָ�������Ȩ������ʱ���Զ��ͷŸ��ڴ�
         */
        void setCalculator(Calculator* c_pointer);

        /**
         * @brief �����ѡ�����Ӧ��ֵ
         * @param solution ָ���ѡ�����ݵĶ�άָ�루��ʽ����Calculator::run���ݣ�
         * @return double������õ�����Ӧ��ֵ
         * @warning ����ǰ����ͨ��setCalculator����Ч��Calculatorʵ��������ᴥ����ָ���쳣
         */
        double getFitness(double** solution);

        /**
         * @brief ���ԴĿ�������������Ե���ǰ����
         * @param source ָ��ԴObjective�����ָ��
         * @note �����������ͨ����copy()���������������Objective����ͬһ������ʵ��
         */
        void copy(Objective* source);

        /**
         * @brief ����С�������������Ŀ��������ȼ���������
         * @param a ���Ƚϵ���һ��Objective����
         * @return bool����ǰ�������ȼ� > a�����ȼ�ʱ����true�����򷵻�false
         * @example ������std::sort����sort(objectives.begin(), objectives.end()) �ᰴ���ȼ��Ӹߵ�������
         */
        bool operator<(const Objective& a)const;
    };
}