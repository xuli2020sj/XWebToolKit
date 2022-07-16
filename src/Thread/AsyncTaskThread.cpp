//
// Created by x on 22-7-8.
//

#include <iostream>
#include "AsyncTaskThread.h"

AsyncTaskThread::AsyncTaskThread(chrono::milliseconds task_interval) : task_interval_(task_interval),
exit_flag_(false)
{
    task_thread_ = new thread(&AsyncTaskThread::doTask, this);
    threadPool_ = &ThreadPool::getThreadPool();
}

AsyncTaskThread::~AsyncTaskThread() {
    if (task_thread_) {
        exit_flag_ = true;
//        cv.notify_one();
//        task_thread_->join();

        threadPool_->waitAndExit();
    }
}

void AsyncTaskThread::doTaskDelay(size_t task_id, size_t milliseconds_, const function<bool()>& func) {
    auto t = make_shared<TaskInfo>(task_id,
                                   chrono::time_point_cast<chrono::milliseconds>(chrono::system_clock::now()).time_since_epoch().count() + milliseconds_,
                                    chrono::milliseconds(milliseconds_).count(),
                                    func);
    {
        lock_guard<mutex> lk(mtx_);
        task_pq_.emplace(t);
    }
    if (!task_pq_.empty()) {
        cv.notify_one();
    }
}

void AsyncTaskThread::doTask() {
    while (!exit_flag_) {
        unique_lock<mutex> ulk(mtx_);
        if (task_pq_.empty()) {
            cv.wait_for(ulk, 2s);
        } else {
            cv.wait_for(ulk, task_interval_);
        }
        uint64_t tp = chrono::time_point_cast<chrono::milliseconds>(chrono::system_clock::now()).time_since_epoch().count();
        if (task_pq_.top()->execute_time_stamp_ < tp) {
            threadPool_->async(task_pq_.top()->task_);
            task_pq_.pop();
        }
    }
}

