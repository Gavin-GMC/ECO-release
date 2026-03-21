#ifndef _CEC13_LARGESCALE_COMPETITION_F13_H
#define _CEC13_LARGESCALE_COMPETITION_F13_H

#include "Benchmarks.h"

namespace CEC13L
{
    class F13 :public Benchmarks {
    protected:
    public:
        F13();
        double compute(double* x);
        ~F13();
    };
}

#endif

