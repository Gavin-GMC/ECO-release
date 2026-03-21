
#ifndef _CEC13_LARGESCALE_COMPETITION_F7_H
#define _CEC13_LARGESCALE_COMPETITION_F7_H

#include "Benchmarks.h"

namespace CEC13L
{
    class F7 :public Benchmarks {
    protected:
    public:
        F7();
        double compute(double* x);
        ~F7();
    };
}

#endif
