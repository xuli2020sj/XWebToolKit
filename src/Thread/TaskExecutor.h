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
#include "Utility/util.h"

namespace xtoolkit {

/**
* cpu负载计算器
*/
class ThreadLoadCounter {
public:
    /**
     * 构造函数
     * @param max_size 统计样本数量
     * @param max_usec 统计时间窗口,亦即最近{max_usec}的cpu负载率
     */
    ThreadLoadCounter(uint64_t max_size, uint64_t max_usec);
    ~ThreadLoadCounter() = default;

    /**
     * 线程进入休眠
     */
    void startSleep();

    /**
     * 休眠唤醒,结束休眠
     */
    void sleepWakeUp();

    /**
     * 返回当前线程cpu使用率，范围为 0 ~ 100
     * @return 当前线程cpu使用率
     */
    int load();

private:
    struct TimeRecord {
        TimeRecord(uint64_t tm, bool slp) {
            _time = tm;
            _sleep = slp;
        }

        bool _sleep;
        uint64_t _time;
    };

private:
    bool _sleeping = true;
    uint64_t _last_sleep_time;
    uint64_t _last_wake_time;
    uint64_t _max_size;
    uint64_t _max_usec;
    std::mutex _mtx;
    std::list<TimeRecord> _time_list;
};

class TaskCancelable : public noncopyable {
public:
    TaskCancelable() = default;
    virtual ~TaskCancelable() = default;
    virtual void cancel() = 0;
};

template<class R, class... ArgTypes>
class TaskCancelableImp;

template<class R, class... ArgTypes>
class TaskCancelableImp<R(ArgTypes...)> : public TaskCancelable {
public:
    using Ptr = std::shared_ptr<TaskCancelableImp>;
    using func_type = std::function<R(ArgTypes...)>;

    ~TaskCancelableImp() = default;

    template<typename FUNC>
    TaskCancelableImp(FUNC &&task) {
        _strongTask = std::make_shared<func_type>(std::forward<FUNC>(task));
        _weakTask = _strongTask;
    }

    void cancel() override {
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
using Task = TaskCancelableImp<void()>;

class TaskExecutor {

};

} // xtoolkit

#endif //XLTOOLKIT_TASKEXECUTOR_H
