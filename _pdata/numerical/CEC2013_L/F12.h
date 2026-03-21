#ifndef _CEC13_LARGESCALE_COMPETITION_F12_H
#define _CEC13_LARGESCALE_COMPETITION_F12_H

#include "Benchmarks.h"

namespace CEC13L{
    class F12 :public Benchmarks {
    protected:
    public:
        F12();
        double compute(double* x);
        ~F12();

    };

}
#endif
