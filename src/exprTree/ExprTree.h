//------------------------Description------------------------
// 表达式树计算器后端:基于 muParserX 解析公式字符串并求值,供 TreeCalculator/公式约束/启发使用。
//-------------------------Reference-------------------------
// 依赖第三方 muParserX(thirdparty/muparserx)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <muparserx/mpParser.h>
#include <muparserx/mpValue.h>
#include <unordered_map>
#include "variable.h"

namespace ECFlow {

    /**
    * @brief ȫ�������Ԫ��ӳ���
    * @details �洢������������Ӧ������������Ԫ������ӳ���ϵ��Ϊ����ʽ�����׶��ṩ������Ϸ���У������
    * @note keyΪ����������ַ�������"+"/"-"/"*"/"sin"/"sqrt"����valueΪԪ����1=һԪ�������2=��Ԫ�������0=�޲κ�����
    * @remark �ñ���Ϊextern������ʵ�ʶ�����calculation_tree.cpp���뵥Ԫ�У�ȫ��Ψһ
    */
    extern const std::unordered_map<std::string, int> g_OperatorArity;

    /**
     * @brief ECFlow��ܱ���ʽ��������װ��
     * @details ��muparserx����ж��η�װ�����Ⱪ¶�����"����-�����-����-ȡ���"���̣�
     *          �ڲ�ͨ�������洢����ά�����������������ⲿ����ָ��ʧЧ���µ�Ұָ�����⣬
     *          ������������顢����ȶ����ͱ���ʽ���㳡��
     * @note ������ѭ��Compile���� �� BatchLinkVariables����� �� Calculate���� �� getResult/getSizeȡ������ĵ���˳��
     */
    class ExpressionTree {
    public:
        /**
         * @brief ���캯��
         * @details ��ʼ��muparserx������ʵ��������Ĭ�Ͻ���������������/����֧�֡�ע���Զ������������
         *          ��ʼ���ڲ������洢����Ϊ��״̬
         */
        ExpressionTree();

        /**
         * @brief ��������
         * @details ʹ�ñ�����Ĭ�������߼������г�Ա������ΪRAII���ͣ�ParserX/Value/vector����
         *          �����ֶ��ͷ��ڴ���Դ
         */
        ~ExpressionTree() = default;

        /**
         * @brief ����ʽ����ӿ�
         * @details ���ַ�����ʽ����ѧ����ʽ����������Ϊmuparserx��ִ�еĽ������ṹ������﷨У����������
         * @param expr_text ���������ѧ����ʽ�ַ�����ʾ����"a + b*c"��"sum(arr1) + mat2[0][1]"��"sin(x) + cos(y)"��
         * @return bool - ��������true=����ɹ���false=����ʧ�ܣ�ԭ���﷨����/δ֪����/�Ƿ������/Ԫ����ƥ�䣩
         * @warning ����ʧ��ʱ�������ı����󶨺ͼ���ӿڲ��ɵ���
         */
        bool Compile(const std::string& expr_text);

        /**
         * @brief ���������󶨽ӿ�
         * @details ������ʽ�еı������ⲿdouble���������ڴ��ַ����������ʵ�ֽ��������ⲿ���ݵĻ�ͨ
         * @param notes ����Ԫ��Ϣ���飨ElementNote���ͣ���ÿ��Ԫ�ذ������������������ͣ�����/����/���󣩡�ά�ȵ���Ϣ
         * @param data_ptrs ��������ָ�����飬ÿ��ָ��ָ���Ӧ������double����������ʼ��ַ
         * @param var_count ��������������notes��data_ptrs�������Ч����һ��
         * @return bool - �󶨽����true=���б����󶨳ɹ���false=��ʧ�ܣ�ԭ�򣺱�����������/���Ͳ�ƥ��/���鳤�Ȳ�һ��/��ָ�룩
         * @note �󶨳ɹ��󣬱������ݻᱻ�������ڲ�m_variableStorage�����������������������һ��
         */
        bool BatchLinkVariables(const ElementNote* notes, const double** data_ptrs, const int var_count);

