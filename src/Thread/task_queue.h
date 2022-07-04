//
// Created by x on 22-7-3.
//

#ifndef XLTOOLKIT_TASK_QUEUE_H
#define XLTOOLKIT_TASK_QUEUE_H

#include <mutex>
#include <semaphore>
#include <functional>
#include <deque>

const size_t sem_num = INT32_MAX;

/**
 * thread safe task queue
 */
using namespace std;
class TaskQueue {
private:
    deque<function<void(void)>> queue_;
    mutable mutex mu_;
    counting_semaphore<sem_num> sem_{0};
public:
    TaskQueue() = default;

    template<typename T>
    void pushTaskBack(T&& task) {
        lock_guard<mutex> lock(mu_);
        queue_.emplace_back(forward<T>(task));
        sem_.release();
    }

    template<typename T>
    void pushTaskFront(T&& task) {
        lock_guard<mutex> lock(mu_);
        queue_.emplace_front(forward<T>(task));
        sem_.release();
    }

    void waitAndExit(size_t n) {
        sem_.release(n);
    }

    bool getTask(function<void(void)> &task) {
        sem_.acquire();
        lock_guard<mutex> lock(mu_);
        if (queue_.empty()) return false;
        task = queue_.front();
        queue_.pop_front();
        return true;
    }

    size_t size() const {
        lock_guard<mutex> lock(mu_);
        return queue_.size();
    }
};

#endif //XLTOOLKIT_TASK_QUEUE_H
