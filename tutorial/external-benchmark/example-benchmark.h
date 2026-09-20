#pragma once

// A tiny project-owned C API used only to demonstrate how an independently
// acquired benchmark can be linked into a user's EvoFlow host program.
namespace ExternalExample
{
    struct Context;

    Context* create(int dimension, double shift);
    Context* clone(const Context* source);
    void destroy(Context* context);
    double evaluate(Context* context, const double* values, int dimension);
}
