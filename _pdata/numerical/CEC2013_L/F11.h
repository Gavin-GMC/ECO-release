#ifndef _CEC13_LARGESCALE_COMPETITION_F11_H
#define _CEC13_LARGESCALE_COMPETITION_F11_H

#include "Benchmarks.h"

namespace CEC13L
{
    class F11 :public Benchmarks {
    protected:
    public:
        F11();
        double compute(double* x);
        ~F11();
    };
}

#endif