        /**
         * @brief ����ֵ��ʽ���½ӿ�
         * @details �����Ѱ󶨱���������Դָ�룬�����ⲿ�����ڴ��ַ��̬�仯�ĳ����������ݻ��������·��䣩
         * @param data_ptrs �µı�������ָ�����飬ָ��˳�������BatchLinkVariables�󶨵ı���˳����ȫһ��
         * @return bool - ���½����true=���³ɹ���false=����ʧ�ܣ�ԭ��ָ��������ƥ��/��ָ��/δִ�й������󶨣�
         * @warning ����ǰ������ִ��BatchLinkVariables��ɱ����󶨣�����ֱ�ӷ���false
         */
        bool UpdateVariables(const double** data_ptrs);

        /**
         * @brief ����ʽ����ӿ�
         * @details ִ���ѱ���ı���ʽ����ȡ�󶨱���������ֵ����ɼ��㲢�����������m_result��Ա����
         * @note �Զ��������/����/�������ͼ��㣬��������ɱ���ʽ�߼��ͱ������͹�ͬ����
         * @warning ����ǰ�������Compile��BatchLinkVariables������ᴥ��δ������Ϊ����������/���ݴ���
         */
        void Calculate();

        /**
         * @brief �����������ӿ�
         * @details ������ļ�������m_result���������ⲿָ����double���������У�֧�ֶ����ͽ����ͳһ����
         * @param result �ⲿ����洢����ָ�룬����ǰ�����㹻�ڴ棨�ڴ��С�ο�getSize()����ֵ��
         * @note ����ǰ������ִ��Calculate�����򵼳�����δ��ʼ������������
         */
        void getResult(double* result);

        /**
         * @brief ������Ȼ�ȡ�ӿ�
         * @details �������һ��Calculateִ�н����Ԫ��������Ϊ�ⲿ�����������ڴ��ṩ����
         * @return int - Ԫ�ظ�������������1�����鷵�����鳤�ȣ����󷵻���������������
         * @note ������Ƚ������һ�μ�������أ�����ִ��Calculate�������»�ȡ
         */
        int getSize();

        /**
        * @brief ��������������ӿ�
        * @details �����뵱ǰCalculationTreeʵ��״̬��ȫһ�µ���ʵ���������ѱ���ı���ʽ�������󶨹�ϵ��
        *          ��������á��������������ڲ�״̬��ʵ�����������������ǳ������
        * @return CalculationTree* - ָ���´�����CalculationTreeʵ����ָ��
        * @note 1. ���������ʵ����ԭʵ���໥�������޸�����һ���ı���ʽ/�����󶨲���Ӱ����һ����
        *       2. ���÷��踺���ͷŷ��ص�ָ���ڴ棨ʹ��delete���������ڴ�й©��
        *       3. ����ǰʵ��δִ��Compile/BatchLinkVariables���������ʵ��Ҳ����δ����/δ��״̬��
        *       4. ��������´����ڲ���mup::ParserXʵ���ͱ����洢������������Դ�������µķ��ʳ�ͻ
        * @warning ����ǰʵ��������Ч״̬�������ʧ�ܣ����������ʵ��Ҳ��̳и���Ч״̬
        */
        ExpressionTree* copy();

    private:
        /**
         * @brief ����muparserxֵ����������
         * @details ���ݱ���Ԫ��Ϣ������ָ�룬���ⲿdouble��������ת��Ϊmup::Value���ͣ�����muparserx����������ֵ��ʽ
         * @param note ����Ԫ��Ϣ��ElementNote��������ʶ���������ͣ�����/����/���󣩺�ά��
         * @param data_ptr ����������ʼ��ַ��double����ָ�룩
         * @return mup::Value - ������muparserxֵ���󣬿�ֱ�ӱ�������ʹ��
         */
        mup::Value BuildValueFromData(const ElementNote& note, double* data_ptr);

        /**
         * @brief �����Ѱ󶨱�����������
         * @details ����ڲ������洢����m_variableStorage�����ý������ı�����״̬�������ڴ�й©�����ݻ���
         * @note �ڲ�����ʱ�������±������ʽǰ����������ǰ
         */
        void clearLinked();


    private:
        mup::ParserX m_parser;                  // muparserx���Ľ�����ʵ�����������ʽ���롢��������������ִ��
        mup::Value m_result;      // ���������棬�洢���һ��Calculate()��ִ�н����֧�ֱ���/����/����
        std::vector<mup::Value> m_variableStorage;// ���ڴ洢���б������ڲ�����, ˳���� LinkVariable ��˳�򱣳�һ��
        std::vector<std::string> m_variableName;// ���ڴ洢���б���������, ˳���� LinkVariable ��˳�򱣳�һ��
    };

} // namespace ECFlow