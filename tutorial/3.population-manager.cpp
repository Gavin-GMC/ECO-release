/*#include"logger.h"
#include"ecfc.h"

long long int buffer;

double func(double** a)
{
	double back = 0;
	for (int i = 0; i < 5; i++)
	{
		back += a[0][i] * a[0][i];
	}
	return back;
}

double heuristic(double** input)
{
	return abs(input[0][0]);
}


int main()
{
	// 问题定义
	ECFC::Problem problem("test_problem");
	problem.addVariable("x", -5, 5, 0.001, 5);
	problem.addObjective("f1", 1, true, "x", func);
	problem.addInspirationFunc("x", "x", heuristic);

	ECFC::ProblemHandle* handler = problem.compile();

	// 逐级构建基本GA进行框架测试
	ECFC::OptimizerBuilder builder;
	builder.setName("test_optimizer");
	builder.setIndividual(ECFC::IndividualType::F_individual);
	builder.setSolutionIni(ECFC::InitializerType::F_random);
	builder.lstrategy.GA(builder.lstrategy.crossover.point(), builder.lstrategy.mutation.bit());
	builder.ltopology.championship();
	builder.setLFramework(ECFC::OffspringGeneratorType::F_generation);
	builder.setSelector(ECFC::SelectorType::F_index, false);
	builder.setArchive(ECFC::BestArchiveType::F_normal);

	builder.setSwarmNumber(1);
	builder.setSwarmSize(20);
	builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);
	// builder.setSwarmConstruct(ECFC::SubswarmConstructerType::F_fix);
	// builder.setSwarmTopology(ECFC::SubswarmTopologyType::F_connected);
	builder.setTerminateMAXFES(1e3);
	// builder.setTerminateMAXTime(1);
	// builder.setTerminateMAXStop(100);
	builder.setLoggerFull(false);
	builder.setLoggerProcess(true);

	ECFC::Optimizer* optimizer = builder.build();
	optimizer->setProblem(&problem);

	optimizer->exe();
	optimizer->logResult();


	return 0;
}
*/

///
/// 指标计算与输出
/// 种群添加形式
/// 断言系统
/// 日志器针对过程和结果差异设置
///