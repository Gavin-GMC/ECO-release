#include"logger.h"
#include"ecfc.h"

long long int buffer;

double f1(double** a)
{
	double back = 0;
	for (int i = 0; i < 5; i++)
	{
		back += a[0][i] * a[0][i];
	}
	return back;
}

double f2(double** a)
{
	double back = 0;
	for (int i = 0; i < 5; i++)
	{
		back += a[0][i];
		back += a[1][i];
	}
	return back;
}

double f3(double** a)
{
	return 0;
}

double heuristic(double** input)
{
	return -1 * abs(input[1][0]);
}

// 问题模块测试内容
// 变量添加，ok
// 目标添加，ok
// 候选解评估，ok
// 候选解比较，ok
// 约束添加，ok
// 约束检查，ok
// 约束传播，ok
// 约束违反程度计算，含变量取值范围，ok
// 目标计算中加入约束惩罚，ok
// 可行域生成，ok
// 启发式添加，ok
// 启发式计算，ok
// 启发式排序，ok
// 启发式最优，ok
//

int main()
{
	// 问题定义
	ECFC::Problem problem("test_problem");
	// 添加变量
	problem.addVariable("x", -5, 5, 0.001, 5);
	problem.addVariable("y", -5, 5, 0.001, 5);
	// 添加目标函数，f1最小化，f2最大化
	problem.addObjective("f1", 1, true, "x", f1);
	problem.addObjective("f2", 1, false, "x,y", f2);
	problem.addObjective("f3", 0, true, "x", f3);
	problem.addObjective("f4", 0, true, "x", f3);
	// 添加启发函数，x绝对值的负值,y随机启发式（默认，可以缺省设置）
	problem.addInspirationFunc("x", "x", heuristic);
	problem.addInspirationRandom("y");
	// 添加约束
	problem.addConstrainUnique("x", 1, "f3");
	problem.addConstrainMinDistance("y", 0.1, 2, "f3");
	problem.addConstrainRange("x", ECFC::EMPTYVALUE, ECFC::EMPTYVALUE, 1, "f4");

	// 问题编译
	ECFC::ProblemHandle* problem_handle = problem.compile();

	// 测试解构建
	ECFC::Solution s1;
	s1.setSize(problem_handle->getProblemSize(), problem_handle->getObjectNumber());
	s1.setDecoder(problem_handle->getSolutionDecoder());
	ECFC::Solution s2;
	s2.setSize(problem_handle->getProblemSize(), problem_handle->getObjectNumber());
	s2.setDecoder(problem_handle->getSolutionDecoder());
	double s1_result[10] = { 0,1,2,2,4,5,6,7,8,9 };
	double s2_result[10] = { 5,6,7,8,9,0,0.1,0.3,0.2,-0.1 };
	for (int i = 0; i < 10; i++)
	{
		s1[i] = s1_result[i];
		s2[i] = s2_result[i];
	}

	// 测试解评估
	problem_handle->solutionEvaluate(s1);
	problem_handle->solutionEvaluate(s2);

	// 测试解比较
	ECFC::Comparer* comparer = problem_handle->getSolutionComparer();
	bool c_result = comparer->isBetter(s1.fitness, s2.fitness);

	// 约束违反程度计算
	double penalty1 = problem_handle->constraintViolation(s1);
	double penalty2 = problem_handle->constraintViolation(s2);
	// 约束模块、启发式模块测试
	double priori_order[5];
	int problem_size = problem_handle->getProblemSize();
	problem_handle->constrainReset();
	problem_handle->setResult(s1);
	for (int i = 0; i < problem_size; i++)
	{
		// 可行域生成
		double* feasible_region = problem_handle->getFeasibleList(i);

		// 获得最优解、最优解对应启发式值和启发式排序
		double priori_choice = problem_handle->getPrioriChoice(i);
		double heuristic_info = problem_handle->getChoiceHeuristic(i, priori_choice);
		problem_handle->getPrioriChoice(i, 5, priori_order);

		// 约束检查
		bool check = problem_handle->constrainCheck(i, priori_choice);
		if (check)
		{
			s1[i] = priori_choice;
			// 约束传播
			problem_handle->constrainChange(i, s1[i]);
		}

		delete[] feasible_region;
	}


	return 0;
}