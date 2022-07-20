#ifndef XLTOOLKIT_THREAD_POOL_H
#define XLTOOLKIT_THREAD_POOL_H

#include "task_queue.h"
#include "thread_group.h"


class ThreadPool {
public:
    enum Priority {
        PRIORITY_LOWEST = 0,
        PRIORITY_LOW,
        PRIORITY_NORMAL,
        PRIORITY_HIGH,
        PRIORITY_HIGHEST
    };
private:
    size_t thread_num_;
    TaskQueue task_queue_;
    ThreadGroup thread_group_;
    Priority priority_;
    volatile bool available_;

    void init() {
        if (thread_num_ > 0) {
            for (int i = 0; i < thread_num_; i++) {
                thread_group_.createThread([this] { run(); });
            }
            available_ = true;
        }
    }

    void run() {
        setPriority(priority_);
        function<void(void)> task;
        while (true) {
            if (task_queue_.getTask(task)) {
                try {
                    task();
                } catch (std::exception& exception) {
                    //todo
                }
                task = nullptr;
            } else {
                break;
            }
        }
    }

public:
    explicit ThreadPool(int thread_num = thread::hardware_concurrency(), Priority priority = PRIORITY_NORMAL) :
    thread_num_(thread_num), priority_(priority), available_(false) {
        init();
    }

    static ThreadPool &getThreadPool() {
        static ThreadPool instance(thread::hardware_concurrency());
        return instance;
    }

    static bool setPriority(Priority priority = PRIORITY_NORMAL, thread::native_handle_type threadId = 0) {
        static int Min = sched_get_priority_min(SCHED_OTHER);
        if (Min == -1) {
            return false;
        }
        static int Max = sched_get_priority_max(SCHED_OTHER);
        if (Max == -1) {
            return false;
        }
        static int Priorities[] = { Min, Min + (Max - Min) / 4, Min
                                                                + (Max - Min) / 2, Min + (Max - Min) / 4, Max };

        if (threadId == 0) {
            threadId = pthread_self();
        }
        struct sched_param params{Priorities[priority]};
        return pthread_setschedparam(threadId, SCHED_OTHER, &params) == 0;
    }

    template<typename T>
    bool async(T &&task) {
        if (!available_) return false;
        if (thread_group_.isThisThreadIn()) {
            task();
        } else {
            task_queue_.pushTaskBack(std::forward<T>(task));
        }
        return true;
    }

    template<typename T>
    bool asyncFirst(T&& task) {
        if (!available_) return false;
        if (thread_group_.isThisThreadIn()) {
            task();
        } else {
            task_queue_.pushTaskFront(forward<T>(task));
        }
        return true;
    }

    template<typename T>
    bool sync(T&& task) {
        if (!available_) return false;
        binary_semaphore sem(0);
        bool flag = asyncFirst([&](){
            task();
            sem.release();
        });
        if (flag) {
            sem.acquire();
        }
        return flag;
    }

    void waitAndExit() {
        available_ = false;
        task_queue_.waitAndExit(thread_num_);
        thread_group_.joinAllThreads();
    }

};




#endif //XLTOOLKIT_THREAD_POOL_H
