//
// Created by x on 22-7-6.
//

#include <thread>
#include "TaskExecutor.h"
#include "Utility/timer.h"
#include <Poller/EventPoller.h>



TaskExecutor::TaskExecutor(uint64_t max_size, uint64_t max_u_sec) {
    auto now_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    last_sleep_time_ = last_wake_time_ = now_time;
    max_size_ = max_size;
    max_u_sec_ = max_u_sec;
}

void TaskExecutor::sleep() {
    sleeping_flag_ = true;
    auto now_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    last_sleep_time_ = now_time;
    uint64_t run_time = last_sleep_time_ - now_time;
    std::lock_guard<std::mutex> lk(mtx_list);
    time_list_.emplace_back(run_time, false);
    if (time_list_.size() > max_size_) {
        time_list_.pop_front();
    }
}

void TaskExecutor::wake() {
    sleeping_flag_ = false;
    auto now_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    last_wake_time_ = now_time;
    uint64_t sleep_time = now_time - last_sleep_time_;
    std::lock_guard<std::mutex> lk(mtx_list);
    time_list_.emplace_back(sleep_time, true);
    if (time_list_.size() > max_size_) {
        time_list_.pop_front();
    }
}

int TaskExecutor::load() {
    std::lock_guard<std::mutex> lk(mtx_list);
    uint64_t total_sleep_time = 0;
    uint64_t total_run_time = 0, total_time = 0;
    std::for_each(time_list_.begin(), time_list_.end(), [&] (const TimeRecord& time_record) {
        if (time_record.sleep_) {
            total_sleep_time += time_record.time_;
        } else {
            total_run_time += time_record.time_;
        }
    });

    auto now_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    if (sleeping_flag_) {
        total_sleep_time += now_time - last_sleep_time_;
    } else {
        total_run_time += now_time - last_wake_time_;
    }

    total_time = total_run_time + total_sleep_time;

    while (!time_list_.empty() && (total_time > max_u_sec_ || time_list_.size() > max_size_)){
        auto& temp = time_list_.front();
        if (temp.sleep_) {
            total_sleep_time -= temp.time_;
        } else {
            total_run_time -= temp.time_;
        }
    }
    if (total_time == 0) return 0;
    return int(total_run_time * 100 /total_time);
}

void TaskExecutor::syncExecuteTask() {

}

TaskExecutor::~TaskExecutor() {
}


Task::Ptr TaskExecutor::asyncFirst(TaskIn task, bool is_sync) {
    return async(std::move(task), is_sync);
}

TaskExecutor::Ptr TaskExecutorGetter::getExecutor() {
    auto thread_pos = thread_pos_;
    if (thread_pos >= threads_.size()) {
        thread_pos = 0;
    }

    TaskExecutor::Ptr executor_min_load = threads_[thread_pos];
    auto min_load = executor_min_load->load();

    for (size_t i = 0; i < threads_.size(); ++i, ++thread_pos) {
        if (thread_pos >= threads_.size()) {
            thread_pos = 0;
        }

        auto th = threads_[thread_pos];
        auto load = th->load();

        if (load < min_load) {
            min_load = load;
            executor_min_load = th;
        }
        if (min_load == 0) {
            break;
        }
    }
    thread_pos_ = thread_pos;
    return executor_min_load;
}

std::vector<int> TaskExecutorGetter::getExecutorLoad() {
    vector<int> vec(threads_.size());
    int i = 0;
    for (auto &executor : threads_) {
        vec[i++] = executor->load();
    }
    return vec;
}

void TaskExecutorGetter::getExecutorDelay(const std::function<void(const std::vector<int> &)> &callback) {
    std::shared_ptr<vector<int> > delay_vec = std::make_shared<vector<int>>(threads_.size());
    shared_ptr<void> finished(nullptr, [callback, delay_vec](void *) {
        //此析构回调触发时，说明已执行完毕所有async任务
        callback((*delay_vec));
    });
    int index = 0;
    for (auto &th : threads_) {
        std::shared_ptr<Timer> delay_ticker = std::make_shared<Timer>();

        th->async([finished, delay_vec, index, delay_ticker]() {
            (*delay_vec)[index] = delay_ticker->getTimerMicroSec();
        }, false);
        ++index;
    }
}

void TaskExecutorGetter::for_each(const std::function<void(const TaskExecutor::Ptr &)> &cb) {
    for (auto& th : threads_) {
        cb(th);
    }
}

size_t TaskExecutorGetter::getExecutorSize() const {
    return threads_.size();
}

size_t TaskExecutorGetter::addPoller(const std::string &name, size_t size, int priority, bool register_thread) {
    auto cpus = std::thread::hardware_concurrency();
    size = size > 0 ? size : cpus;
    for (size_t i = 0; i < size; ++i) {
        EventPoller::Ptr poller(new EventPoller((ThreadPool::Priority) priority));
        poller->runLoop(false, register_thread);
        auto full_name = name + " " + to_string(i);
        poller->async([i, cpus, full_name, this]() {
            bool ret = setThreadAffinity(i % cpus);
            if (!ret) {
                InfoL << "set thread affinity failed";
            }
        });
        threads_.emplace_back(std::move(poller));
    }
    return size;
}

bool TaskExecutorGetter::setThreadAffinity(int i) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    if (i >= 0) {
        CPU_SET(i, &mask);
    } else {
        for (auto j = 0u; j < std::thread::hardware_concurrency(); ++j) {
            CPU_SET(j, &mask);
        }
    }
    if (!pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask)) {
        return true;
    }
    return false;
}

std::string TaskExecutorGetter::limitString(const char *name, size_t max_size) {
    std::string str = name;
    if (str.size() + 1 > max_size) {
        auto erased = str.size() + 1 - max_size + 3;
        str.replace(5, erased, "...");
    }
    return str;
}

void TaskExecutorGetter::setThreadName(const char *name) {
    pthread_setname_np(pthread_self(), limitString(name, 16).data());
}
