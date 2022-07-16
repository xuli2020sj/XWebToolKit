//
// Created by x on 22-7-10.
//



#include "Thread/AsyncTaskThread.h"
#include "iostream"
#include "signal.h"
#include "Utility/Logger.h"


using namespace std;

int main() {
    Logger::getLogger().addChannel(std::make_shared<ConsoleChannel>("s", Trace));
    Logger::getLogger().setWriter(std::make_shared<AsyncLogWriter>());

    TRACE << "start";
    AsyncTaskThread::getAsyncTaskThread().doTaskDelay(1, 1000, [](){
        cerr << "do task delay 1s  "<<  chrono::time_point_cast<chrono::milliseconds>(chrono::system_clock::now()).time_since_epoch().count()
        << endl;

        return true;
    });

    AsyncTaskThread::getAsyncTaskThread().doTaskDelay(1, 5000, [](){
        cerr << "do task delay 5s  "<< chrono::time_point_cast<chrono::milliseconds>(chrono::system_clock::now()).time_since_epoch().count()
                                  << endl;
        return true;
    });
    AsyncTaskThread::getAsyncTaskThread().doTaskDelay(1, 2000, [](){
        cerr << "do task delay 2s  "<< chrono::time_point_cast<chrono::milliseconds>(chrono::system_clock::now()).time_since_epoch().count()
                                  << endl;
        return true;
    });

    AsyncTaskThread::getAsyncTaskThread().doTaskDelay(1, 3000, [](){
        cerr << "do task delay 3s  "<< chrono::time_point_cast<chrono::milliseconds>(chrono::system_clock::now()).time_since_epoch().count()
                                  << endl;
        return true;
    });


    TRACE << "finished";
    this_thread::sleep_for(6s);
    AsyncTaskThread::destroy();

}