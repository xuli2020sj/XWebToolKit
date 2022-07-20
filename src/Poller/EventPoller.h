//
// Created by x on 22-7-4.
//

#ifndef XWebTOOLKIT_EVENTPOLLER_H
#define XWebTOOLKIT_EVENTPOLLER_H

#include <functional>
#include <memory>
#include <unordered_map>
#include <sys/epoll.h>

#include "Thread/thread_pool.h"
#include "Poller/PipeWrapper.h"
#include "Utility/Logger.h"
#include "Thread/TaskExecutor.h"

using PollEventCB = function<void(int event)>;
using PollDelCB = function<void(bool success)>;
using PollAsyncCB = function<void(void)>;
using DelayTask = TaskCancelable<uint64_t(void)>;

class EventPoller : public enable_shared_from_this<EventPoller>, public TaskExecutor {
private:
public:
    Task::Ptr async(TaskIn task, bool may_sync) override;
    Task::Ptr asyncFirst(TaskIn task, bool may_sync) override;
    Task::Ptr async_(TaskIn task, bool may_sync, bool is_first);
private:
    int epoll_fd_ = -1;
    mutex mtx_event_map_;
    ThreadPool::Priority priority_;
    std::mutex mtx_running_;
    binary_semaphore sem_loop_ = std::binary_semaphore(0);
    thread* loop_thread_{nullptr};
    thread::id loop_thread_id_;
    bool exit_flag_{false};
    PipeWrapper pipe_;

    Logger::Ptr logger_;

    //从其他线程切换过来的任务
    std::mutex mtx_task_;
    list<Task::Ptr> _list_task;

    bool isCurrentThread() const;
    DelayTask::Ptr doDelayTask(uint64_t delay_ms, function<uint64_t()> task);
public:
    typedef enum {
        EVENT_READ = 1 << 0,
        EVENT_WRITE = 1 << 1,
        EVENT_ERROR = 1 << 2,
        EVENT_LT = 1 << 3
    } poll_event;

    using Ptr = shared_ptr<EventPoller>;


    explicit EventPoller(ThreadPool::Priority priority = ThreadPool::PRIORITY_HIGHEST);
    void init();
    ~EventPoller() override;
    static EventPoller& getEventPoller(bool enable_self_run = false);
    static Ptr  getCurrentPoller();
    static void destroy();

    int addEvent(int fd, int event, PollEventCB &&cb);
    int delEvent(int fd, PollDelCB&& cb = nullptr);
    int modifyEvent(int fd, int event);
    void onPipeEvent();

    void runLoop(bool blocked, bool ref_self);
    void sync(PollAsyncCB&& cb);
    void async(PollAsyncCB&& cb);

private:
    unordered_map<int, shared_ptr<PollEventCB >> event_map_;
    std::multimap<uint64_t, DelayTask::Ptr> delay_task_map_;
    uint64_t flushDelayTask(uint64_t now);
    uint64_t getMinDelay();
};




class EventPollerPool : public std::enable_shared_from_this<EventPollerPool>, public TaskExecutorGetter {
public:
    using Ptr = std::shared_ptr<EventPollerPool>;
    ~EventPollerPool() = default;
    static EventPollerPool &Instance();
    static void setPoolSize(size_t size = 0);
    EventPoller::Ptr getFirstPoller();
    EventPoller::Ptr getPoller(bool prefer_current_thread = true);
    void preferCurrentThread(bool flag = true);

private:
    EventPollerPool();

private:
    bool prefer_current_thread_ = true;
};

#endif //XWebTOOLKIT_EVENTPOLLER_H
