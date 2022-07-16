//
// Created by x on 22-7-12.
//

#include <iostream>
#include <iomanip>
#include "chrono"
#include "iostream"
//using namespace date;
using namespace std;
using namespace std::chrono;

int main () {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto time_since_epoch = now.time_since_epoch();
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch) % 1000;
    std::chrono::microseconds us = std::chrono::duration_cast<std::chrono::microseconds>(time_since_epoch) % 1000000;
    std::chrono::nanoseconds ns = std::chrono::duration_cast<std::chrono::nanoseconds>(time_since_epoch) % 1000000000;
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X") << ":"<< ms.count();
    cout << ss.str();
}