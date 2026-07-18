#pragma once
#include <sortlib.hpp>   // template bodies live in-header so all TUs can instantiate them

namespace ECFlow
{
    /**
    * @brief �������ṹ�壺���ڰ�ID�Ͷ�Ӧ��ֵ��֧���Զ������͵�����Ƚ�
    * @tparam idT ID�����ͣ���int��std::string�ȣ�
    * @tparam vlT ֵ�����ͣ���int��float��double�ȣ�
    */
    template<class idT, class vlT>
    struct sortHelper {
        // ��ʶID�������ڹ������ݵ�Ψһ��ʶ��
        idT id;
        // ��ID�󶨵���ֵ������ĺ������ݣ�
        vlT value;

        /**
         * @brief Ĭ�Ϲ��캯��
         * @note �ṩ�չ��캯����֧��Ĭ�ϳ�ʼ������sortHelper<int, float> helper;��
         */
        sortHelper() {}

        /**
         * @brief �������Ĺ��캯������ʼ��ID�Ͷ�Ӧ��ֵ
         * @param id Ҫ�󶨵�IDֵ
         * @param value Ҫ�󶨵���ֵ
         */
        sortHelper(idT id, vlT value)
        {
            this->id = id;
            this->value = value;
        }

        /**
         * @brief ����С���������������������ıȽϹ���
         * @param a ���Ƚϵ���һ��sortHelper����
         * @return bool ��ǰ�����value < a.value ʱ����true�����򷵻�false
         * @note �Ƚϵĺ�����value��Ա��id����Ϊ�������ݲ�����Ƚ�
         */
        bool operator<(const sortHelper& a)const
        {
            return value < a.value;
        }

        /**
         * @brief ���ش�������������ڽ�������ıȽϹ���
         * @param a ���Ƚϵ���һ��sortHelper����
         * @return bool ��ǰ�����value > a.value ʱ����true�����򷵻�false
         * @note �Ƚϵĺ�����value��Ա��id����Ϊ�������ݲ�����Ƚ�
         */
        bool operator>(const sortHelper& a)const
        {
            return value > a.value;
        }
    };

    /**
     * @brief ͨ����������������ԭ���������ԭ������
     * @tparam T ����Ԫ�ص����ͣ���֧��<��>����������Զ����˱ȽϹ���
     * @param left ָ��������ʼλ�õ�ָ�루�������߽磩
     * @param right ָ������ĩβ��һ��λ�õ�ָ�루������ұ߽磬����ҿ���
     * @param is_ascending �Ƿ���������Ĭ��true������false������
     * @note ����Χ�� [left, right)��������leftָ���Ԫ�أ�������rightָ���Ԫ��
     */
    template<class T>
    void sort(T* left, T* right, bool is_ascending = true)
    {
        sortlib::sortArray<T>(left, right - left, is_ascending ? sortlib::SortOrder::ASC : sortlib::SortOrder::DESC);
    }

    /**
     * @brief ָ����������������ָ��T���͵�ָ�������������
     * @tparam T ָ��ָ���Ԫ�����ͣ���֧��<��>����������Զ����˱ȽϹ���
     * @param left ָ��ָ��������ʼλ�õ�ָ�루�������߽磩
     * @param right ָ��ָ������ĩβ��һ��λ�õ�ָ�루������ұ߽磬����ҿ���
     * @param is_ascending �Ƿ���������Ĭ��true������false������
     * @note 1. �������ָ��ָ���**ʵ��ֵ**������ָ���ַ����
     *       2. ����Χ�� [left, right)��������ָ���ָ��˳�򣬲��޸�ԭ����
     */
    template<class T>
    void pointer_sort(T** left, T** right, bool is_ascending = true)
    {
        sortlib::sortPointerArray<T>(left, right - left, is_ascending ? sortlib::SortOrder::ASC : sortlib::SortOrder::DESC);
    }

    /**
     * @brief ���������������������һ�������ͬʱ��ͬ�������ڶ��������Ԫ��˳��
     * @tparam T1 ��һ�������Ԫ�����ͣ���������ݣ���֧��<��>�������
     * @tparam T2 �ڶ��������Ԫ�����ͣ���ͬ������˳�����������ƣ�
     * @param a ��һ�����飨����ĺ������飬�Դ������˳��Ϊ��׼��
     * @param b �ڶ������飨�������飬����a������ͬ������Ԫ��λ�ã�
     * @param len �����������Ч���ȣ����뱣֤a��b�ĳ��ȡ�len�������Խ�磩
     * @param is_ascending �Ƿ���������Ĭ��true������false������
     * @note 1. ���������Ԫ����һһ��Ӧ�ģ�a[i] ��Ӧ b[i]��
     *       2. �����a���鰴��������b�����Ԫ��λ��ͬ������a�仯
     */
    template <typename T1, typename T2>
    void sort_associated_arrays(T1* a, T2* b, size_t len, bool is_ascending = true)
    {
        sortlib::sortAssociatedArrays(a, b, len, is_ascending ? sortlib::SortOrder::ASC : sortlib::SortOrder::DESC);
    }
}
