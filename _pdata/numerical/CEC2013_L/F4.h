#ifndef _CEC13_LARGESCALE_COMPETITION_F4_H
#define _CEC13_LARGESCALE_COMPETITION_F4_H

#include "Benchmarks.h"

namespace CEC13L
{
    class F4 :public Benchmarks {
    protected:
    public:
        F4();
        double compute(double* x);
        /* double compute(vector<double> x) ; */
        ~F4();
    };
}

#endif
