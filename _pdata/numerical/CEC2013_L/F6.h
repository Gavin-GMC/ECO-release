#ifndef _CEC13_LARGESCALE_COMPETITION_F6_H
#define _CEC13_LARGESCALE_COMPETITION_F6_H

#include "Benchmarks.h"

namespace CEC13L {
    class F6 :public Benchmarks {
    protected:
    public:
        F6();
        double compute(double* x);
        ~F6();
    };
}

#endif
