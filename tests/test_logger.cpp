//
// Created by x on 22-7-12.
//


#include <iostream>
#include "Utility/Logger.h"

using namespace std;
class TestLog
{
public:
    template<typename T>
    TestLog(const T &t){
        _ss << t;
    };
    ~TestLog(){};

    //通过此友元方法，可以打印自定义数据类型
    friend ostream& operator<<(ostream& out,const TestLog& obj){
        return out << obj._ss.str();
    }
private:
    stringstream _ss;
};

int main() {
    Logger::getLogger().addChannel(std::make_shared<ConsoleChannel> ());
//    Logger::getLogger().setLoggerLevel(LWarn);
    Logger::getLogger().setWriter(std::make_shared<AsyncLogWriter>());

    InfoL << "测试std::cout风格打印：";

    TraceL << "object int"<< TestLog((int)1);
    DebugL << "object short:"<<TestLog((short)2)  << endl << endl << endl;
    InfoL << "object float:" << TestLog((float)3.12345678)  << endl;
    WarnL << "object double:" << TestLog((double)4.12345678901234567)  << endl;
    ErrorL << "object void *:" << TestLog((void *)0x12345678) << endl;
    ErrorL << "object string:" << TestLog("test string") << endl;

    sleep(1);

    TraceL << "int:"<< (int)1  << endl;
    DebugL << "short:"<< (short)2  << endl;
    InfoL << "float:" << (float)3.12345678  << endl;
    WarnL << "double:" << (double)4.12345678901234567  << endl;
    ErrorL << "void *:" << (void *)0x12345678 << endl;

    ErrorL << "without endl!";


    for (int i = 0; i < 2; ++i) {
        DebugL << "this is a log repeated 2 times " ;
    }

    for (int i = 0; i < 100; ++i) {
        DebugL << "this is a log repeated 100 times " << i;
//        this_thread::sleep_for(chrono::milliseconds(10));
    }



    InfoL << "done!";
    cout << "done" << endl;

    return 0;
}