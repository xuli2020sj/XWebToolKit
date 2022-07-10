//
// Created by x on 22-7-8.
//

#ifndef XLTOOLKIT_ASYNCTASKTHREAD_H
#define XLTOOLKIT_ASYNCTASKTHREAD_H


#include <cstdio>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <chrono>
#include <condition_variable>
#include <utility>
#include <map>
#include <queue>
#include "thread_pool.h"
#include "Utility/Logger.h"

#define TASK_INTERVAL 50ms

using namespace std;

typedef struct TaskInfo{
    size_t task_id_;
    size_t execute_time_stamp_;
    size_t execute_time_interval_;
    std::function<bool()> task_;
    bool operator<(TaskInfo& other) const {
        return this->execute_time_stamp_ < other.execute_time_stamp_;
    }
    TaskInfo() {}
    TaskInfo(size_t task_id, size_t execute_time_point,  size_t execute_time_interval,
             std::function<bool()> task) : task_id_(task_id),
                                           execute_time_stamp_(execute_time_point), execute_time_interval_(execute_time_interval), task_(std::move(task)){}
} TaskInfo;

auto cmp = [](shared_ptr<TaskInfo>& s1, shared_ptr<TaskInfo>& s2){
    return s1->execute_time_stamp_ > s2->execute_time_stamp_;
};

class AsyncTaskThread {
    using TaskInfoPtr = shared_ptr<TaskInfo>;
private:
    mutable mutex mtx_;
    bool exit_flag_;
    thread* task_thread_;
    condition_variable_any cv;
    chrono::milliseconds task_interval_;
    priority_queue<shared_ptr<TaskInfo>, vector<shared_ptr<TaskInfo>>, decltype(cmp)> task_pq_;
    void doTask();
    ThreadPool* threadPool_;

public:
    explicit AsyncTaskThread(chrono::milliseconds task_interval);
    virtual ~AsyncTaskThread();
    static AsyncTaskThread& getAsyncTaskThread() {
        static auto* instance = new AsyncTaskThread(TASK_INTERVAL);
        return *instance;
    }

    static void destroy() {
        delete &getAsyncTaskThread();
    }

    void doTaskDelay(size_t task_id, size_t milliseconds, const function<bool()>& func);
};


#endif //XLTOOLKIT_ASYNCTASKTHREAD_H
