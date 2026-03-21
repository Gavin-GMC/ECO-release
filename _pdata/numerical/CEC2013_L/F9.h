#ifndef _CEC13_LARGESCALE_COMPETITION_F9_H
#define _CEC13_LARGESCALE_COMPETITION_F9_H

#include "Benchmarks.h"

namespace CEC13L {
    class F9 :public Benchmarks {
    protected:
    public:
        F9();
        double compute(double* x);
        ~F9();
    };
}

#endif
