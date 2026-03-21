#include"ecfc.h"

int main()
{
	// 优化器构建
	ECFC::OptimizerBuilder builder;

	builder.setName("GA_point_bit");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.point(1, 0.9), sp_setter->lstrategy.mutation.bit(0.01));

		sp_setter->ltopology.championship(2, 1);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	    builder.setName("GA_SBX_PM");
    {
        builder.setIndividual(ECFC::IndividualType::F_individual);
        builder.setArchive(ECFC::BestArchiveType::F_normal);
        builder.setTerminateMAXFES(1e5);

        // 当前测试建立在单种群基础上
        builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

        // 添加种群
        ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
        sp_setter->setSwarmSize(100);

        sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

        sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.SBX(20, 1), sp_setter->lstrategy.mutation.PM(20, 0.01));

        sp_setter->ltopology.championship(2, 1);

        sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

        sp_setter->setRepairMethod(ECFC::RepairType::F_boundary);

        sp_setter->setSelector(ECFC::SelectorType::F_rank, false);

        sp_setter->setTerminateMAXFES(1e5);
        sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

        // 日志设置
        builder.setLoggerFull(false);
        builder.setLoggerProcess(true);

        builder.saveConfigure(true);

        builder.clearConfigiure();
    }

	builder.setName("GA_no_turnover");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.no(), sp_setter->lstrategy.mutation.overturn(1, 1));

		sp_setter->ltopology.championship(2, 1);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("GA_uniform_gauss");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.uniform(0.9), sp_setter->lstrategy.mutation.gauss(3, 0.01));

		sp_setter->ltopology.championship(2, 1);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("GA_uniform_turnover");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.uniform(0.9), sp_setter->lstrategy.mutation.overturn(1, 1));

		sp_setter->ltopology.championship(2, 1);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("GA_no_exchange");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.no(), sp_setter->lstrategy.mutation.exchange(5, 1));

		sp_setter->ltopology.championship(2, 1);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("GA_partialmap_exchange");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.partialMap(0.9), sp_setter->lstrategy.mutation.exchange(5, 0.1));

		sp_setter->ltopology.championship(2, 1);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("GA_cycle_insert");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.cycle(0.9), sp_setter->lstrategy.mutation.insert(5, 0.1));

		sp_setter->ltopology.championship(2, 1);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("GA_order_turnover");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.order(0.9), sp_setter->lstrategy.mutation.overturn(5, 0.1));

		sp_setter->ltopology.championship(2, 1);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("GA_subtourExchange_insert");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.subtourExchange(0.9), sp_setter->lstrategy.mutation.insert(5, 0.1));

		sp_setter->ltopology.championship(2, 1);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("GA_subtourExchange_no");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.subtourExchange(0.9), sp_setter->lstrategy.mutation.no());

		sp_setter->ltopology.championship(2, 1);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("GA_positionBased_exchange");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.positionBased(0.9, 0.1), sp_setter->lstrategy.mutation.exchange(5, 0.1));

		sp_setter->ltopology.championship(2, 1);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("EDA");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.EDA(ECFC::DistributionType::F_Gaussian, ECFC::EMPTYVALUE, 0.5);

		sp_setter->ltopology.isolate();

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("DE");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.DE(sp_setter->lstrategy.crossover.difference(0.5, 0.5));

		sp_setter->ltopology.random(2);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("AS");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(10);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.AS(1, 2, 0.5);

		sp_setter->ltopology.isolate();

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_construct_order);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("ACS");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(10);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.ACS(1, 2, 0.1, 0.1, 0.9);

		sp_setter->ltopology.isolate();

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_construct_parallel);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("PSO");
	{
		builder.setIndividual(ECFC::IndividualType::F_particle);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.PSO(2, 2, 0.9, 0.5 * 100 / 1e5);

		sp_setter->ltopology.PSO();

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_construct_order);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

    builder.setName("CSO");
    {
        builder.setIndividual(ECFC::IndividualType::F_particle);
        builder.setArchive(ECFC::BestArchiveType::F_normal);
        builder.setTerminateMAXFES(1e5);

        // 当前测试建立在单种群基础上
        builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

        // 添加种群
        ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
        sp_setter->setSwarmSize(100);

        sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

        double c[2] = { 1, 0.1 };
        sp_setter->lstrategy.PSO(2, c, 0.9, ECFC::EMPTYVALUE);

        sp_setter->ltopology.CSO();

        sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_construct_order);

        sp_setter->setRepairMethod(ECFC::RepairType::F_boundary);

        sp_setter->setSelector(ECFC::SelectorType::F_index, false);

        sp_setter->setTerminateMAXFES(1e5);
        sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

        // 日志设置
        builder.setLoggerFull(false);
        builder.setLoggerProcess(true);

        builder.saveConfigure(true);

        builder.clearConfigiure();
    }

	builder.setName("LLSO");
	{
		builder.setIndividual(ECFC::IndividualType::F_particle);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.PSO(2, 2, 0.9, ECFC::EMPTYVALUE);

		sp_setter->ltopology.LLSO(4);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_construct_order);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, true);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("SDLSO");
	{
		builder.setIndividual(ECFC::IndividualType::F_particle);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.PSO(2, 2, 0.9, ECFC::EMPTYVALUE);

		sp_setter->ltopology.SDLSO();

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_construct_order);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, true);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("SPSO");
	{
		builder.setIndividual(ECFC::IndividualType::F_set_particle);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.SetPSO(2, 2, 0.9, true, true, 0.5 * 100 / 1e5);

		sp_setter->ltopology.PSO();

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_construct_order);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("SLLSO");
	{
		builder.setIndividual(ECFC::IndividualType::F_set_particle);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.SetPSO(2, 2, 0.9, true, true, 0.5 * 100 / 1e5);

		sp_setter->ltopology.LLSO(4);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_construct_order);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("SSDLSO");
	{
		builder.setIndividual(ECFC::IndividualType::F_set_particle);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.SetPSO(2, 2, 0.9, true, true, 0.5 * 100 / 1e5);

		sp_setter->ltopology.SDLSO();

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_construct_order);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("GA_no_turnover_1");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.no(), sp_setter->lstrategy.mutation.overturn(1, 1));

		sp_setter->ltopology.roulrtte(1);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("GA_no_turnover_2");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.no(), sp_setter->lstrategy.mutation.overturn(2, 1));

		sp_setter->ltopology.roulrtte(1);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("GA_no_turnover_3");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.no(), sp_setter->lstrategy.mutation.overturn(5, 1));

		sp_setter->ltopology.roulrtte(1);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_greedy);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("GA_uniform_turnover_1");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.uniform(0.01), sp_setter->lstrategy.mutation.overturn(1, 1));

		sp_setter->ltopology.championship(2, 1);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_greedy);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("GA_uniform_turnover_2");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_greedy);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.uniform(0.01), sp_setter->lstrategy.mutation.overturn(1, 1));

		sp_setter->ltopology.championship(2, 1);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_greedy);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("GA_uniform_turnover_3");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.uniform(0.01), sp_setter->lstrategy.mutation.overturn(1, 1));

		sp_setter->ltopology.championship(2, 1);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_greedy);

		sp_setter->setSelector(ECFC::SelectorType::F_index, true);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("GA_partialmap_exchange_1");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.partialMap(0.9, false), sp_setter->lstrategy.mutation.exchange(5, 0.1));

		sp_setter->ltopology.championship(2, 1);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("GA_cycle_insert_1");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.cycle(0.9), sp_setter->lstrategy.mutation.insert(1, 1));

		sp_setter->ltopology.championship(2, 1);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("GA_order_turnover_1");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.order(0.9), sp_setter->lstrategy.mutation.overturn(1, 0.01));

		sp_setter->ltopology.championship(2, 1);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("GA_positionBased_exchange_1");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.positionBased(0.9, 0.01), sp_setter->lstrategy.mutation.exchange(5, 0.1));

		sp_setter->ltopology.championship(2, 1);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("GA_point_bit_1");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_greedy);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.point(1, 0.9), sp_setter->lstrategy.mutation.bit(0.01));

		sp_setter->ltopology.championship(2, 1);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("GA_point_bit_2");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.point(1, 0.9), sp_setter->lstrategy.mutation.bit(0.01));

		sp_setter->ltopology.championship(2, 1);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_greedy);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("GA_point_bit_3");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.point(1, 0.9), sp_setter->lstrategy.mutation.bit(0.01));

		sp_setter->ltopology.championship(2, 1);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_greedy);

		sp_setter->setSelector(ECFC::SelectorType::F_rank, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("GA_SBX_PM_1");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.SBX(1, 0.9), sp_setter->lstrategy.mutation.PM(20, 0.01));

		sp_setter->ltopology.championship(2, 1);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_greedy);

		sp_setter->setSelector(ECFC::SelectorType::F_index, true);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("AS_1");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(10);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.AS(1, 1, 0.5);

		sp_setter->ltopology.isolate();

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_construct_parallel);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("ACS_1");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(10);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.ACS(1, 2, 0.5, 0.5, 0.9);

		sp_setter->ltopology.isolate();

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_construct_parallel);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("ACS_2");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(10);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.ACS(1, 2, 0.1, 0.1, 0.9);

		sp_setter->ltopology.isolate();

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_construct_order);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("ACS_3");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(10);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.ACS(2, 1, 0.1, 0.1, 0.9);

		sp_setter->ltopology.isolate();

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_construct_parallel);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("SPSO_1");
	{
		builder.setIndividual(ECFC::IndividualType::F_set_particle);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.SetPSO(2, 2, 0.9, true, false, 0.5 * 100 / 1e5);

		sp_setter->ltopology.PSO();

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_construct_order);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("SPSO_2");
	{
		builder.setIndividual(ECFC::IndividualType::F_set_particle);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.SetPSO(2, 2, 0.9, false, true, 0.5 * 100 / 1e5);

		sp_setter->ltopology.PSO();

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_construct_order);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("SPSO_3");
	{
		builder.setIndividual(ECFC::IndividualType::F_set_particle);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.SetPSO(2, 2, 0.9, false, false, 0.5 * 100 / 1e5);

		sp_setter->ltopology.PSO();

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_construct_order);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("SLLSO_1");
	{
		builder.setIndividual(ECFC::IndividualType::F_set_particle);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.SetPSO(2, 2, 0.9, true, true, 0.5 * 100 / 1e5);

		sp_setter->ltopology.LLSO(4);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_construct_order);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, true);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("SLLSO_2");
	{
		builder.setIndividual(ECFC::IndividualType::F_set_particle);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.SetPSO(2, 2, 0.9, true, true, ECFC::EMPTYVALUE);

		sp_setter->ltopology.LLSO(4);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_construct_order);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, true);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("SLLSO_3");
	{
		builder.setIndividual(ECFC::IndividualType::F_set_particle);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.SetPSO(2, 2, 0.9, true, true, ECFC::EMPTYVALUE);

		sp_setter->ltopology.LLSO(10);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_construct_order);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_rank, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("SLLSO_4");
	{
		builder.setIndividual(ECFC::IndividualType::F_set_particle);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.SetPSO(2, 2, 0.9, true, true, ECFC::EMPTYVALUE);

		sp_setter->ltopology.LLSO(4);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_construct_order);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, true);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("SSDLSO_1");
	{
		builder.setIndividual(ECFC::IndividualType::F_set_particle);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.SetPSO(2, 2, 0.9, true, true, ECFC::EMPTYVALUE);

		sp_setter->ltopology.SDLSO();

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_construct_order);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, true);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("CS+GA");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.uniform(0.01), sp_setter->lstrategy.mutation.overturn(1, 1));

		sp_setter->ltopology.CSO();

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_greedy);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("LL+GA");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.cycle(0.9), sp_setter->lstrategy.mutation.insert(5, 1));

		sp_setter->ltopology.LLSO(4);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_greedy);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("SDL+GA");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.GA(sp_setter->lstrategy.crossover.partialMap(0.9, false), sp_setter->lstrategy.mutation.exchange(5, 0.1));

		sp_setter->ltopology.SDLSO();

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_greedy);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("LL+ACS");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(10);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.ACS(1, 2, 0.1, 0.1, 0.9);

		sp_setter->ltopology.LLSO(4);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_construct_parallel);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("random+SPSO");
	{
		builder.setIndividual(ECFC::IndividualType::F_set_particle);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.SetPSO(2, 2, 0.9, true, true, 0.5 * 100 / 1e5);

		sp_setter->ltopology.random(2);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_construct_order);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("roulrtte+SPSO");
	{
		builder.setIndividual(ECFC::IndividualType::F_set_particle);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.SetPSO(2, 2, 0.9, true, true, 0.5 * 100 / 1e5);

		sp_setter->ltopology.roulrtte(2);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_construct_order);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("championship+SPSO");
	{
		builder.setIndividual(ECFC::IndividualType::F_set_particle);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.SetPSO(2, 2, 0.9, true, true, 0.5 * 100 / 1e5);

		sp_setter->ltopology.championship(2, 2);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_construct_order);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("randomConfig_1");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(10);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_random);

		sp_setter->lstrategy.ACS(1, 2, 0.1, 0.1, 0.9);

		sp_setter->ltopology.LLSO(4);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_greedy);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("randomConfig_2");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_greedy);

		sp_setter->lstrategy.EDA(ECFC::DistributionType::F_Gaussian, ECFC::EMPTYVALUE, 0.2);

		sp_setter->ltopology.SDLSO();

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_construct_order);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_index, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("randomConfig_3");
	{
		builder.setIndividual(ECFC::IndividualType::F_particle);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_greedy);

		sp_setter->lstrategy.PSO(2, 2, 0.9, 0.5 * 100 / 1e5);

		sp_setter->ltopology.championship(2, 2);

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_close, true);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	builder.setName("randomConfig_4");
	{
		builder.setIndividual(ECFC::IndividualType::F_individual);
		builder.setArchive(ECFC::BestArchiveType::F_normal);
		builder.setTerminateMAXFES(1e5);

		// 当前测试建立在单种群基础上
		builder.setSwarmManager(ECFC::SubswarmManagerType::F_single);

		// 添加种群
		ECFC::SubpopulationSetter* sp_setter = builder.addSubpopulation("1");
		sp_setter->setSwarmSize(100);

		sp_setter->setSolutionIni(ECFC::InitializerType::F_greedy);

		sp_setter->lstrategy.EDA(ECFC::DistributionType::F_Gaussian, ECFC::EMPTYVALUE, 0.2);

		sp_setter->ltopology.roulrtte();

		sp_setter->setLFramework(ECFC::OffspringGeneratorType::F_generation_nocheck);

		sp_setter->setRepairMethod(ECFC::RepairType::F_random);

		sp_setter->setSelector(ECFC::SelectorType::F_rank, false);

		sp_setter->setTerminateMAXFES(1e5);
		sp_setter->setArchive(ECFC::BestArchiveType::F_normal);

		// 日志设置
		builder.setLoggerFull(false);
		builder.setLoggerProcess(true);

		builder.saveConfigure(true);

		builder.clearConfigiure();
	}

	return 0;
}
