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

using PollEventCB = function<void(int event)>;
using PollDelCB = function<void(bool success)>;
using PollAsyncCB = function<void(void)>;

typedef enum {
    EVENT_READ = 1 << 0,
    EVENT_WRITE = 1 << 1,
    EVENT_ERROR = 1 << 2,
    EVENT_LT = 1 << 3
} poll_event;


class EventPoller {
private:
    int epoll_fd_ = -1;
    unordered_map<int, PollEventCB> event_map_;
    mutex mtx_event_map_;

    ThreadPool::Priority priority_;
    std::mutex mtx_running_;

public:
    EventPoller(bool enable_self_run = false);
    virtual ~EventPoller();
    static EventPoller& getEventPoller(bool enable_self_run = false) {
        static auto* event_poller = new EventPoller(enable_self_run);
        return *event_poller;
    }

    static void destroy() {
        delete &EventPoller::getEventPoller();
    }

    int addEvent(int fd, int event, PollEventCB &&cb);
    int delEvent(int fd, PollDelCB&& cb = nullptr);
    int modifyEvent(int fd, int event);



};


#endif //XWebTOOLKIT_EVENTPOLLER_H
