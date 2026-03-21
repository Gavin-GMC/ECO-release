#ifndef _CEC13_LARGESCALE_COMPETITION_F5_H
#define _CEC13_LARGESCALE_COMPETITION_F5_H

#include "Benchmarks.h"

namespace CEC13L {
    class F5 :public Benchmarks {
    protected:
    public:
        F5();
        double compute(double* x);
        /* double compute(vector<double> x) ; */
        ~F5();
    };
}

#endif
