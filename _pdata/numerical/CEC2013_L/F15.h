#ifndef _CEC13_LARGESCALE_COMPETITION_F15_H
#define _CEC13_LARGESCALE_COMPETITION_F15_H

#include "Benchmarks.h"

namespace CEC13L
{
    class F15 :public Benchmarks {
    protected:
        void generateInterArray();
    public:
        F15();
        double compute(double* x);
        ~F15();
    };
}

#endif
