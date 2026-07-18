#include <iostream>
#include <sstream>
#include <iomanip>

#include"variable.h"
#include"objective.h"

#include"solution.h"

using namespace ECFlow;

// ���캯��ʵ��
SolutionDecoder::SolutionDecoder(ElementNote* variables, int variable_number, Objective* objectives, int object_number)
{
    // ��ʼ������ע������
    solution_size = 0;

    notes.resize(variable_number);
    for (int i = 0; i < variable_number; i++)
    {
        notes[i] = variables[i];
        solution_size += notes[i]._length;
    }

    // ��ʼ��Ŀ�꺯����������
    object_name.resize(object_number);
    for (int i = 0; i < object_number; i++)
        object_name[i] = objectives[i].getName();
}

// ��������ʵ�֣���ʵ�֣�
SolutionDecoder::~SolutionDecoder()
{

}

// ��ȡ��������
int SolutionDecoder::getVariableNumber()
{
    return notes.size();
}

// ��ȡ������ά��
int SolutionDecoder::getSolutionSize()
{
    return solution_size;
}

// ��ȡĿ�꺯������
int SolutionDecoder::getObjectNumber()
{
    return object_name.size();
}

// —— 优化器层只读访问器(补回原 public notes/variable_sizes/object_name)——
const ElementNote& SolutionDecoder::getNote(int i) const { return notes[i]; }
int SolutionDecoder::getVariableSize(int i) const { return notes[i]._length; }   // = 原 variable_sizes[i]
const std::string& SolutionDecoder::getObjectName(int j) const { return object_name[j]; }

variableMapTable SolutionDecoder::getMap(const SolutionDecoder* old)
{
    variableMapTable back;
    int begin = 0;
    for (int i = 0; i < notes.size(); i++)
    {
        back.begin_index.push_back(begin);
        back.map_length.push_back(notes[i]._length);
        begin += notes[i]._length;

        int index = 0;
        int result_index = -1;
        for (int j = 0; j < old->notes.size(); j++)
        {
            if (notes[i] == old->notes[j])
            {
                result_index = index;
                break;
            }
            index += old->notes[j]._length;
        }
        back.map_index.push_back(result_index);
    }

    return back;
}

// �����ʽ������ʵ�֣�Ĭ�ϲ������ظ�������
std::string SolutionDecoder::toString(double* result, double* fitness, bool full_print)
{
    std::ostringstream oss;

    if (full_print)
    {
        // ���Ŀ�꺯�����ƺ���Ӧ��ֵ
        for (int j = 0; j < getObjectNumber(); j++)
        {
            oss << object_name[j] + ":\t";
            oss << std::to_string(fitness[j]) + "\t";
        }
        oss << "\n";

        int index = 0;
        // �������ӡ���ƺ���ֵ
        for (int vid = 0; vid < getVariableNumber(); vid++)
        {
            oss << "v" + std::to_string(vid + 1) + "-\t" + notes[vid]._name + ":";
            for (int did = 0; did < notes[vid]._length; did++)
            {
                oss << "\t" << std::defaultfloat << std::setprecision(15) << result[index];
                index++;
            }
            oss << "\n";
        }
        oss << "EOS";
    }
    else
    {
        // �����Ŀ�꺯�����ƺ���Ӧ��ֵ
        for (int j = 0; j < getObjectNumber(); j++)
        {
            oss << object_name[j] + ":\t";
            oss << std::to_string(fitness[j]) + "\t";
        }
    }

    return oss.str();
}