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

double heuristic(double** input)
{
	return -1 * abs(input[1][0]);
}

int main()
{
	// 问题定义
	ECFC::Problem problem("test_problem");
	// 添加变量
	problem.addVariable("x", -5, 5, 0.001, 5);
	// 添加目标函数，f1最小化，f2最大化
	problem.addObjective("f1", 1, true, "x", f1);
	// 添加启发函数，x绝对值的负值,y随机启发式（默认，可以缺省设置）
	problem.addInspirationFunc("x", "x", heuristic);
	// 添加约束
	problem.addConstrainUnique("x", 1);
	problem.addConstrainRange("x", ECFC::EMPTYVALUE, ECFC::EMPTYVALUE, 1);

	// 问题编译
	ECFC::ProblemHandle* problem_handle = problem.compile();

	// 优化器构建
	ECFC::OptimizerBuilder builder;
	builder.setName("test_optimizer");
	builder.setIndividual(ECFC::IndividualType::F_individual);
	builder.setArchive(ECFC::BestArchiveType::F_normal);
	builder.setTerminateMAXFES(1e3);
	// builder.setTerminateMAXTime(1);
	// builder.setTerminateMAXStop(100);

	// 当前测试建立在单种群基础上
	builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);
	// builder.setSwarmConstruct(ECFC::SubswarmConstructerType::F_fix);
	// builder.setSwarmTopology(ECFC::SubswarmTopologyType::F_connected);

	// 添加种群
	ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
	sp_setter->setSwarmSize(20);
	sp_setter->setSolutionIni(ECFC::InitializerType::F_random);
	sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.point(), sp_setter->lstrategy.mutation.bit());
	sp_setter->ltopology.championship();
	sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);
	sp_setter->setRepairMethod(ECFC::RepairType::F_random);
	sp_setter->setSelector(ECFC::SelectorType::F_index, false);
	sp_setter->setTerminateMAXFES(1e3);
	// sp_setter->setTerminateMAXTime(1);
	// sp_setter->setTerminateMAXStop(100);
	sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

	// 日志设置
	builder.setLoggerFull(false);
	builder.setLoggerProcess(false);
	builder.saveConfigure();

	return 0;
}