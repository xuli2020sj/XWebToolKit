//
// Created by x on 2022/7/18.
//

#include <csignal>
#include <iostream>
#include <random>
#include "Utility/Logger.h"
#include "Utility/timer.h"
#include "Poller/EventPoller.h"

using namespace std;

/**
 * cpu负载均衡测试
 * @return
 */

class _StrPrinter : public std::string {
public:
    _StrPrinter() {}

    template<typename T>
    _StrPrinter& operator <<(T && data) {
        _stream << std::forward<T>(data);
        this->std::string::operator=(_stream.str());
        return *this;
    }

    std::string operator <<(std::ostream&(*f)(std::ostream&)) const {
        return *this;
    }

private:
    std::stringstream _stream;
};

int main() {
    static bool  exit_flag = false;
    signal(SIGINT, [](int) { exit_flag = true; });
    //设置日志
    Logger::getLogger().addChannel(std::make_shared<ConsoleChannel>());

    Timer timer;
    std::default_random_engine random;
    while(!exit_flag){
//        InfoL << timer.getTimerMilliSec();
            auto vec = EventPollerPool::Instance().getExecutorLoad();
            stringstream ss;
            for(auto load : vec){
                ss << load << "-";
            }
            DebugL << "cpu负载:" << ss.str();

            EventPollerPool::Instance().getExecutorDelay([](const vector<int> &vec){
                stringstream ss2;
                for(auto delay : vec){
                    ss2 << delay << "-";
                }
                DebugL << "cpu任务执行延时:" << ss2.str();
            });


        EventPollerPool::Instance().getExecutor()->async([&random](){
            auto usec = random() * random() / random() % 4000;
            usleep(usec);
        }, false);
        usleep(40);
    }
    return 0;
}
