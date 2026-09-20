// SPDX-License-Identifier: BSD-3-Clause

#include "example-benchmark.h"

#include <stdexcept>
#include <vector>

namespace ExternalExample
{
    struct Context
    {
        int dimension;
        double shift;
        std::vector<double> scratch;
    };

    Context* create(int dimension, double shift)
    {
        if (dimension <= 0) throw std::invalid_argument("dimension must be positive");
        return new Context{ dimension, shift, std::vector<double>(dimension, 0.0) };
    }

    Context* clone(const Context* source)
    {
        if (!source) return nullptr;
        return create(source->dimension, source->shift);
    }

    void destroy(Context* context)
    {
        delete context;
    }

    double evaluate(Context* context, const double* values, int dimension)
    {
        if (!context || !values || dimension != context->dimension)
            throw std::invalid_argument("invalid external benchmark evaluation request");
        double result = 0.0;
        for (int i = 0; i < dimension; ++i)
        {
            context->scratch[i] = values[i] - context->shift;
            result += context->scratch[i] * context->scratch[i];
        }
        return result;
    }
}
