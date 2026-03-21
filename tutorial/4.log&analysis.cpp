#include"ecfc.h"

int main()
{
	ECFC::ECAnalyzer analyzer;
	std::vector<std::string> optimizerList = { "ACS", "LL+GA",  "CS+GA" };
	std::vector<std::string> problemList = { "a280", "att48", "att532", "kroA100"};

	for (int i = 0; i < optimizerList.size(); i++)
		analyzer.addOptimizer(optimizerList[i], "tsp", 20);
	for (int i = 0; i < problemList.size(); i++)
		analyzer.addProblem(problemList[i]);

	double** paras = new double* [problemList.size()];
	for (int i = 0; i < problemList.size(); i++)
	{
		paras[i] = new double[1 + 1];
		paras[i][0] = 1;
		paras[i][1] = 0;
	}

	analyzer.setMetric(ECFC::ECAnalyzer::MetricType::mean, paras);
	analyzer.addStatisticalTool(ECFC::ECAnalyzer::MetricType::mean);
	analyzer.addStatisticalTool(ECFC::ECAnalyzer::MetricType::std);
	analyzer.addSignificanceTest();
	analyzer.addBestStatistic();
	analyzer.addRankStatistic();
	analyzer.addWinLoseStatistic();

	analyzer.cal();

	analyzer.printToLatex("test");
}