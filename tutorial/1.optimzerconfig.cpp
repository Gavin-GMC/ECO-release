#include"ecfc.h"

int main()
{
	// 优化器构建
	ECFC::OptimizerBuilder builder;
	builder.setName("GA");
	builder.setIndividual(ECFC::IndividualType::F_individual);
	builder.setArchive(ECFC::BestArchiveType::F_normal);
	builder.setTerminateMAXFES(1e5);
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
	sp_setter->setTerminateMAXFES(1e5);
	// sp_setter->setTerminateMAXTime(1);
	// sp_setter->setTerminateMAXStop(100);
	sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

	// 日志设置
	builder.setLoggerFull(false);
	builder.setLoggerProcess(false);
	builder.saveConfigure();

	ECFC::Optimizer* optimizer = builder.build();

	builder.clearConfigiure();
	
	// 从已有配置中载入
	builder.loadConfigure("GA");
	ECFC::Optimizer* optimizer1 = builder.build();

	return 0;
}