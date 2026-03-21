#ifndef _CEC13_LARGESCALE_COMPETITION_F2_H
#define _CEC13_LARGESCALE_COMPETITION_F2_H

#include "Benchmarks.h"

namespace CEC13L
{
    class F2 :public Benchmarks {
    protected:

    public:
        F2();
        double compute(double* x);
        /* double compute(vector<double> x) ; */
        ~F2();
    };
}

#endif
