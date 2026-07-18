//------------------------Description------------------------
// eccalcul_functor:计算器用的抽象仿函数基类(虚 operator() + copy),供模板携带局部状态。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include<functional>

namespace ECFlow
{
	// ��Ҫ����ʲô��
	// ������ģ���У���Ϊ��������ĺ���������ȫ�֡���̬��
	// �������ᵼ��ȫ�ֻ�����ֻ�ܴ���һ��ͬ������
	// ������ֻ��һ�����ظ��Ķ���ᵼ�´���
	// ����ܲ���ͨ������ʹͬһ������ģ��Ĳ�ͬʾ����ʵ�����ݡ���������������ʽ�����ȵķ���
	// 
	// ʵ��˼·��
	// ���ڰ�������ָ��ķº��������еģ��º���֮��ֻʵ�������ݵķ��룬������Ҫ���뺯��ָ�룿
	// ��������ָ�룬ֱ���ڷº�����д��
	// �������º������Ͳ�ͬ����ô�����أ�
	// �ڻ���ģ������һ���º����Ļ��࣬�����Ķ�������̳�
	// 
	// ����ϸ�ڣ�
	// ������ģ�廹��һ����ͨ�����أ�ģ��ɣ����������ͳһ
	// ���ˣ�ģ���м��
	// 
	// ���Ծ��к������������
	// �������û��������Ϊֱ����STL��ģ������
	// ���������������ΪSTL��ģ��̫������
	// 
	//
		/*
		// һԪ�º���
		template <class _Arg, class _Result>
		struct unary_ecfunctor : std::unary_function<_Arg, _Result>
		{
			unary_ecfunctor() {}

			virtual _Result operator()(const _Arg& a) const = 0;

			virtual unary_ecfunctor* copy() = 0;
		};
		*/

		// ��calculatorʹ�õķº���
	struct eccalcul_functor
		// struct eccalcul_functor
	{
		eccalcul_functor() {}

		virtual ~eccalcul_functor()
		{

		}

		virtual double operator()(double** a) const = 0;

		virtual eccalcul_functor* copy() = 0;
	};

}