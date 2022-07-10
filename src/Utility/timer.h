#ifndef _Timer_
#define _Timer_

#include <iostream>
#include <chrono>

using namespace std;
using namespace std::chrono;

class Timer
{
public:
    Timer()
    {
        update();
    }

    ~Timer()
    {
    }

    void update()
    {
        start_ = high_resolution_clock::now();
    }

    double getTimerSecond()
    {
        return getTimerMicroSec() * 0.000001;
    }

    double getTimerMilliSec()
    {
        return getTimerMicroSec()*0.001;
    }

    long long getTimerMicroSec()
    {
        return duration_cast<microseconds>(high_resolution_clock::now() - start_).count();
    }
private:
    time_point<high_resolution_clock> start_;
};

#endif