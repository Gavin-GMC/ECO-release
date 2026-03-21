#ifndef _CEC13_LARGESCALE_COMPETITION_F8_H
#define _CEC13_LARGESCALE_COMPETITION_F8_H

#include "Benchmarks.h"

namespace CEC13L
{
    class F8 :public Benchmarks {
    protected:
    public:
        F8();
        double compute(double* x);
        ~F8();
    };
}

#endif
