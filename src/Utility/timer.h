#ifndef _Timer_
#define _Timer_

#include <iostream>
#include <chrono>

using namespace std;
using namespace std::chrono;

class Timer {
public:
    Timer() {
        reset();
    }

    ~Timer() {
    }

    void reset() {
        start_ = std::chrono::steady_clock::now().time_since_epoch();
    }

    uint64_t getTimerSec() {
        auto now_s = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        return now_s - std::chrono::duration_cast<std::chrono::seconds>(start_).count();
    }

    uint64_t getTimerMilliSec() {
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        return now_ms - std::chrono::duration_cast<std::chrono::milliseconds>(start_).count();
    }

    uint64_t getTimerMicroSec() {
        auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        return now_us - std::chrono::duration_cast<std::chrono::microseconds>(start_).count();
    }
private:
    duration<long, ratio<1, 1000000000>> start_;
};

#endif