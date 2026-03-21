#ifndef _CEC13_LARGESCALE_COMPETITION_F10_H
#define _CEC13_LARGESCALE_COMPETITION_F10_H

#include "Benchmarks.h"

namespace CEC13L
{
    class F10 :public Benchmarks {
    protected:
    public:
        F10();
        double compute(double* x);
        ~F10();
    };
}

#endif
