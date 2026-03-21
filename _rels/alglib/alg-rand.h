#ifndef ALG_RAND_H
#define ALG_RAND_H

namespace alglib {
    
    void setseed(unsigned int seed);
    
    //return a value between 0-1
    double rand01();

    //return a value between 0-1 but not be 0 or 1
    double rand01_();

    // return a random int between 0-2147483648
    int wide_rand();

    template<class T>
    void shuffle(T* arr, size_t n);
}

#endif // !ALG_RAND_H



