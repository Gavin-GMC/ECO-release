#ifndef _CEC13_LARGESCALE_COMPETITION_F1_H
#define _CEC13_LARGESCALE_COMPETITION_F1_H

#include "Benchmarks.h"

namespace CEC13L
{
    class F1 :public Benchmarks {
    public:
        F1();
        double compute(double* x);
        /* double compute(vector<double> x) ; */
        ~F1();
    };
}

#endif
