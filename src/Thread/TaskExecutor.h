//
// Created by x on 22-7-6.
//

#ifndef XLTOOLKIT_TASKEXECUTOR_H
#define XLTOOLKIT_TASKEXECUTOR_H

#include <cstdint>
#include <mutex>
#include <list>
#include <functional>
#include "memory"
#include <semaphore>




template<class R, class... ArgTypes>
class TaskCancelable;

template<class R, class... ArgTypes>
class TaskCancelable<R(ArgTypes...)>{
public:
    using Ptr = std::shared_ptr<TaskCancelable>;
    using func_type = std::function<R(ArgTypes...)>;

    ~TaskCancelable() = default;

    template<typename FUNC>
    TaskCancelable(FUNC &&task) {
        _strongTask = std::make_shared<func_type>(std::forward<FUNC>(task));
        _weakTask = _strongTask;
    }

    void cancel()  {
        _strongTask = nullptr;
    }

    operator bool() {
        return _strongTask && *_strongTask;
    }

    void operator=(std::nullptr_t) {
        _strongTask = nullptr;
    }

    R operator()(ArgTypes ...args) const {
        auto strongTask = _weakTask.lock();
        if (strongTask && *strongTask) {
            return (*strongTask)(std::forward<ArgTypes>(args)...);
        }
        return defaultValue<R>();
    }

    template<typename T>
    static typename std::enable_if<std::is_void<T>::value, void>::type
    defaultValue() {}

    template<typename T>
    static typename std::enable_if<std::is_pointer<T>::value, T>::type
    defaultValue() {
        return nullptr;
    }

    template<typename T>
    static typename std::enable_if<std::is_integral<T>::value, T>::type
    defaultValue() {
        return 0;
    }

protected:
    std::weak_ptr<func_type> _weakTask;
    std::shared_ptr<func_type> _strongTask;
};

using TaskIn = std::function<void()>;
using Task = TaskCancelable<void()>;

class TaskExecutor {
public:
    using Ptr = std::shared_ptr<TaskExecutor>;

    explicit TaskExecutor(uint64_t max_size = 32, uint64_t max_u_sec = 2 * 1000 * 1000);;
    virtual ~TaskExecutor();

    void sleep();
    void wake();
    int load();

    virtual Task::Ptr async(TaskIn task, bool is_sync) = 0;
    virtual Task::Ptr asyncFirst(TaskIn task, bool is_sync);
    void syncExecuteTask();

private:
    struct TimeRecord {
        bool sleep_;
        uint64_t time_;
        TimeRecord(uint64_t run_time, bool sleep_flag) {
            time_ = run_time;
            sleep_ = sleep_flag;
        }
    };

    bool sleeping_flag_ = true;
    uint64_t last_sleep_time_;
    uint64_t last_wake_time_;
    uint64_t max_size_;
    uint64_t max_u_sec_;
    std::mutex mtx_list;
    std::list<TimeRecord> time_list_;
};

class TaskExecutorGetter {
    using Ptr = std::shared_ptr<TaskExecutorGetter>;
public:
    TaskExecutorGetter() = default;
    ~TaskExecutorGetter() = default;

    TaskExecutor::Ptr getExecutor() ;
    std::vector<int> getExecutorLoad();
    void getExecutorDelay(const std::function<void(const std::vector<int> &)> &callback);
    void for_each(const std::function<void(const TaskExecutor::Ptr &)> &cb);
    size_t getExecutorSize() const ;

protected:
    size_t addPoller(const std::string &name, size_t size, int priority, bool register_thread);
    size_t thread_pos_ = 0;
    std::vector<TaskExecutor::Ptr> threads_;

private:
    static bool setThreadAffinity(int i);
    static std::string limitString(const char *name, size_t max_size);
    static void setThreadName(const char *name);
};

#endif //XLTOOLKIT_TASKEXECUTOR_H
