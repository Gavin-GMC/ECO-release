#ifndef _CEC13_LARGESCALE_COMPETITION_F14_H
#define _CEC13_LARGESCALE_COMPETITION_F14_H

#include "Benchmarks.h"

namespace CEC13L
{
    class F14 :public Benchmarks {
    public:
        F14();
        double compute(double* x);
        ~F14();
    };
}

#endif

