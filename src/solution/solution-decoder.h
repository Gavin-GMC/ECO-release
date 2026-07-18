//------------------------Description------------------------
// This file defines the solution decoder, which provides a
//  correspondence between the result vector and the variables.
//-------------------------Copyright-------------------------
// Copyright (c) 2024 ���������ƣ���ȷ�ϣ�, All Rights Reserved.
// You are free to use the ECFlow����ȷ�ϣ� for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference 
// "δȷ��"
//-----------------------------------------------------------

#pragma once
#include<string>
#include<vector>

namespace ECFlow
{
    struct ElementNote;
    class Objective;

    /**
     * @struct variableMapTable
     * @brief ��ͬ�ı�����ʽ�½�ı���ӳ���
     * @details ��װ�˴�ԭ���뵽�±���ı���ӳ���ϵ�����������Ǩ��
     */
    struct variableMapTable
    {
        std::vector<int> begin_index;
        std::vector<int> map_index;
        std::vector<int> map_length;
    };

    /**
     * @struct SolutionDecoder
     * @brief ����������������Ľṹ��
     * @details ��װ�˱���ע����Ϣ��ElementNote����Ŀ�꺯�����ƣ�
     *          �ṩ����������ɶ��ַ�����ת�����ܣ�ʹ��std::vector������̬���ݣ�
     *          �����ֶ������ڴ棨vector�Զ�ά���������ڣ���
     */
    struct SolutionDecoder
    {
    private:
        /**
         * @var notes
         * @brief ����ע����Ϣ������
         * @details �洢ÿ��������Ԫ��Ϣ�������ơ����ȵȣ������ȵ��ڱ���������
         *          �ɹ��캯����ʼ����˽�г�Ա����ͨ��getVariableNumber()��ӷ��ʳ��ȡ�
         */
        std::vector<ElementNote> notes;

        /**
         * @var solution_size
         * @brief ���ά��
         * @details �洢��ѡ�����ά����
         */
        int solution_size;

        /**
         * @var object_name
         * @brief Ŀ�꺯�����Ƶ�����
         * @details �洢ÿ��Ŀ�꺯���������ַ��������ȵ���Ŀ�꺯��������
         *          �ɹ��캯����Objective��������ȡ���Ƴ�ʼ����
         *          ˽�г�Ա����ͨ��getObjectNumber()��ӷ��ʳ��ȡ�
         */
        std::vector<std::string> object_name;

    public:
        /**
         * @brief ���캯��
         * @details ��ʼ������ע����Ϣ��Ŀ�꺯������������
         *          1. �ӱ������鿽��ElementNote��notes������
         *          2. ��Objective������ȡ���Ƶ�object_name������
         * @param variables ElementNote���͵ı�������ָ�루�ǿգ�
         * @param variable_number ������������������
         * @param objectives Objective���͵�Ŀ�꺯������ָ�루�ǿգ�
         * @param object_number Ŀ�꺯����������������
         * @warning �����ָ�����������ᵼ��δ������Ϊ����������ʼ���쳣��
         */
        SolutionDecoder(ElementNote* variables, int variable_number, Objective* objectives, int object_number);

        /**
         * @brief ��������
         * @details ��������������Ϊ��Ա������std::vector���Զ��ͷ��ڴ棩��
         *          �����ֶ��ͷ���Դ��
         */
        ~SolutionDecoder();

        /**
         * @brief ��ȡ��������
         * @return int - notes�����ĳ��ȣ��������������Ǹ�������
         */
        int getVariableNumber();

        /**
         * @brief ��ȡ��ά��
         * @return int - ���б�������ά��
         */
        int getSolutionSize();

        /**
         * @brief ��ȡĿ�꺯������
         * @return int - object_name�����ĳ��ȣ���Ŀ�꺯���������Ǹ�������
         */
        int getObjectNumber();

        // —— 优化器层所需的只读访问器(补回重构中被封装/删除的接口;供 SetParticle / lstrategy-aco 等)——
        // getNote:第 i 个决策变量的注记(原 public `notes[i]`,重构后 notes 转私有)。
        const ElementNote& getNote(int i) const;
        // getVariableSize:第 i 个变量的元素个数(恢复原 `variable_sizes[i]` = 变量 getLength();等于 notes[i]._length)。
        int getVariableSize(int i) const;
        // getObjectName:第 j 个目标名(原 public `object_name[j]`)。
        const std::string& getObjectName(int j) const;

        /**
         * @brief ��ȡԭ��������ǰ�����ӳ���ϵ
         * @return variableMapTable - old�������½⵽��ǰ�������ı���ӳ���
         */
        variableMapTable getMap(const SolutionDecoder* old);

        /**
         * @brief ����ֵ�����ʽ��Ϊ�ɶ��ַ���
         * @details ֧���������ģʽ��
         *          - ���ģʽ��full_print=false�������������Ŀ�꺯�����ƺͶ�Ӧfitnessֵ��
         *          - ����ģʽ��full_print=true�������Ŀ�꺯��+���б��������ƺͶ�Ӧresultֵ��
         * @param result ������ֵ�������ָ�루�ǿգ�������������б���_length֮�ͣ�
         * @param fitness Ŀ�꺯����Ӧ��ֵ����ָ�루�ǿգ����������Ŀ�꺯��������
         * @param full_print �Ƿ����������Ϣ��Ĭ��false�����ģʽ��
         * @return std::string ��ʽ����Ŀɶ��ַ���
         * @warning �����ָ������鳤�Ȳ�ƥ��ᵼ���ڴ�Խ�������쳣
         * @note ��ֵ�������Ϊ15λ��ʹ��Ĭ�ϸ����ʽ��std::defaultfloat��
         */
        std::string toString(double* result, double* fitness, bool full_print = false);
    };
}