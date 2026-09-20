// 7.external-benchmark.cpp - connect an independently supplied C/C++ benchmark.
// Build this project-owned demonstration with:
//   .\build.ps1 tutorial\7.external-benchmark.cpp -Extra tutorial\external-benchmark\example-benchmark.cpp
// Replace the example header/source with the benchmark acquired by the user and
// link it in the user's host project. Do not copy third-party benchmark code into EvoFlow.

// SPDX-License-Identifier: BSD-3-Clause

#include <cmath>
#include <cstdio>
#include "evoflow.h"
#include "external-benchmark/example-benchmark.h"

using namespace EvoFlow;

class ExternalObjective final : public eccalcul_functor
{
public:
    ExternalObjective(int dimension, double shift)
        : _dimension(dimension), _context(ExternalExample::create(dimension, shift)) {}

    ExternalObjective(const ExternalObjective& source)
        : _dimension(source._dimension), _context(ExternalExample::clone(source._context)) {}

    ExternalObjective& operator=(const ExternalObjective&) = delete;

    ~ExternalObjective() override { ExternalExample::destroy(_context); }

    double operator()(double** input) const override
    {
        // A single EvoFlow vector variable is already contiguous at input[0].
        return ExternalExample::evaluate(_context, input[0], _dimension);
    }

    eccalcul_functor* copy() override
    {
        // Each compiled problem/subpopulation receives an independent external
        // context. This is required when the benchmark owns mutable scratch state.
        return new ExternalObjective(*this);
    }

private:
    int _dimension;
    ExternalExample::Context* _context;
};

int main()
{
    const int dimension = 5;
    const int population = 20;
    const int maxFes = 5000;

    Problem problem("external-shifted-sphere");
    problem.addVariable("x", -5.0, 5.0, 0.001, dimension, 1, VariableType::continuous);
    ExternalObjective objective(dimension, 1.25);
    problem.addObjective("fitness", 1, true, "x", &objective);

    ConfigBuilder config;
    auto workflow = config.workflow("pso");
    workflow.initializer("Random");
    workflow.topology.pgBest();
    workflow.repair.boundary();
    workflow.strategy.velocityDriven(2, 2.0, 0.7, 0.2);
    workflow.generator.orderedConstruct();
    workflow.evaluator.basic();
    workflow.selector.index(false);
    config.name("external_demo").maxFES(maxFes).manager("Single").gArchive("Basic");
    config.subpopulation("1").size(population).workflow("pso").maxFES(maxFes).archive("Basic");

    Optimizer* optimizer = config.buildOptimizer();
    optimizer->setProblem(&problem);
    optimizer->exe(42);

    Solution* best = nullptr;
    int bestSize = 0;
    optimizer->getBest(best, bestSize);
    const double fitness = best && bestSize > 0 ? best->fitness[0] : INFINITY;
    std::printf("external benchmark best fitness = %.8f\n", fitness);
    delete optimizer;

    const bool ok = std::isfinite(fitness) && fitness < 1.0;
    std::printf("7.external-benchmark tutorial: %s\n", ok ? "DONE" : "FAILED");
    return ok ? 0 : 1;
}
