#ifndef _CEC13_LARGESCALE_COMPETITION_F3_H
#define _CEC13_LARGESCALE_COMPETITION_F3_H

#include "Benchmarks.h"

namespace CEC13L
{
    class F3 :public Benchmarks {
    protected:

    public:
        F3();
        double compute(double* x);
        /* double compute(vector<double> x) ; */
        ~F3();
    };
}

#endif
